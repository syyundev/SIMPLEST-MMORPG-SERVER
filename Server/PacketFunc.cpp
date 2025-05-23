#include "pch.h"
#include "PacketFunc.h"

#include "Player.h"
#include "Board.h"
#include "ServerObjectManager.h"
#include "Sector.h"	
#include "Session.h"
#include "SessionManager.h"
#include "Monster.h"

void Process_CS_LOGIN_PACKET(const std::shared_ptr<Session>& session, const CS_LOGIN_PACKET& recvPkt)
{
	auto myPlayer = make_shared<Player>();
	const Pos myPos{ randomPos(dre), randomPos(dre) };

	myPlayer->SetOwnerSession(session);
	session->SetPlayer(myPlayer);
	myPlayer->SetName(recvPkt.name);
	myPlayer->SetPos(myPos);
	myPlayer->SetStartPos(myPos);
	myPlayer->SetServerState(S_STATE::ST_INGAME);
	myPlayer->SetDir(DIRECTION_TYPE::LEFT);

	MANAGER(Board)->GetSector(myPos)->Add(myPlayer->GetID());

	// 나에게 정보 보내주기.
	{
		SC_LOGIN_INFO_PACKET sendPkt;
		sendPkt.size = sizeof(sendPkt);
		sendPkt.type = SC_LOGIN_INFO;
		sendPkt.id = myPlayer->GetID();
		sendPkt.x = myPlayer->GetPos().x;
		sendPkt.y = myPlayer->GetPos().y;
		sendPkt.dir = myPlayer->GetDir();
		memcpy(sendPkt.name, myPlayer->GetName().data(), myPlayer->GetName().size());
		sendPkt.name[myPlayer->GetName().size()] = 0;
		
		sendPkt.hp = myPlayer->GetHP();
		sendPkt.max_hp = myPlayer->GetMaxHP();
		sendPkt.exp = myPlayer->GetExp();
		sendPkt.level = myPlayer->GetLevel();

		auto sendBuffer = make_shared<SendBuffer>();
		sendBuffer->Append(sendPkt);
		session->RegistSend(std::move(sendBuffer));
	}
	// CS_LOGIN
	// 1. 내 시야 안에 있는 모든 섹터들의 목록을 갖고 온다.
	// 2. 해당 섹터들 안에 있는 모든 오브젝트들을 돌면서
	//		a. 플레이어일 경우 ->  내가 들어왔다고 알려준다
	//		b. 몬스터일 경우 -> 깨워준다.
	// 3. 나에게 상대방에 대한 정보 알려준다.
	{
		// 1. 일단 나의 위치를 알아야 한다.
		// 2. 나의 위치를 알았으면, 나의 시야각을 기준으로 시야각 만큼의 범위를 구한다.
		// 3. 해당 위치에 해당하는 섹터들의 목록을 들고온다.
		// 4. 들고 온 섹터들 목록을 돌면서 각 섹터가 들고 있는 오브젝트들을 가져온다.

		auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(myPos);

		for(const int secID : neighborSecList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();

			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

				if(obj->GetID() == myPlayer->GetID()) continue;

				if(false == MANAGER(Board)->CanSee(myPos, obj->GetPos())) continue;

				switch(auto type = static_cast<OBJECT_TYPE>(obj->GetObjType())) {
					case OBJECT_TYPE::PLAYER:
					{
						auto player = std::static_pointer_cast<Player>(obj);

						SC_ADD_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_ADD_OBJECT;
						sendPkt.id = myPlayer->GetID();
						sendPkt.x = myPos.x;
						sendPkt.y = myPos.y;
						memcpy(sendPkt.name, myPlayer->GetName().data(), myPlayer->GetName().size());
						sendPkt.objType = myPlayer->GetObjType();
						sendPkt.dir = myPlayer->GetDir();
						sendPkt.hp = myPlayer->GetHP();
						sendPkt.maxHP = myPlayer->GetMaxHP();
						sendPkt.exp = myPlayer->GetExp();
						sendPkt.level = myPlayer->GetLevel();
						player->InsertViewList(myPlayer->GetID());

						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
						break;
					}
					case OBJECT_TYPE::MONSTER:
					{
						auto monster = std::static_pointer_cast<Monster>(obj);
						monster->WakeUp();   // 관찰자 추가
						break;
					}
					case OBJECT_TYPE::ITEM:
					{

						break;
					}
					default:
						break;
				}

				// 나에게 상대방 정보 보내주기
				{
					SC_ADD_OBJECT_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_OBJECT;
					sendPkt.id = obj->GetID();
					sendPkt.x = obj->GetPos().x;
					sendPkt.y = obj->GetPos().y;
					memcpy(sendPkt.name, obj->GetName().data(), obj->GetName().size());
					sendPkt.objType = obj->GetObjType();
					if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::PLAYER) {
						auto p = std::static_pointer_cast<Player>(obj);
						sendPkt.dir = p->GetDir();
						sendPkt.hp = p->GetHP();
						sendPkt.maxHP = p->GetMaxHP();
						sendPkt.exp = p->GetExp();
						sendPkt.level = p->GetLevel();
					}
					else if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
						auto m = std::static_pointer_cast<Monster>(obj);
						sendPkt.dir = std::static_pointer_cast<Monster>(obj)->GetDir();
						sendPkt.hp = m->GetHP();
						sendPkt.maxHP = m->GetMaxHP();
						sendPkt.exp = m->GetExp();
						sendPkt.level = m->GetLevel();
					}
					
					myPlayer->InsertViewList(obj->GetID());

					auto sendBuffer = make_shared<SendBuffer>();
					sendBuffer->Append(sendPkt);
					session->RegistSend(std::move(sendBuffer));
				}
			}
		}
	}

	MANAGER(ServerObjectManager)->AddServerObject(std::move(myPlayer));
}

void Process_CS_MOVE_PACKET(const std::shared_ptr<Session>& session, const CS_MOVE_PACKET& recvPkt)
{
	auto myPlayer = session->GetPlayer();
	long long current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

#ifdef MOVE_INTERVAL_1S
	long long prevTime = myPlayer->GetLastMoveTime();

	if(current_time - prevTime < 1000) {
		cout << "아직 못움직여!\n";
		return;
	}
#endif

	const Pos prevPos{ myPlayer->GetPos() };
	Pos nextPos{ prevPos };

	myPlayer->SetLastMoveTime(recvPkt.move_time);
	myPlayer->SetDir(static_cast<DIRECTION_TYPE>(recvPkt.direction));

	switch(recvPkt.direction) {
		case MOVE_UP:
		{
			nextPos.y -= 1;
			break;
		}
		case MOVE_DOWN:
		{
			nextPos.y += 1;
			break;
		}
		case MOVE_LEFT:
		{
			nextPos.x -= 1;
			break;
		}
		case MOVE_RIGHT:
		{
			nextPos.x += 1;
			break;
		}
		default:
			break;
	}

	if(prevPos == nextPos)
		return;

	if(MANAGER(Board)->CanGo(nextPos)) {
		myPlayer->SetLastMoveTime(current_time);
		myPlayer->SetPos(nextPos);
		MANAGER(Board)->GetSector(nextPos)->Add(myPlayer->GetID());

		unordered_set<int> nearList;
		myPlayer->m_viewLock.lock();
		auto oldViewList = myPlayer->m_viewList;
		myPlayer->m_viewLock.unlock();

		auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(myPlayer->GetPos());

		for(const int secID : neighborSecList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();

			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

				if(obj->GetID() == myPlayer->GetID()) continue;

				if(MANAGER(Board)->CanSee(myPlayer->GetPos(), obj->GetPos()))
					nearList.insert(obj->GetID());
			}
		}

		// 나에게 이동좌표 보내주기
		{
			SC_MOVE_OBJECT_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_MOVE_OBJECT;
			sendPkt.id = myPlayer->GetID();
			sendPkt.x = myPlayer->GetPos().x;
			sendPkt.y = myPlayer->GetPos().y;
			sendPkt.move_time = static_cast<int>(myPlayer->GetLastMoveTime());
			sendPkt.dir = myPlayer->GetDir();

			auto sendBuffer = make_shared<SendBuffer>();
			sendBuffer->Append(sendPkt);
			session->RegistSend(std::move(sendBuffer));
		}

		for(const int objID : nearList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

			if(obj->GetID() == myPlayer->GetID()) continue;

			switch(auto type = static_cast<OBJECT_TYPE>(obj->GetObjType())) {
				case OBJECT_TYPE::PLAYER:
				{
					auto player = std::static_pointer_cast<Player>(obj);

					player->m_viewLock.lock();
					if(player->m_viewList.end() != player->m_viewList.find(myPlayer->GetID())) {
						player->m_viewLock.unlock();


						SC_MOVE_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_MOVE_OBJECT;
						sendPkt.id = myPlayer->GetID();
						sendPkt.x = myPlayer->GetPos().x;
						sendPkt.y = myPlayer->GetPos().y;
						sendPkt.move_time = static_cast<int>(myPlayer->GetLastMoveTime());
						sendPkt.dir = myPlayer->GetDir();

						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
					}
					else {
						player->m_viewLock.unlock();

						SC_ADD_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_ADD_OBJECT;
						sendPkt.id = myPlayer->GetID();
						sendPkt.x = myPlayer->GetPos().x;
						sendPkt.y = myPlayer->GetPos().y;
						memcpy(sendPkt.name, myPlayer->GetName().data(), myPlayer->GetName().size());
						sendPkt.objType = myPlayer->GetObjType();
						sendPkt.dir = myPlayer->GetDir();
						sendPkt.hp = myPlayer->GetHP();
						sendPkt.maxHP = myPlayer->GetMaxHP();
						sendPkt.exp = myPlayer->GetExp();
						sendPkt.level = myPlayer->GetLevel();

						player->InsertViewList(myPlayer->GetID());

						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
					}
					break;
				}
				case OBJECT_TYPE::MONSTER:
				{
					auto monster = std::static_pointer_cast<Monster>(obj);
					monster->WakeUp();
					break;
				}
				default:
					break;
			}

			if(oldViewList.find(objID) == oldViewList.end()) {
				SC_ADD_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_ADD_OBJECT;
				sendPkt.id = obj->GetID();
				sendPkt.x = obj->GetPos().x;
				sendPkt.y = obj->GetPos().y;
				memcpy(sendPkt.name, obj->GetName().data(), obj->GetName().size());
				sendPkt.objType = obj->GetObjType();
				if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::PLAYER) {
					auto p = std::static_pointer_cast<Player>(obj);
					sendPkt.dir = p->GetDir();
					sendPkt.dir = p->GetDir();
					sendPkt.hp = p->GetHP();
					sendPkt.maxHP = p->GetMaxHP();
					sendPkt.exp = p->GetExp();
					sendPkt.level = p->GetLevel();
				}
				else if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
					auto m = std::static_pointer_cast<Monster>(obj);
					sendPkt.dir = std::static_pointer_cast<Monster>(obj)->GetDir();
					sendPkt.hp = m->GetHP();
					sendPkt.maxHP = m->GetMaxHP();
					sendPkt.exp = m->GetExp();
					sendPkt.level = m->GetLevel();
				}
				myPlayer->InsertViewList(objID);

				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				session->RegistSend(std::move(sendBuffer));
			}
		}

		for(const int objID : oldViewList) {
			if(nearList.find(objID) == nearList.end()) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

				myPlayer->DeleteViewList(objID);

				SC_REMOVE_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_REMOVE_OBJECT;
				sendPkt.id = objID;
				sendPkt.objType = obj->GetObjType();

				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				session->RegistSend(std::move(sendBuffer));

				switch(auto type = static_cast<OBJECT_TYPE>(obj->GetObjType())) {
					case OBJECT_TYPE::PLAYER:
					{
						auto player = std::static_pointer_cast<Player>(obj);

						player->DeleteViewList(myPlayer->GetID());

						SC_REMOVE_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_REMOVE_OBJECT;
						sendPkt.id = myPlayer->GetID();
						sendPkt.objType = myPlayer->GetObjType();

						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
						break;
					}
					case OBJECT_TYPE::MONSTER:
					{
						break;
					}
					case OBJECT_TYPE::ITEM:
					{
						break;
					}
					default:
						break;
				}
			}
		}
	}
}

void Process_CS_ATTACK_PACKET(const std::shared_ptr<Session>& session, const CS_ATTACK_PACKET& recvPkt)
{
	auto myPlayer = session->GetPlayer();

	long long curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#ifdef ATTACK_INTERVAL_1S
	long long prevAttackTime = myPlayer->GetLastAttackTime();
	
	if(curTime - prevAttackTime < 1000) {
		cout << "아직 못 공격해!\n";
		return;
	}
#endif

	const Pos playerPos = myPlayer->GetPos();

	auto sectorList = MANAGER(Board)->GetNeighborSectorList(playerPos);

	for(const int secID : sectorList) {
		auto sector = MANAGER(Board)->GetSector(secID);

		auto objList = sector->GetObjList();

		for(const int objID : objList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

			if(static_cast<OBJECT_TYPE>(obj->GetObjType()) != OBJECT_TYPE::MONSTER) continue;

			auto monster = std::static_pointer_cast<Monster>(obj);

			static array<Pos, 4> attackDir{ Pos{-1,0}, Pos{1,0}, Pos{0,-1}, Pos{0,1} };

			for(const Pos& dir : attackDir) {
				Pos attackPos = playerPos + dir;

				const Pos monsterPos = monster->GetPos();

				if(attackPos == monsterPos) {
					myPlayer->Attack(objID);
					myPlayer->SetLastAttackTime(curTime);
				}
			}
		}
	}
}
