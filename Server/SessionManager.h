#pragma once
#include "Singleton.hpp"

class Session;

class SessionManager : public Singleton<SessionManager> {
	SINGLETON(SessionManager)
public:
	static constexpr int														MAX_SESSION_COUNT = 20000;
	std::atomic_int																m_currentSessionCount = 0;

	concurrency::concurrent_unordered_map<uint64_t, atomic<shared_ptr<Session>>> m_sessions;

private:

public:
	void Init();
	void AddSession(shared_ptr<Session> session);
	void RemoveSession(uint64_t id);
	shared_ptr<Session> GetSession(const uint64_t id);
	const auto& GetSessions() const noexcept { return m_sessions; }
	void RemoveAllSessions();

public:
	template<typename PacketType>
	void Broadcast(PacketType&& pkt)
	{
		for(auto& [id, p] : m_sessions) {
			shared_ptr<Session> session = p;
			if(session != nullptr && session->mServerState == ST_INGAME)
				session->RegistSend(std::forward<PacketType>(pkt));
		}
	}

	int GetCurrentSessionCount() const noexcept { return m_currentSessionCount; }
};

