#pragma once
#include "Singleton.hpp"

class ServerObject;

class ServerObjectManager : public Singleton<ServerObjectManager> {
	SINGLETON(ServerObjectManager)
private:
	unordered_map<uint64_t, shared_ptr<ServerObject>> m_serverObject;

public:
	void AddServerObject(shared_ptr<ServerObject> gameObject);
	shared_ptr<ServerObject> GetGameObject(const uint64_t id);
};

