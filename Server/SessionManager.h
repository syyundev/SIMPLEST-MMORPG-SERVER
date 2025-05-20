#pragma once
#include "Singleton.hpp"

class Session;
class SendBuffer;

class SessionManager : public Singleton<SessionManager> {
	SINGLETON(SessionManager)
private:
	static constexpr int													MAX_SESSION_COUNT = 30000;
	std::atomic_int															m_currentSessionCount = 0;
	concurrency::concurrent_unordered_map<int, atomic<shared_ptr<Session>>> m_sessions;

public:
	void Init();
	void AddSession(shared_ptr<Session> session);
	void RemoveSession(int id);
	shared_ptr<Session> GetSession(const int id);
	const auto& GetSessions() const noexcept { return m_sessions; }
	int GetCurrentSessionCount() const noexcept { return m_currentSessionCount; }
	void RemoveAllSessions();
	
	void Broadcast(shared_ptr<SendBuffer> sendBuffer);
};

