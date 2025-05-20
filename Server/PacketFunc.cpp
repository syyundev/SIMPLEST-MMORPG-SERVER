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
	const Pos myPos{ rand() % W_WIDTH, rand() % W_HEIGHT };

	myPlayer->SetOwnerSession(session);
	session->SetPlayer(myPlayer);
	myPlayer->SetName(recvPkt.name);
	myPlayer->SetPos(myPos);
	myPlayer->SetState(S_STATE::ST_INGAME);

	MANAGER(Board)->GetSector(myPos)->Add(myPlayer->GetID());

	// 나에게 정보 보내주기.
	{
		SC_LOGIN_INFO_PACKET sendPkt;
		sendPkt.size = sizeof(sendPkt);
		sendPkt.type = SC_LOGIN_INFO;
		sendPkt.id = myPlayer->GetID();
		sendPkt.x = myPlayer->GetPos().x;
		sendPkt.y = myPlayer->GetPos().y;

		const Stat& stat = myPlayer->GetStat();
		sendPkt.hp = stat.hp;
		sendPkt.max_hp = stat.maxHp;
		sendPkt.exp = stat.exp;
		sendPkt.level = stat.level;

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

				if(obj == nullptr || obj->GetState() != ST_INGAME) continue;

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
						player->InsertViewList(myPlayer->GetID());

						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
						break;
					}
					case OBJECT_TYPE::MONSTER:
					{
						std::static_pointer_cast<Monster>(obj)->WakeUp();
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

	const Pos prevPos{ myPlayer->GetPos() };
	Pos nextPos{ prevPos };

	myPlayer->SetLastMoveTime(recvPkt.move_time);

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

				if(obj == nullptr || obj->GetState() != ST_INGAME) continue;

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
			sendPkt.move_time = myPlayer->GetLastMoveTime();

			auto sendBuffer = make_shared<SendBuffer>();
			sendBuffer->Append(sendPkt);
			session->RegistSend(std::move(sendBuffer));
		}

		for(const int objID : nearList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetState() != ST_INGAME) continue;

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
						sendPkt.move_time = myPlayer->GetLastMoveTime();

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
						player->InsertViewList(myPlayer->GetID());

						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
					}
					break;
				}
				case OBJECT_TYPE::MONSTER:
				{
					std::static_pointer_cast<Monster>(obj)->WakeUp();
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
				myPlayer->InsertViewList(objID);

				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				session->RegistSend(std::move(sendBuffer));
			}
		}

		for(const int objID : oldViewList) {
			if(nearList.find(objID) == nearList.end()) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetState() != ST_INGAME) continue;

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
					default:
						break;
				}
			}
		}


	}
}

