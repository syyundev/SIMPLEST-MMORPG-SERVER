#include "pch.h"
#include "SessionManager.h"

#include "Session.h"
#include "ServerObjectManager.h"

void SessionManager::Init()
{

}

void SessionManager::AddSession(shared_ptr<Session> session)
{
	const int id = session->GetID();

	if(m_sessions.find(session->GetID()) == m_sessions.end()) {
		m_sessions.insert(make_pair(id, std::move(session)));
		m_currentSessionCount++;
	}
}

void SessionManager::RemoveSession(int id)
{
	if(m_sessions.find(id) != m_sessions.end())
		m_sessions.at(id) = nullptr;

	m_currentSessionCount--;
}

shared_ptr<Session> SessionManager::GetSession(const int id)
{
	if(m_sessions.find(id) == m_sessions.end())
		return nullptr;

	return m_sessions.at(id);
}

void SessionManager::RemoveAllSessions()
{
	m_sessions.clear();
	m_currentSessionCount = 0;
}

void SessionManager::Broadcast(shared_ptr<SendBuffer> sendBuffer)
{
	for(auto& [id, p] : m_sessions) {
		shared_ptr<Session> session = p;
		if(session != nullptr && session->mServerState == ST_INGAME)
			session->RegistSend(sendBuffer);
	}
}

