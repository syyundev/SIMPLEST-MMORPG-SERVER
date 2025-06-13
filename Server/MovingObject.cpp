#include "pch.h"
#include "MovingObject.h"
#include "Player.h"
#include "Monster.h"
#include "TaskQueue.h"
#include "Board.h"
#include "Sector.h"
#include "ServerObjectManager.h"
#include "Session.h"
#include "Item.h"

MovingObject::MovingObject(const OBJECT_TYPE type)
	:ServerObject(type), m_lastMoveTime{ 0 }, m_dir(DIRECTION_TYPE::LEFT), m_lastAttackTime{ 0 }, m_alive{ true }, m_attackDamage{10}
{
	m_stat.hp = 100;
	m_stat.maxHp = 100;
	m_stat.exp = 0;
	m_stat.level = 1;
}

MovingObject::~MovingObject()
{
}

void MovingObject::SetHP(const int hp)
{
	m_stat.hp = hp;
}

void MovingObject::SetExp(const int exp) noexcept
{
}

void MovingObject::AddExp(const int amount) noexcept
{
	m_stat.exp += amount;

	while(m_stat.exp >= m_stat.level * 100) {
		m_stat.exp -= m_stat.level * 100;
		m_stat.level++;
		cout << std::format("{}번 플레이어 레벨 {}로 레벨업!", GetID(), m_stat.level.load()).c_str() << endl;
	}
}

void MovingObject::SubHP(const int amount) noexcept
{
	if(m_alive == false)
		return;

	m_stat.hp.fetch_sub(amount);

	if(m_stat.hp > 0) {
		return;
	}

	switch(auto type = static_cast<OBJECT_TYPE>(GetObjType())) {
		case OBJECT_TYPE::PLAYER:
		{
			auto player = std::static_pointer_cast<Player>(shared_from_this());
			
			bool expected{ true };
			if(false == m_alive.compare_exchange_strong(expected, false))
				return;

			SetState(MOVING_OBJECT_STATE::DEAD);
			SC_OBJECT_STATE_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_OBJECT_STATE;
			sendPkt.id = GetID();
			sendPkt.hp = GetHP();
			sendPkt.maxHP = GetMaxHP();
			sendPkt.exp = GetExp();
			sendPkt.level = GetLevel();
			sendPkt.state = static_cast<unsigned char>(GetState());

			{
				auto sendBuffer = make_shared<SendBuffer>();
				sendBuffer->Append(sendPkt);
				auto myPlayer = std::static_pointer_cast<Player>(MANAGER(ServerObjectManager)->GetGameObject(GetID()));
				if(nullptr != myPlayer)
					myPlayer->GetOwnerSession()->RegistSend(std::move(sendBuffer));
			}
			
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

			cout << std::format("{}번 플레이어 사망!\n", GetID());
			MANAGER(TaskQueue)->AddTask(Task{ GetID(), std::chrono::high_resolution_clock::now() + PLAYER_RESPAWN_TIME, EVENT_TYPE::REVIVE, 0 });
			break;
		}
		case OBJECT_TYPE::MONSTER:
		{
			// 아이템 생성 
			constexpr int itemProb{ 100 };
			static std::uniform_int_distribution<int> uid{ 0,99 };

			const int randomValue{ uid(dre) };

			const Pos monsterPos{ GetPos() };

			if(randomValue < itemProb) {
				auto item = make_shared<Item>(ITEM_TYPE::POTION);
				item->SetPos(monsterPos);
				item->SetServerState(SERVER_STATE::ST_INGAME);

				SC_ADD_OBJECT_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_ADD_OBJECT;
				sendPkt.id = item->GetID();
				sendPkt.x = item->GetPos().x;
				sendPkt.y = item->GetPos().y;
				memcpy(sendPkt.name, item->GetName().data(), item->GetName().size());
				sendPkt.objType = item->GetObjType();
				sendPkt.detail = static_cast<unsigned char>(ITEM_TYPE::POTION);
				MANAGER(Board)->GetSector(item->GetPos())->Add(item->GetID());
				MANAGER(ServerObjectManager)->AddServerObject(std::move(item));

				auto neighborSecList = MANAGER(Board)->GetNeighborSectorList(monsterPos);

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

			cout << std::format("{}번 몬스터 사망!\n", GetID());
			bool expected{ true };
			if(false == m_alive.compare_exchange_strong(expected, false))
				return;

			SetState(MOVING_OBJECT_STATE::DEAD);
			
			SC_OBJECT_STATE_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_OBJECT_STATE;
			sendPkt.id = GetID();
			sendPkt.hp = GetHP();
			sendPkt.maxHP = GetMaxHP();
			sendPkt.exp = GetExp();
			sendPkt.level = GetLevel();	
			sendPkt.objType = GetObjType();
			sendPkt.state = static_cast<unsigned char>(GetState());
				
			// 주변 애들에게 정보 보내주기
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

			MANAGER(TaskQueue)->AddTask(Task{ GetID(), std::chrono::high_resolution_clock::now() + MONSTER_RESPAWN_TIME, EVENT_TYPE::REVIVE, 0 });
			break;
		}
		default:
			break;
	}
}
