#include "pch.h"
#include "ServerObjectManager.h"

#include "ServerObject.h"
#include "Board.h"
#include "Sector.h"
#include "SessionManager.h"
#include "Session.h"

void ServerObjectManager::AddServerObject(shared_ptr<ServerObject> gameObject)
{
	const int id = gameObject->GetID();

	m_serverObject.insert(make_pair(id, std::move(gameObject)));
}

shared_ptr<ServerObject> ServerObjectManager::GetGameObject(const int id)
{
	auto it = m_serverObject.find(id);
	if(it != m_serverObject.end())
		return it->second;   
	return nullptr;
}

void ServerObjectManager::RemoveServerObject(const int id)
{
	m_serverObject.at(id) = nullptr;
}
