#include "pch.h"
#include "PacketFunc.h"

#include "Player.h"
#include "Board.h"
#include "ServerObjectManager.h"
#include "Sector.h"	
#include "Session.h"
#include "SessionManager.h"
#include "Monster.h"
#include "TaskQueue.h"
#include "Item.h"
#include "DBManager.h"

void Process_CS_LOGIN_PACKET(const std::shared_ptr<Session>& session, const CS_LOGIN_PACKET& recvPkt)
{
	string name = recvPkt.name;

	const int id = recvPkt.id;

	auto myPlayer = MANAGER(DBManager)->GetUserInfo(recvPkt.id);

	if(nullptr == myPlayer) {
		auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);

		if(obj == nullptr) {
			// 1. DB에 해당 ID가 존재하지 않을 때
			myPlayer = MANAGER(DBManager)->AddUserInfo(id, recvPkt.name);
		}
		else {
			SC_LOGIN_FAIL_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_LOGIN_FAIL;

			if(id < 0 || id >= MONSTER_START_ID) {
				sendPkt.reason = 2; 	// 부적절한 ID
			}
			else {
				// 2. 누군가 해당 ID를 사용중일 때 
				sendPkt.reason = 1;		// 다른 클라에서 사용중
			}

			auto sendBuffer = make_shared<SendBuffer>();
			sendBuffer->Append(sendPkt);
			session->RegistSend(std::move(sendBuffer));
			return;
		}
	}
	if(myPlayer == nullptr)
		return;

	MANAGER(Board)->GetSector(myPlayer->GetPos())->Add(myPlayer->GetID());

	myPlayer->SetOwnerSession(session);
	session->SetPlayer(myPlayer);
	myPlayer->SetServerState(SERVER_STATE::ST_INGAME);
	myPlayer->SetDir(DIRECTION_TYPE::LEFT);
	MANAGER(Board)->GetSector(myPlayer->GetPos())->Add(myPlayer->GetID());

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

		auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(myPlayer->GetPos());

		for(const int secID : neighborSecList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();

			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

				if(obj->GetID() == myPlayer->GetID()) continue;

				if(false == MANAGER(Board)->CanSee(myPlayer->GetPos(), obj->GetPos())) continue;

				switch(auto type = static_cast<OBJECT_TYPE>(obj->GetObjType())) {
					case OBJECT_TYPE::PLAYER:
					{
						auto player = std::static_pointer_cast<Player>(obj);

						SC_ADD_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_ADD_OBJECT;
						sendPkt.id = myPlayer->GetID();
						sendPkt.x = myPlayer->GetPos().x;
						sendPkt.y = myPlayer->GetPos().y;
						memcpy(sendPkt.name, myPlayer->GetName().data(), myPlayer->GetName().size());
						sendPkt.name[myPlayer->GetName().size()] = 0;
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
						monster->WakeUp(myPlayer->GetID());   // 관찰자 추가
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
					sendPkt.name[obj->GetName().size()] = 0;
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
	MANAGER(TaskQueue)->AddTask(Task{ myPlayer->GetID(), std::chrono::high_resolution_clock::now() + 5s ,EVENT_TYPE::HEAL, 0 });
	MANAGER(ServerObjectManager)->AddServerObject(std::move(myPlayer));
}

void Process_CS_MOVE_PACKET(const std::shared_ptr<Session>& session, const CS_MOVE_PACKET& recvPkt)
{
	auto myPlayer = session->GetPlayer();
	if(myPlayer == nullptr) return;
	if(myPlayer->IsAlive() == false || myPlayer->GetServerState() != SERVER_STATE::ST_INGAME)
		return;

	long long current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

#ifdef MOVE_INTERVAL_1S
	long long prevTime = myPlayer->GetLastMoveTime();

	if(current_time - prevTime < 1000) {
		cout << "아직 못움직여!\n";
		return;
	}
#endif

	if(myPlayer->IsAlive() == false)
		return;

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

		auto oldSector = MANAGER(Board)->GetSector(prevPos);
		auto newSector = MANAGER(Board)->GetSector(nextPos);

		if(oldSector not_eq newSector) {
			const int myID{ myPlayer->GetID() };
			MANAGER(Board)->GetSector(prevPos)->Remove(myID);
			MANAGER(Board)->GetSector(nextPos)->Add(myID);
		}

		unordered_set<int> nearList;

		myPlayer->m_viewLock.lock_shared();
		auto oldViewList = myPlayer->m_viewList;
		myPlayer->m_viewLock.unlock_shared();

		auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(myPlayer->GetPos());

		for(const int secID : neighborSecList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();

			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

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

			if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

			if(obj->GetID() == myPlayer->GetID()) continue;

			switch(auto type = static_cast<OBJECT_TYPE>(obj->GetObjType())) {
				case OBJECT_TYPE::PLAYER:
				{
					auto player = std::static_pointer_cast<Player>(obj);

					player->m_viewLock.lock_shared();
					if(player->m_viewList.end() != player->m_viewList.find(myPlayer->GetID())) {
						player->m_viewLock.unlock_shared();


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
						player->m_viewLock.unlock_shared();

						SC_ADD_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_ADD_OBJECT;
						sendPkt.id = myPlayer->GetID();
						sendPkt.x = myPlayer->GetPos().x;
						sendPkt.y = myPlayer->GetPos().y;
						memcpy(sendPkt.name, myPlayer->GetName().data(), myPlayer->GetName().size());
						sendPkt.name[myPlayer->GetName().size()] = 0;
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
					// 몬스터는 1초뒤 깨어나서 움직인다.
					auto monster = std::static_pointer_cast<Monster>(obj);
					monster->WakeUp(myPlayer->GetID());
					break;
				}
				default:
					break;
			}

			// oldViewList에 없는데, 방금 움직여서 nearList에 있다면 -> 새로 등장
			if(oldViewList.find(objID) == oldViewList.end()) {
				SC_ADD_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_ADD_OBJECT;
				sendPkt.id = obj->GetID();
				sendPkt.x = obj->GetPos().x;
				sendPkt.y = obj->GetPos().y;
				memcpy(sendPkt.name, obj->GetName().data(), obj->GetName().size());
				sendPkt.name[obj->GetName().size()] = 0;
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
				myPlayer->InsertViewList(objID);

				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				session->RegistSend(std::move(sendBuffer));
			}
		}

		// oldViewList에 있는데, 방금 움직여서 nearList에 없다면 -> 나갔다.
		for(const int objID : oldViewList) {
			if(nearList.find(objID) == nearList.end()) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

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

	if(myPlayer == nullptr) return;

	if(myPlayer->IsAlive() == false || myPlayer->GetServerState() != SERVER_STATE::ST_INGAME)
		return;

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

			if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

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

void Process_CS_ITEM_PICK_UP_PACKET(const std::shared_ptr<Session>& session, const CS_ITEM_PICK_UP_PACKET& recvPkt)
{
	auto myPlayer = session->GetPlayer();

	if(myPlayer == nullptr) return;

	if(myPlayer->IsAlive() == false || myPlayer->GetServerState() != SERVER_STATE::ST_INGAME)
		return;

	const Pos myPos = myPlayer->GetPos();

	auto sectorList = MANAGER(Board)->GetNeighborSectorList(myPos);

	for(const int sectorID : sectorList) {
		const auto sector = MANAGER(Board)->GetSector(sectorID);

		const auto objList = sector->GetObjList();

		for(const int objID : objList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

			if(OBJECT_TYPE::ITEM != static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

			const Pos itemPos = obj->GetPos();

			// TODO: Player는 체력을 회복한다.
			auto item = std::static_pointer_cast<Item>(obj);

			if(myPos == itemPos) {
				cout << "아이템 먹고 체력 회복!" << endl;
				myPlayer->AddHP(10);

				// 나에게 상태 정보 보내주기
				{
					SC_OBJECT_STATE_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_OBJECT_STATE;
					sendPkt.id = myPlayer->GetID();
					sendPkt.objType = myPlayer->GetObjType();
					sendPkt.hp = myPlayer->GetHP();
					sendPkt.maxHP = myPlayer->GetMaxHP();
					sendPkt.exp = myPlayer->GetExp();
					sendPkt.level = myPlayer->GetLevel();

					auto sendBuffer{ make_shared<SendBuffer>() };
					sendBuffer->Append(sendPkt);
					session->RegistSend(std::move(sendBuffer));
				}

				{
					SC_REMOVE_OBJECT_PACKET	 sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_REMOVE_OBJECT;
					sendPkt.id = item->GetID();
					sendPkt.objType = item->GetObjType();

					auto sendBuffer{ make_shared<SendBuffer>() };
					sendBuffer->Append(sendPkt);
					session->RegistSend(std::move(sendBuffer));
				}

				// Item을 먹었으면 아이템은 맵에서 사라져야 한다.
				// 1. Sector에서 삭제
				// 2. ServerObjectManager에서 삭제


				{
					auto sectorList = MANAGER(Board)->GetNeighborSectorList(item->GetPos());
					for(const int sectorID : sectorList) {
						const auto sector = MANAGER(Board)->GetSector(sectorID);
						const auto objList = sector->GetObjList();

						for(const int objID : objList) {
							auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

							if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

							if(OBJECT_TYPE::PLAYER != static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;
							{
								SC_REMOVE_OBJECT_PACKET	 sendPkt;
								sendPkt.size = sizeof(sendPkt);
								sendPkt.type = SC_REMOVE_OBJECT;
								sendPkt.id = item->GetID();
								sendPkt.objType = item->GetObjType();

								auto sendBuffer{ make_shared<SendBuffer>() };
								sendBuffer->Append(sendPkt);
								std::static_pointer_cast<Player>(obj)->GetOwnerSession()->RegistSend(std::move(sendBuffer));
							}

							// 다른 애들에게 상태 정보 보내주기
							{
								SC_OBJECT_STATE_PACKET sendPkt;
								sendPkt.size = sizeof(sendPkt);
								sendPkt.type = SC_OBJECT_STATE;
								sendPkt.id = myPlayer->GetID();
								sendPkt.objType = myPlayer->GetObjType();
								sendPkt.hp = myPlayer->GetHP();
								sendPkt.maxHP = myPlayer->GetMaxHP();
								sendPkt.exp = myPlayer->GetExp();
								sendPkt.level = myPlayer->GetLevel();

								auto sendBuffer{ make_shared<SendBuffer>() };
								sendBuffer->Append(sendPkt);
								std::static_pointer_cast<Player>(obj)->GetOwnerSession()->RegistSend(std::move(sendBuffer));
							}
						}
					}

					{
						auto sector = MANAGER(Board)->GetSector(itemPos);
						const int itemID = item->GetID();
						sector->Remove(itemID);
						MANAGER(ServerObjectManager)->RemoveServerObject(itemID);
					}
				}
			}
		}
	}
}

void Process_CS_TELEPORT_PACKET(const std::shared_ptr<Session>& session, const CS_TELEPORT_PACKET& recvPkt)
{
	auto player = session->GetPlayer();
	Pos prevPos{ player->GetPos() };

	if(player == nullptr) return;

	if(player->IsAlive() == false || player->GetServerState() != SERVER_STATE::ST_INGAME)
		return;

	Pos tpPos{ -1, -1 };

	while(true) {
		tpPos = Pos{ randomPos(dre), randomPos(dre) };

		if(MANAGER(Board)->CanGo(tpPos))
			break;
	}

	auto oldSector = MANAGER(Board)->GetSector(player->GetPos());
	player->SetPos(tpPos);
	auto newSector = MANAGER(Board)->GetSector(player->GetPos());

	if(oldSector != newSector) {
		oldSector->Remove(player->GetID());
		newSector->Add(player->GetID());
	}
	else {
		newSector->Add(player->GetID());
	}

	unordered_set<int> nearList;

	player->m_viewLock.lock_shared();
	auto oldViewList = player->m_viewList;
	player->m_viewLock.unlock_shared();

	auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(player->GetPos());

	for(const int secID : neighborSecList) {
		auto sector = MANAGER(Board)->GetSector(secID);

		auto objList = sector->GetObjList();

		for(const int objID : objList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

			if(obj->GetID() == player->GetID()) continue;

			if(MANAGER(Board)->CanSee(player->GetPos(), obj->GetPos()))
				nearList.insert(obj->GetID());
		}
	}

	// 나에게 이동좌표 보내주기
	{
		SC_MOVE_OBJECT_PACKET sendPkt;
		sendPkt.size = sizeof(sendPkt);
		sendPkt.type = SC_MOVE_OBJECT;
		sendPkt.id = player->GetID();
		sendPkt.x = player->GetPos().x;
		sendPkt.y = player->GetPos().y;
		sendPkt.move_time = static_cast<int>(player->GetLastMoveTime());
		sendPkt.dir = player->GetDir();

		auto sendBuffer = make_shared<SendBuffer>();
		sendBuffer->Append(sendPkt);
		session->RegistSend(std::move(sendBuffer));
	}

	for(const int objID : nearList) {
		auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

		if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

		if(obj->GetID() == player->GetID()) continue;

		switch(auto type = static_cast<OBJECT_TYPE>(obj->GetObjType())) {
			case OBJECT_TYPE::PLAYER:
			{
				auto player = std::static_pointer_cast<Player>(obj);

				player->m_viewLock.lock_shared();
				if(player->m_viewList.end() != player->m_viewList.find(player->GetID())) {
					player->m_viewLock.unlock_shared();


					SC_MOVE_OBJECT_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_MOVE_OBJECT;
					sendPkt.id = player->GetID();
					sendPkt.x = player->GetPos().x;
					sendPkt.y = player->GetPos().y;
					sendPkt.move_time = static_cast<int>(player->GetLastMoveTime());
					sendPkt.dir = player->GetDir();

					auto sendBuffer = make_shared<SendBuffer>();
					sendBuffer->Append(sendPkt);
					player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
				}
				else {
					player->m_viewLock.unlock_shared();

					SC_ADD_OBJECT_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_OBJECT;
					sendPkt.id = player->GetID();
					sendPkt.x = player->GetPos().x;
					sendPkt.y = player->GetPos().y;
					memcpy(sendPkt.name, player->GetName().data(), player->GetName().size());
					sendPkt.name[player->GetName().size()] = 0;
					sendPkt.objType = player->GetObjType();
					sendPkt.dir = player->GetDir();
					sendPkt.hp = player->GetHP();
					sendPkt.maxHP = player->GetMaxHP();
					sendPkt.exp = player->GetExp();
					sendPkt.level = player->GetLevel();

					player->InsertViewList(player->GetID());

					auto sendBuffer = make_shared<SendBuffer>();
					sendBuffer->Append(sendPkt);
					player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
				}
				break;
			}
			case OBJECT_TYPE::MONSTER:
			{
				// 몬스터는 1초뒤 깨어나서 움직인다.
				auto monster = std::static_pointer_cast<Monster>(obj);
				monster->WakeUp(player->GetID());
				break;
			}
			default:
				break;
		}

		// oldViewList에 없는데, 방금 움직여서 nearList에 있다면 -> 새로 등장
		if(oldViewList.find(objID) == oldViewList.end()) {
			SC_ADD_OBJECT_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_ADD_OBJECT;
			sendPkt.id = obj->GetID();
			sendPkt.x = obj->GetPos().x;
			sendPkt.y = obj->GetPos().y;
			memcpy(sendPkt.name, obj->GetName().data(), obj->GetName().size());
			sendPkt.name[obj->GetName().size()] = 0;
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
			player->InsertViewList(objID);

			auto sendBuffer = make_shared<SendBuffer>();
			sendBuffer->Append(sendPkt);
			session->RegistSend(std::move(sendBuffer));
		}
	}

	// oldViewList에 있는데, 방금 움직여서 nearList에 없다면 -> 나갔다.
	for(const int objID : oldViewList) {
		if(nearList.find(objID) == nearList.end()) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

			player->DeleteViewList(objID);

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

					player->DeleteViewList(player->GetID());

					SC_REMOVE_OBJECT_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_REMOVE_OBJECT;
					sendPkt.id = player->GetID();
					sendPkt.objType = player->GetObjType();

					auto sendBuffer = make_shared<SendBuffer>();
					sendBuffer->Append(sendPkt);
					player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
					break;
				}
				default:
					break;
			}
		}
	}

	println("{}번 플레이어 {},{} -> {},{} TP!", player->GetID(), prevPos.x, prevPos.y, tpPos.x, tpPos.y);
}