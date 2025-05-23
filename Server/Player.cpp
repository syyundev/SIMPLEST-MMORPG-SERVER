#include "pch.h"
#include "Player.h"

#include "TaskQueue.h"

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

void Player::Revive()
{
}
