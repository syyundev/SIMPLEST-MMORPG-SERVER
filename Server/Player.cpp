#include "pch.h"
#include "Player.h"

#include "TaskQueue.h"
#include "Board.h"
#include "Sector.h"
#include "ServerObjectManager.h"
#include "Session.h"
#include "Monster.h"

Player::Player()
	:MovingObject(OBJECT_TYPE::PLAYER), m_attackPower{10}
{
	static atomic<int> playerID{1};
	SetID(playerID);
	playerID++;
	SetHP(100);
}

Player::~Player()
{
}

void Player::InsertViewList(const int id)
{
	lock_guard<mutex> lk{ m_viewLock };
	m_viewList.insert(id);
}

void Player::DeleteViewList(const int id)
{
	lock_guard<mutex> lk{ m_viewLock };
	if(m_viewList.find(id) != m_viewList.end())
		m_viewList.erase(id);
}

void Player::Attack(const int targetID)
{
	auto monster = std::static_pointer_cast<Monster>(MANAGER(ServerObjectManager)->GetGameObject(targetID));
	const Pos monsterPos = monster->GetPos();

	if(monster == nullptr)
		return;

	if(false == monster->IsAlive()|| false == monster->IsActive())
		return;

	const int attackDamage = 10;
	cout << std::format("{}번 플레이어가 {}번 몬스터에게 {}만큼 데미지를 가했습니다!", GetID(), monster->GetID(), attackDamage).c_str() << endl;
	monster->SubHP(attackDamage);
	monster->SetTarget(std::static_pointer_cast<Player>(shared_from_this()));
	
	int monsterHP = monster->GetHP();
	if(monsterHP <= 0) {
		int gainedEXP{};
		const int level = GetLevel();
		switch(auto type = static_cast<MONSTER_TYPE>(monster->GetMonType())) {
			case MONSTER_TYPE::PEACE_FIX:
				gainedEXP = level * level * 2;
				break;
			case MONSTER_TYPE::PEACE_ROAMING:
				gainedEXP = level * level * 2 * 2;
				break;
			case MONSTER_TYPE::AGRO_FIX:
				gainedEXP = level * level * 2 * 2;
				break;
			case MONSTER_TYPE::AGRO_ROAMING:
				gainedEXP = level * level * 2 * 4;
				break;
			default:
				break;
		}

		AddExp(gainedEXP);
	
		cout << std::format("{}번 플레이어가 몬스터 {}번을 무찔러서 {}만큼의 경험치를 획득했습니다!", GetID(), monster->GetID(), gainedEXP) << endl;

		SC_OBJECT_STATE_PACKET sendPkt;
		sendPkt.size = sizeof(sendPkt);
		sendPkt.type = SC_OBJECT_STATE;
		sendPkt.id =GetID();
		sendPkt.objType = GetObjType();
		sendPkt.hp = GetHP();
		sendPkt.maxHP =GetMaxHP();
		sendPkt.level = GetLevel();
		sendPkt.exp = GetExp();
		auto sendBuffer = make_shared<SendBuffer>();
		sendBuffer->Append(sendPkt);
		m_ownerSession.lock()->RegistSend(std::move(sendBuffer));
	}

	SC_OBJECT_STATE_PACKET sendPkt;
	sendPkt.size = sizeof(sendPkt);
	sendPkt.type = SC_OBJECT_STATE;
	sendPkt.id = monster->GetID();
	sendPkt.objType = monster->GetObjType();
	sendPkt.hp = monster->GetHP();
	sendPkt.maxHP = monster->GetMaxHP();
	sendPkt.level = monster->GetLevel();
	sendPkt.exp = monster->GetExp();
	auto sendBuffer = make_shared<SendBuffer>();
	sendBuffer->Append(sendPkt);
	m_ownerSession.lock()->RegistSend(std::move(sendBuffer));

	{
		auto sectorList = MANAGER(Board)->GetNeighborSectorList(monsterPos);
		for(const int secID : sectorList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();
			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

				if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) continue;

				if(obj->GetID() == GetID()) continue;

				if(MANAGER(Board)->CanSee(monster->GetPos(), obj->GetPos())) {
					auto sendBuffer = make_shared<SendBuffer>();
					sendBuffer->Append(sendPkt);
					std::static_pointer_cast<Player>(obj)->GetOwnerSession()->RegistSend(std::move(sendBuffer));
				}
			}
		}

	}
}

void Player::Revive()
{
	bool expected{ false };


	// TODO: 나에게 ADD_OBJECT_PACKET과 SC_OBJECT_STATE_PACKET 같이 보내줘야 함.

	if(m_alive.compare_exchange_strong(expected, true)) {
		int exp = GetExp();
		exp /= 2;
		SetExp(exp);
		SetHP(GetMaxHP());

		SetPos(m_startPos);
		SetState(MOVING_OBJECT_STATE::IDLE);
		unordered_set<int> nearList;
		m_viewLock.lock();
		auto oldViewList = m_viewList;
		m_viewLock.unlock();

		auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(GetPos());

		for(const int secID : neighborSecList) {
			auto sector = MANAGER(Board)->GetSector(secID);

			auto objList = sector->GetObjList();

			for(const int objID : objList) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

				if(obj->GetID() == GetID()) continue;

				if(MANAGER(Board)->CanSee(GetPos(), obj->GetPos()))
					nearList.insert(obj->GetID());
			}
		}

		// 나에게 상태패킷 보내주기
		{
			SC_OBJECT_STATE_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_OBJECT_STATE;
			sendPkt.id = GetID();
			sendPkt.objType = GetObjType();
			sendPkt.hp = GetHP();
			sendPkt.maxHP = GetMaxHP();
			sendPkt.exp = GetExp();
			sendPkt.level = GetLevel();
			auto sendBuffer = make_shared<SendBuffer>();
			sendBuffer->Append(sendPkt);
			GetOwnerSession()->RegistSend(std::move(sendBuffer));
		}

		//// 나에게 이동좌표 보내주기
		{
			SC_MOVE_OBJECT_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_MOVE_OBJECT;
			sendPkt.id = GetID();
			sendPkt.x = GetPos().x;
			sendPkt.y = GetPos().y;
			sendPkt.move_time = static_cast<int>(GetLastMoveTime());
			sendPkt.dir = GetDir();

			auto sendBuffer = make_shared<SendBuffer>();
			sendBuffer->Append(sendPkt);
			GetOwnerSession()->RegistSend(std::move(sendBuffer));
		}

		for(const int objID : nearList) {
			auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

			if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

			if(obj->GetID() == GetID()) continue;

			switch(auto type = static_cast<OBJECT_TYPE>(obj->GetObjType())) {
				case OBJECT_TYPE::PLAYER:
				{
					auto player = std::static_pointer_cast<Player>(obj);

					player->m_viewLock.lock();
					if(player->m_viewList.end() != player->m_viewList.find(GetID())) {
						player->m_viewLock.unlock();


						SC_MOVE_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_MOVE_OBJECT;
						sendPkt.id = GetID();
						sendPkt.x = GetPos().x;
						sendPkt.y = GetPos().y;
						sendPkt.move_time = static_cast<int>(GetLastMoveTime());
						sendPkt.dir = GetDir();

						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
					}
					else {
						player->m_viewLock.unlock();

						SC_ADD_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_ADD_OBJECT;
						sendPkt.id = GetID();
						sendPkt.x = GetPos().x;
						sendPkt.y = GetPos().y;
						memcpy(sendPkt.name, GetName().data(), GetName().size());
						sendPkt.name[GetName().size()] = 0;
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
				InsertViewList(objID);

				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				GetOwnerSession()->RegistSend(std::move(sendBuffer));
			}
		}

		for(const int objID : oldViewList) {
			if(nearList.find(objID) == nearList.end()) {
				auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

				if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

				DeleteViewList(objID);

				SC_REMOVE_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_REMOVE_OBJECT;
				sendPkt.id = objID;
				sendPkt.objType = obj->GetObjType();

				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				GetOwnerSession()->RegistSend(std::move(sendBuffer));

				switch(auto type = static_cast<OBJECT_TYPE>(obj->GetObjType())) {
					case OBJECT_TYPE::PLAYER:
					{
						auto player = std::static_pointer_cast<Player>(obj);

						player->DeleteViewList(GetID());

						SC_REMOVE_OBJECT_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_REMOVE_OBJECT;
						sendPkt.id = GetID();
						sendPkt.objType = GetObjType();

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

		//MANAGER(Board)->GetSector(m_startPos)->Add(GetID());

		//// TOOD: 이동 전 위치에서 SC_REMOVE_OBJECT 날려줘야함.
		//
		//unordered_set<int> oldViewList;


		// 
		//
		//// TODO: 이동 후 SC_ADD_OBJECT_PACKET 날려줘야함.

		//SC_OBJECT_STATE_PACKET sendPkt;
		//sendPkt.size = sizeof(sendPkt);
		//sendPkt.type = SC_OBJECT_STATE;
		//sendPkt.id = GetID();
		//sendPkt.hp = GetHP();
		//sendPkt.maxHP = GetMaxHP();
		//sendPkt.exp = GetExp();
		//sendPkt.level = GetLevel();

		//auto sendBuffer = make_shared<SendBuffer>();
		//sendBuffer->Append(sendPkt);
		//GetOwnerSession()->RegistSend(sendBuffer);

		//auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(GetPos());

		//for(const int secID : neighborSecList) {
		//	auto sector = MANAGER(Board)->GetSector(secID);

		//	auto objList = sector->GetObjList();

		//	for(const int objID : objList) {
		//		auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

		//		if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

		//		if(OBJECT_TYPE::MONSTER == static_cast<OBJECT_TYPE>(obj->GetObjType())) continue;

		//		if(MANAGER(Board)->CanSee(GetPos(), obj->GetPos())) {
		//			auto player = std::static_pointer_cast<Player>(obj);

		//			auto sendBuffer = make_shared<SendBuffer>();
		//			sendBuffer->Append(sendPkt);
		//			player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
		//		}
		//	}
		//}
	}
}
