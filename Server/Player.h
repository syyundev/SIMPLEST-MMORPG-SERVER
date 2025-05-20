#pragma once

#include "ServerObject.h"

class Session;

class Player : public ServerObject {
private:
	weak_ptr<Session>	m_ownerSession;

public:
	mutex				m_pLock;

	mutex				m_viewLock;
	unordered_set<int>	m_viewList;


public:
	Player();
	virtual ~Player();

public:
	void InsertViewList(const int id);
	void DeleteViewList(const int id);
	void SetOwnerSession(const std::shared_ptr<Session> session) noexcept { m_ownerSession = session; }
	shared_ptr<Session> GetOwnerSession() const noexcept { return m_ownerSession.lock(); }

};

