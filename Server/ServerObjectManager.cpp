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

shared_ptr<ServerObject> ServerObjectManager::GetGameObject(const string_view name)
{
	auto it = std::find_if(m_serverObject.cbegin(), m_serverObject.cend(), [&name](const auto& kv){         // 원자적으로 포인터를 읽어 온다
		std::shared_ptr<ServerObject> ptr = kv.second.load(std::memory_order_acquire);
		return ptr && ptr->GetName() == name;
		});

	if(it != m_serverObject.cend())
		return it->second;

	return nullptr;
}

void ServerObjectManager::RemoveServerObject(const int id)
{
	m_serverObject.at(id) = nullptr;
	lock_guard<mutex> lk{ m_mutex };
	m_serverObject.unsafe_erase(id);
}

void ServerObjectManager::Broadcast()
{

}
