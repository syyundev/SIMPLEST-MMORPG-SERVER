#include "pch.h"
#include "Player.h"

#include	"TaskQueue.h"
#include	"Board.h"
#include	"Sector.h"
#include "ServerObjectManager.h"
#include "Session.h"
#include "Monster.h"

Player::Player()
	:MovingObject(OBJECT_TYPE::PLAYER), m_attackPower{10}
{
	static atomic<int> playerID{1};
	SetID(playerID);
	playerID++;
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

void Player::Test()
{

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
	
	int monsterHP = monster->GetHP();
	if(monsterHP <= 0) {
		const int gaiendExp = GainExp();

		cout << std::format("{}번 플레이어가 몬스터 {}번을 무찔러서 {}만큼의 경험치를 획득했습니다!", GetID(), monster->GetID(), gaiendExp) << endl;

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

	if(m_alive.compare_exchange_strong(expected, true)) {
		SetHP(GetMaxHP());
		int exp = GetExp();
		exp /= 2;
		SetExp(exp);
		SetPos(m_startPos);
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

		auto sendBuffer = make_shared<SendBuffer>();
		sendBuffer->Append(sendPkt);
		GetOwnerSession()->RegistSend(sendBuffer);

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
