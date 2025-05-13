#include "pch.h"
#include "ServerObjectManager.h"

#include "ServerObject.h"

void ServerObjectManager::AddServerObject(shared_ptr<ServerObject> gameObject)
{
	const uint64_t id = gameObject->GetID();

	if(m_serverObject.end() != m_serverObject.find(id))
		return;

	m_serverObject.insert(make_pair(id, std::move(gameObject)));
}

shared_ptr<ServerObject> ServerObjectManager::GetGameObject(const uint64_t id)
{
	if(m_serverObject.end() == m_serverObject.find(id))
		return nullptr;

	return m_serverObject[id];
}
