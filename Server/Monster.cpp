#include "pch.h"
#include "Monster.h"

#include "TaskQueue.h"
#include "ServerObjectManager.h"
#include "Board.h"
#include "Sector.h"
#include "Player.h"
#include "Session.h"

Monster::Monster()
	:MovingObject(OBJECT_TYPE::MONSTER), m_isActive(false)
{
	// 몬스터 아이디는 30000부터 시작
	static int monsterID = 30000;
	SetID(monsterID);

	if(monsterID < 80'000) {
		m_monType = MONSTER_TYPE::PEACE_FIX;
		SetName("M_PF_" + to_string(monsterID));
	}
	else if(monsterID < 130'000) {
		m_monType = MONSTER_TYPE::PEACE_ROAMING;
		SetName("M_PR_" + to_string(monsterID));
	}
	else if(monsterID < 180'000) {
		m_monType = MONSTER_TYPE::AGRO_FIX;
		SetName("M_AF_" + to_string(monsterID));
	}
	else {
		m_monType = MONSTER_TYPE::AGRO_ROAMING;
		SetName("M_AR_" + to_string(monsterID));
	}

	monsterID++;

#ifdef AI_LUA
	auto L = m_luaState = luaL_newstate();
	luaL_openlibs(L);
	luaL_loadfile(L, "npc.lua");
	lua_pcall(L, 0, 0, 0);

	lua_getglobal(L, "set_uid");
	lua_pushnumber(L, GetID());
	lua_pcall(L, 1, 0, 0);

	lua_register(L, "API_SendMessage", API_SendMessage);
	lua_register(L, "API_get_x", API_get_x);
	lua_register(L, "API_get_y", API_get_y);
	lua_register(L, "API_RANDOM_X", API_get_random_x);
	lua_register(L, "API_RANDOM_Y", API_get_random_y);
#endif
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

			if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

			if(OBJECT_TYPE::PLAYER != static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

			if(MANAGER(Board)->CanSee(GetPos(), obj->GetPos()))
				oldViewList.insert(objID);
		}
	}

	switch(m_monType) {
		case MONSTER_TYPE::PEACE_FIX:
		{
			auto target = m_target.lock();
			if(target == nullptr)
				return;
			
			Trace();
			break;
		}
		case MONSTER_TYPE::PEACE_ROAMING:
		{
			auto target = m_target.lock();
			if(target == nullptr)
				RandomMove();
			else {
				Trace();
			}
			break;
		}
		case MONSTER_TYPE::AGRO_FIX:
		{
			if(SearchTarget())
				Trace();
			break;
		}
		case MONSTER_TYPE::AGRO_ROAMING:
		{
			// AGRO_ROAMING: 원래 위치에서 20x20 공간 자유롭게 이동하다가, 11x11영역 근처에 플레이어 접근 시 쫓아온다.

			// 1. 타겟을 찾는다
			//	- 타겟 존재 시, 타겟을 따라 간다
			//	- 타겟이 존재하지 않을 시, 20x20 범위에서 자유롭게 돌아다닌다.


			// 타겟을 찾앗으면 
			if(SearchTarget()) {
				Trace();
			}
			else {
				// 타겟이 없으면 원래 위치에서 20x20 반경 내에서 Random Move
				RandomMove();
			}
			break;
		}
		default:
			break;
	}

	SetState(MOVING_OBJECT_STATE::MOVE);

	unordered_set<int> newViewList;
	{

		auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(GetPos());

		for(const int secID : neighborSecList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();

			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

				if(OBJECT_TYPE::PLAYER != static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

				if(MANAGER(Board)->CanSee(GetPos(), obj->GetPos()))
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
				sendPkt.id = GetID();
				memcpy(sendPkt.name, GetName().data(), GetName().size());
				sendPkt.name[GetName().size()] = 0;
				sendPkt.x = GetPos().x;
				sendPkt.y = GetPos().y;
				sendPkt.objType = GetObjType();
				sendPkt.dir = GetDir();
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
				sendPkt.dir = GetDir();
				sendPkt.move_time = static_cast<int>(GetLastMoveTime());
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
		std::cout << "MOVE: " << current_time - GetLastMoveTime() << "ms \n";
	}
	SetLastMoveTime(current_time);
}

void Monster::WakeUp(const int wakerID)
{
	MANAGER(TaskQueue)->AddTask(Task{ GetID(), std::chrono::high_resolution_clock::now() +1ms, EVENT_TYPE::HELLO, wakerID });

	if(IsAlive() == false)
		return;
	
	if(m_isActive == true)
		return;

	bool expected{ false };

	if(false == m_isActive.compare_exchange_strong(expected, true)) {
		return;
	}
	else {
		MANAGER(TaskQueue)->AddTask(Task{ GetID(), std::chrono::high_resolution_clock::now() + 1s , EVENT_TYPE::MOVE, 0 });
	}
}

void Monster::Attack(const int targetID)
{

}

void Monster::Revive()
{
	bool expected{ false };

	if(m_alive.compare_exchange_strong(expected, true)) {
		SetHP(GetMaxHP());
		SetAlive(true);
		SetActive(false);
		SetState(MOVING_OBJECT_STATE::IDLE);
		m_target.reset();

		SC_OBJECT_STATE_PACKET sendPkt;
		sendPkt.size = sizeof(sendPkt);
		sendPkt.type = SC_OBJECT_STATE;
		sendPkt.objType = GetObjType();
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

				if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

				if(OBJECT_TYPE::PLAYER != static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

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

std::vector<Pos> Monster::DoAstar()
{
	auto player = m_target.lock();

	if(nullptr == player)
		return{};

	const Pos startPos = GetPos();
	const Pos destPos = player->GetPos();

	static const Pos dirs[4] = { Pos{1,0}, Pos{-1,0}, Pos{0,1}, Pos{0,-1} };

	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
	std::unordered_map<Pos, int, PosHash> gScore;
	std::unordered_map<Pos, Pos, PosHash> cameFrom;

	gScore[startPos] = 0;
	openSet.push({ startPos, 0, heuristic(startPos, destPos) });

	while(!openSet.empty()) {
		Node current = openSet.top();
		openSet.pop();

		if(current.pos == destPos)
			return reconstructPath(cameFrom, current.pos);

		for(const Pos& d : dirs) {
			Pos neighbor{ static_cast<short>(current.pos.x + d.x),
						  static_cast<short>(current.pos.y + d.y) };
			if(neighbor.x < 0 || neighbor.x >= Board::BOARD_WIDTH
				|| neighbor.y < 0 || neighbor.y >= Board::BOARD_HEIGHT)
				continue;
			if(MANAGER(Board)->m_boards[neighbor.y][neighbor.x] == TILE_TYPE::OBSTACLE)
				continue;

			int tentativeG = current.g + 1;
			if(!gScore.count(neighbor) || tentativeG < gScore[neighbor]) {
				gScore[neighbor] = tentativeG;
				int fScore = tentativeG + heuristic(neighbor, destPos);
				openSet.push({ neighbor, tentativeG, fScore });
				cameFrom[neighbor] = current.pos;
			}
		}	
	}

	return{};
}

std::vector<Pos> Monster::reconstructPath(const std::unordered_map<Pos, Pos, PosHash>& cameFrom, Pos current)
{
	std::vector<Pos> path;
	while(cameFrom.find(current) != cameFrom.end()) {
		path.push_back(current);
		current = cameFrom.at(current);
	}
	path.push_back(current); // 시작 위치
	std::reverse(path.begin(), path.end());
	return path;
}

void Monster::Trace()
{
	auto path = DoAstar();

	Pos prevPos = GetPos();

	if(path.size() == 0)
		return;

	if(path.size() > 2) {
		Pos nextPos = path[1];
		SetDir(GetLookDir(prevPos, nextPos));
		SetPos(path[1]);
	}
	else {
		auto target = m_target.lock();
		if(target != nullptr && target->IsAlive()) {
			const int targetID = target->GetID();
			MANAGER(TaskQueue)->AddTask(Task{ GetID(), std::chrono::high_resolution_clock::now() + 1s, EVENT_TYPE::ATTACK, targetID });
		}
	}
}

void Monster::RandomMove()
{
	// 20x20 위치를 자유롭게...
	static constexpr int MOVE_RADIUS = 10;  
	
	const Pos prevPos{ GetPos() };
	
	Pos nextPos{ prevPos };

	switch(rand() % 4) {
		case MOVE_UP:
			SetDir(DIRECTION_TYPE::UP);
			nextPos.y -= 1;
			break;
		case MOVE_DOWN:
			SetDir(DIRECTION_TYPE::DOWN);
			nextPos.y += 1;
			break;
		case MOVE_LEFT:
			SetDir(DIRECTION_TYPE::LEFT);
			nextPos.x -= 1;
			break;
		case MOVE_RIGHT:
			SetDir(DIRECTION_TYPE::RIGHT);
			nextPos.x += 1;
			break;
		default:
			break;
	}

	if(std::abs(nextPos.x - prevPos.x) <= MOVE_RADIUS
		&& std::abs(nextPos.y - prevPos.y) <= MOVE_RADIUS
		&& MANAGER(Board)->CanGo(nextPos)) 
	{
		auto oldSector = MANAGER(Board)->GetSector(prevPos);
		auto newSector = MANAGER(Board)->GetSector(nextPos);

		if(oldSector not_eq newSector) {
			oldSector->Remove(GetID());
			newSector->Add(GetID());
		}
		SetPos(nextPos);
	}
}

bool Monster::SearchTarget()
{
	const Pos myPos = GetPos();

	auto neighborSectorList = MANAGER(Board)->GetNeighborSectorList(myPos, 5);

	for(const int sectorID : neighborSectorList) {
		auto sector = MANAGER(Board)->GetSector(sectorID);

		auto objList = sector->GetObjList();

		for(const int objID : objList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(objID == GetID())
				continue;

			if(obj == nullptr || obj->GetServerState() != SERVER_STATE::ST_INGAME || static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER)
				continue;

			const Pos objPos = obj->GetPos();

			if(MANAGER(Board)->CanSee(GetPos(), obj->GetPos()), 11) {
				m_target = std::static_pointer_cast<Player>(obj);
				return true;
			}
		}
	}

	return false;
}

DIRECTION_TYPE Monster::GetLookDir(const Pos prev, const Pos next)
{
	Pos dir = next - prev;

	if(dir.x > 0)
		return DIRECTION_TYPE::RIGHT;
	else if(dir.x < 0)
		return DIRECTION_TYPE::LEFT;
	else if(dir.y > 0)
		return DIRECTION_TYPE::DOWN;
	else
		return DIRECTION_TYPE::UP;
}
