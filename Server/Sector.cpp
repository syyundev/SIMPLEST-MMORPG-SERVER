#include "pch.h"
#include "Sector.h"

#include "ServerObject.h"
#include "ServerObjectManager.h"

Sector::Sector(const int indexX, const int indexY)
	:mIndexX{indexX}, mIndexY{indexY}	
{
	static int id = 0;
	mID = ++id;
}

void Sector::Add(const int id)
{
	lock_guard<mutex> lk{ m_mutex };
	m_serverObjectsList.insert(id);
}

void Sector::Remove(const int id)
{
	lock_guard<mutex> lk{ m_mutex };
	if(m_serverObjectsList.find(id) != m_serverObjectsList.end())
		m_serverObjectsList.erase(id);
}

unordered_set<int> Sector::GetObjList()	   noexcept
{
	lock_guard<mutex> lk{ m_mutex };
	return m_serverObjectsList;
}
