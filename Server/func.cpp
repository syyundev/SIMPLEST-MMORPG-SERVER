#include "pch.h"
#include "func.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "Session.h"
#include "Player.h"
#include "Board.h"
#include "Monster.h"
#include "Sector.h"
#include "TaskQueue.h"
int API_SendMessage(lua_State* L)
{
	cout << "SendMessage" << endl;
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	auto obj = MANAGER(ServerObjectManager)->GetGameObject(user_id);

	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;
	if(static_cast<OBJECT_TYPE>(obj->GetObjType()) != OBJECT_TYPE::PLAYER) return -1;
	
	SC_CHAT_PACKET sendPkt;
	sendPkt.size = sizeof(sendPkt);
	sendPkt.type = SC_CHAT;
	sendPkt.id = my_id;
	strcpy(sendPkt.chat, mess);
	auto sendBuffer{ make_shared<SendBuffer>() };
	sendBuffer->Append(sendPkt);
	std::static_pointer_cast<Player>(obj)->GetOwnerSession()->RegistSend(std::move(sendBuffer));

	cout << mess << endl;

	auto monster = std::static_pointer_cast<Monster>(MANAGER(ServerObjectManager)->GetGameObject(my_id));

	if(monster->GetMoveCount() < 3) {
		bool expected{ false };
		if(monster->m_flag.compare_exchange_strong(expected, true)) {
			MANAGER(TaskQueue)->AddTask(Task{ monster->GetID(), std::chrono::high_resolution_clock::now() + 1s, EVENT_TYPE::MONSTER_RANDOM_MOVE,user_id });
		}
	}
	return 0;
}

int API_get_x(lua_State* L)
{
	const int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto obj = MANAGER(ServerObjectManager)->GetGameObject(user_id);
	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;
	int x = obj->GetPos().x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	const int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto obj = MANAGER(ServerObjectManager)->GetGameObject(user_id);
	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;
	int y = obj->GetPos().y;
	lua_pushnumber(L, y);
	return 1;
}

int API_get_monster_move_count(lua_State* L)
{
	const int monster_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto obj = MANAGER(ServerObjectManager)->GetGameObject(monster_id);
	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;
	int monster_move_count = std::static_pointer_cast<Monster>(obj)->GetMoveCount();
	println("{}", monster_move_count);
	lua_pushnumber(L, monster_move_count);
	return 1;
}

int API_MonsterRandomMove(lua_State* L)
{
	cout << "MonsterRamdomMove" << endl;
	const int monster_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto obj = MANAGER(ServerObjectManager)->GetGameObject(monster_id);
	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;

	auto monster = std::static_pointer_cast<Monster>(obj);

	static constexpr int MOVE_RADIUS = 10;

	const Pos prevPos{ obj->GetPos() };

	Pos nextPos{ prevPos };

	switch(rand() % 4) {
		case MOVE_UP:
			monster->SetDir(DIRECTION_TYPE::UP);
			nextPos.y -= 1;
			break;
		case MOVE_DOWN:
			monster->SetDir(DIRECTION_TYPE::DOWN);
			nextPos.y += 1;
			break;
		case MOVE_LEFT:
			monster->SetDir(DIRECTION_TYPE::LEFT);
			nextPos.x -= 1;
			break;
		case MOVE_RIGHT:
			monster->SetDir(DIRECTION_TYPE::RIGHT);
			nextPos.x += 1;
			break;
		default:
			break;
	}

	if(std::abs(nextPos.x - prevPos.x) <= MOVE_RADIUS
		&& std::abs(nextPos.y - prevPos.y) <= MOVE_RADIUS
		&& MANAGER(Board)->CanGo(nextPos)) {
		auto oldSector = MANAGER(Board)->GetSector(prevPos);
		auto newSector = MANAGER(Board)->GetSector(nextPos);

		if(oldSector not_eq newSector) {
			oldSector->Remove(monster->GetID());
			newSector->Add(monster->GetID());
		}
		monster->SetPos(nextPos);
	}

	unordered_set<int> oldViewList;

	auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(monster->GetPos());

	for(const int secID : neighborSecList) {
		auto sector = MANAGER(Board)->GetSector(secID);

		auto objList = sector->GetObjList();

		for(const int objID : objList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

			if(OBJECT_TYPE::PLAYER != static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

			if(MANAGER(Board)->CanSee(monster->GetPos(), obj->GetPos()))
				oldViewList.insert(objID);
		}
	}


	monster->SetState(MOVING_OBJECT_STATE::MOVE);

	unordered_set<int> newViewList;
	{

		auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(monster->GetPos());

		for(const int secID : neighborSecList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();

			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

				if(OBJECT_TYPE::PLAYER != static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

				if(MANAGER(Board)->CanSee(monster->GetPos(), obj->GetPos()))
					newViewList.insert(objID);
			}
		}

		for(const int objID : newViewList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

			auto player = std::static_pointer_cast<Player>(obj);

			if(oldViewList.find(objID) == oldViewList.end()) {

				SC_ADD_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_ADD_OBJECT;
				sendPkt.id = monster->GetID();
				memcpy(sendPkt.name, monster->GetName().data(), monster->GetName().size());
				sendPkt.name[monster->GetName().size()] = 0;
				sendPkt.x = monster->GetPos().x;
				sendPkt.y = monster->GetPos().y;
				sendPkt.objType = monster->GetObjType();
				sendPkt.dir = monster->GetDir();
				sendPkt.hp = monster->GetHP();
				sendPkt.maxHP = monster->GetMaxHP();
				sendPkt.exp = monster->GetExp();
				sendPkt.level = monster->GetLevel();
				player->InsertViewList(monster->GetID());
				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
			}
			else {
				SC_MOVE_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_MOVE_OBJECT;
				sendPkt.id = monster->GetID();
				sendPkt.x = monster->GetPos().x;
				sendPkt.y = monster->GetPos().y;
				sendPkt.dir = monster->GetDir();
				sendPkt.move_time = static_cast<int>(monster->GetLastMoveTime());
				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				if(player) {
					const auto session = player->GetOwnerSession();
					if(session == nullptr)
						continue;

					session->RegistSend(std::move(sendBuffer));
				}
			}
		}

		for(const int objID : oldViewList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);
			auto player = std::static_pointer_cast<Player>(obj);
			if(newViewList.find(objID) == newViewList.end()) {
				player->m_viewLock.lock();
				if(player->m_viewList.find(monster->GetID()) != player->m_viewList.end()) {
					player->m_viewLock.unlock();

					player->DeleteViewList(objID);

					SC_REMOVE_OBJECT_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_REMOVE_OBJECT;
					sendPkt.id = monster->GetID();
					sendPkt.objType = monster->GetObjType();

					auto sendBuffer = make_shared<SendBuffer>();
					sendBuffer->Append(sendPkt);
					player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
				}
				else {
					player->m_viewLock.unlock();
				}
			}
		}
	}

	long long current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	if(30'000 == monster->GetID()) {
#ifdef DEBUG
		std::cout << "MOVE: " << current_time - GetLastMoveTime() << "ms \n";
#endif
	}
	monster->SetLastMoveTime(current_time);


	return 1;
}

int API_ResetMonsterMoveCount(lua_State* L)
{
	const int monster_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto obj = MANAGER(ServerObjectManager)->GetGameObject(monster_id);
	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;
	std::static_pointer_cast<Monster>(obj)->ResetMoveCount();
	return 1;
}
