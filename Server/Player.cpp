#include "pch.h"
#include "Player.h"

#include "TaskQueue.h"

Player::Player()
	:ServerObject(OBJECT_TYPE::PLAYER)
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
