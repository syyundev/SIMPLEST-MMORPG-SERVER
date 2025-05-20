#pragma once
#include "Singleton.hpp"

class ServerObject;

class ServerObjectManager : public Singleton<ServerObjectManager> {
	SINGLETON(ServerObjectManager)
private:
	concurrency::concurrent_unordered_map<int, std::atomic<shared_ptr<ServerObject>>> m_serverObject;

public:
	void						AddServerObject(shared_ptr<ServerObject> gameObject);
	shared_ptr<ServerObject>	GetGameObject(const int id);
	void						RemoveServerObject(const int id);
};

