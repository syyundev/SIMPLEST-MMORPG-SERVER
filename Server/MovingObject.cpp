#include "pch.h"
#include "MovingObject.h"
#include "Player.h"
#include "Monster.h"
#include "TaskQueue.h"
#include "Board.h"
#include "Sector.h"
#include "ServerObjectManager.h"
#include "Session.h"

MovingObject::MovingObject(const OBJECT_TYPE type)
	:ServerObject(type), m_lastMoveTime{ 0 }, m_dir(DIRECTION_TYPE::LEFT), m_lastAttackTime{ 0 }, m_alive(true)
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

	SetState(MOVING_OBJECT_STATE::DEAD);

	switch(auto type = static_cast<OBJECT_TYPE>(GetObjType())) {
		case OBJECT_TYPE::PLAYER:
		{
			// TODO: Player 부활
			auto player = std::static_pointer_cast<Player>(shared_from_this());
			
			bool expected{ true };
			if(false == m_alive.compare_exchange_strong(expected, false))
				return;

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

			cout << std::format("{}번 플레이어 사망!\n", GetID());
			MANAGER(TaskQueue)->AddTask(Task{ GetID(), std::chrono::high_resolution_clock::now() + 5s, EVENT_TYPE::REVIVE, 0 });

			//player->SetPos(player->GetStartPos());
			//m_stat.hp = m_stat.maxHp;
			//m_stat.exp.store(m_stat.exp / 2);
			break;
		}
		case OBJECT_TYPE::MONSTER:
		{
			// TODO: 몬스터 부활
			// 몬스터 체력이 0이되면 몬스터는 죽는다
			// 몬스터의 상태는 죽은 상태가 되어야 한다
			// 몬스터가 죽고 나면, 몇 초뒤 부활 할 수 있는 이벤트를 넣어줘야한다.

			// 만약, 쓰러진 상태면 꺠우지 말아야한다.
			//	쓰러진 위치에서 부활
			//  채력 만땅, hp = maxHP;
			cout << std::format("{}번 몬스터 사망!\n", GetID());
			bool expected{ true };
			if(false == m_alive.compare_exchange_strong(expected, false))
				return;

			SC_OBJECT_STATE_PACKET sendPkt;
			sendPkt.size = sizeof(sendPkt);
			sendPkt.type = SC_OBJECT_STATE;
			sendPkt.id = GetID();
			sendPkt.hp = GetHP();
			sendPkt.maxHP = GetMaxHP();
			sendPkt.exp = GetExp();
			sendPkt.level = GetLevel();

			// 주변 애들에게 정보 보내주기
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

			MANAGER(TaskQueue)->AddTask(Task{ GetID(), std::chrono::high_resolution_clock::now() + 5s, EVENT_TYPE::REVIVE, 0 });
			break;
		}
		default:
			break;
	}
}
