#pragma once

#include "MovingObject.h"

class Session;

class Player : public MovingObject {
private:
	weak_ptr<Session>	m_ownerSession;
	int					m_attackPower;
public:
	Pos					m_startPos;

	mutex				m_pLock;
	shared_mutex		m_viewLock;
	unordered_set<int>	m_viewList;


public:
	Player();
	virtual ~Player();

public:
	void	SetStartPos(const Pos pos) noexcept{ m_startPos = pos; }
	
	Pos		GetStartPos() const noexcept { return m_startPos; }
	int		GetAttackPower() const noexcept { return m_attackPower; }
	void	InsertViewList(const int id);
	void	DeleteViewList(const int id);
	void	SetOwnerSession(const std::shared_ptr<Session> session) noexcept { m_ownerSession = session; }
	shared_ptr<Session> GetOwnerSession() const noexcept { return m_ownerSession.lock(); }
	
public:
	virtual void Attack(const int targetID) override;
	virtual void Revive() override;

};

