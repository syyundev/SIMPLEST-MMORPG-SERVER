#include "pch.h"
#include "Sector.h"

#include "SessionManager.h"

Sector::Sector(const int indexX, const int indexY)
	:mIndexX{indexX},mIndexY{indexY}
{
	static int id = 0;
	mID = ++id;
}

void Sector::Add(const int id)
{
	mSecLock.lock();
	if(mSessions.count(id) == 0)
		mSessions.insert(id);
	mSecLock.unlock();
}

void Sector::Remove(const int id)
{
	mSecLock.lock();
	if(mSessions.count(id) != 0)
		mSessions.erase(id);
	mSecLock.unlock();
}

shared_ptr<Session> Sector::FindSession(const int id)
{
	mSecLock.lock();
	if(mSessions.count(id)) {
		shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);
		mSecLock.unlock();
		return session;
	}
	mSecLock.unlock();
	return nullptr;
}
