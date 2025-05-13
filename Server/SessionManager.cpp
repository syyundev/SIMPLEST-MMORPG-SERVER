#include "pch.h"
#include "SessionManager.h"

#include "Session.h"

void SessionManager::Init()
{

}

void SessionManager::AddSession(shared_ptr<Session> session)
{
	shared_ptr<Session> p = session;
	const uint64_t id = p->GetID();
	{
		if(m_sessions.count(id) == 0) {
			m_sessions.insert(make_pair(id, p));
			m_currentSessionCount++;
		}
	}
}

void SessionManager::RemoveSession(uint64_t id)
{ 
	if(m_sessions.count(id) != 0)
		m_sessions.at(id) = nullptr;

	m_currentSessionCount--;
}	

shared_ptr<Session> SessionManager::GetSession(const uint64_t id)
{
	if(m_sessions.count(id) == 0)
		return nullptr;

	return m_sessions.at(id);
}

void SessionManager::RemoveAllSessions()
{
	m_sessions.clear();
	m_currentSessionCount = 0;
}

