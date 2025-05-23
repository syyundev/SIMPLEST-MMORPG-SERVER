#include "pch.h"
#include "Monster.h"

#include "TaskQueue.h"
#include "ServerObjectManager.h"
#include "Board.h"
#include "Sector.h"
#include "Player.h"
#include "Session.h"

Monster::Monster(const MONSTER_TYPE type)
	:MovingObject(OBJECT_TYPE::MONSTER), m_monType(type), m_isActive(false)
{
	// 몬스터 아이디는 30000부터 시작
	static int monsterID = 30000;
	SetID(monsterID);
	monsterID++;
}

Monster::~Monster()
{
}

void Monster::Move()
{
	if(m_isActive == false || IsAlive() == false)
		return;

	unordered_set<int> oldViewList;

	auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(GetPos());

	for(const int secID : neighborSecList) {
		auto sector = MANAGER(Board)->GetSector(secID);

		auto objList = sector->GetObjList();

		for(const int objID : objList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

			if(OBJECT_TYPE::MONSTER == static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

			if(MANAGER(Board)->CanSee(GetPos(), obj->GetPos()))
				oldViewList.insert(objID);
		}
	}

	Pos prevPos{ GetPos() };
	Pos nextPos{ prevPos };

	switch(rand() % 4) {
		case MOVE_UP:
			nextPos.y -= 1;
			break;
		case MOVE_DOWN:
			nextPos.y += 1;
			break;
		case MOVE_LEFT:
			nextPos.x -= 1;
			break;
		case MOVE_RIGHT:
			nextPos.x += 1;
			break;
		default:
			break;
	}
	
	if(MANAGER(Board)->CanGo(nextPos))
		SetPos(nextPos);
		SetState(MOVING_OBJECT_STATE::MOVE);

		unordered_set<int> newViewList;
	{

		auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(GetPos());

		for(const int secID : neighborSecList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();

			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

				if(OBJECT_TYPE::MONSTER == static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

				if(MANAGER(Board)->CanSee(GetPos(), obj->GetPos()))
					newViewList.insert(objID);
			}
		}

		for(const int objID : newViewList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

			auto player = std::static_pointer_cast<Player>(obj);
			
			if(oldViewList.find(objID) == oldViewList.end()) {

				SC_ADD_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_ADD_OBJECT;
				sendPkt.id = GetID();
				memcpy(sendPkt.name, GetName().data(), GetName().size());
				sendPkt.x = GetPos().x;
				sendPkt.y = GetPos().y;
				sendPkt.objType = GetObjType();
				sendPkt.hp = GetHP();
				sendPkt.maxHP = GetMaxHP();
				sendPkt.exp = GetExp();
					sendPkt.level = GetLevel();
				player->InsertViewList(GetID());
				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
			}
			else {
				SC_MOVE_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_MOVE_OBJECT;
				sendPkt.id = GetID();
				sendPkt.x = GetPos().x;
				sendPkt.y = GetPos().y;
				sendPkt.move_time = static_cast<int>(GetLastMoveTime());
				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
			}
		}

		for(const int objID : oldViewList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);
			auto player = std::static_pointer_cast<Player>(obj);
			if(newViewList.find(objID) == newViewList.end()) {
				player->m_viewLock.lock();
				if(player->m_viewList.find(GetID()) != player->m_viewList.end()) {
					player->m_viewLock.unlock();
					
					player->DeleteViewList(objID);

					SC_REMOVE_OBJECT_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_REMOVE_OBJECT;
					sendPkt.id = GetID();
					sendPkt.objType = GetObjType();

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
	if(30'000 == GetID()) {
		std::cout << "MOVE: " << current_time -GetLastMoveTime() << "ms \n";
	}
	SetLastMoveTime(current_time);

	//MANAGER(TaskQueue)->AddTask(Task{
	//	GetID(),
	//	std::chrono::high_resolution_clock::now() + 1s,
	//	TASK_TYPE::MOVE,
	//	0
	//});
}

void Monster::WakeUp()
{
	if(IsAlive() == false)
		return;

	if(m_isActive == true)
		return;

	bool expected{ false };

	if(false == m_isActive.compare_exchange_strong(expected, true)) {
		return;
	}
	else {
		MANAGER(TaskQueue)->AddTask(Task{ GetID(), std::chrono::high_resolution_clock::now() + 1s , TASK_TYPE::MONSTER_MOVE, 0 });
	}
}

void Monster::Revive()
{
	bool expected{ false };

	if(m_alive.compare_exchange_strong(expected, true)) {
			SetHP(GetMaxHP());
			SetAlive(true);
			SetState(MOVING_OBJECT_STATE::IDLE);

			SC_OBJECT_STATE_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_OBJECT_STATE;
			sendPkt.id = GetID();
			sendPkt.hp = GetHP();
			sendPkt.maxHP = GetMaxHP();
			sendPkt.exp = GetExp();
			sendPkt.level = GetLevel();

			auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(GetPos());

			for(const int secID : neighborSecList) {
				auto sector = MANAGER(Board)->GetSector(secID);

				auto objList = sector->GetObjList();

				for(const int objID : objList) {
					auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

					if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

					if(OBJECT_TYPE::MONSTER == static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

					if(MANAGER(Board)->CanSee(GetPos(), obj->GetPos())) {
						auto player = std::static_pointer_cast<Player>(obj);
						
						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
					}
				}
			}
	}
}
