#pragma once

class Session;

class Sector {
private:
	int mIndexX, mIndexY;
	int mID;

public:
	mutex mSecLock;
	//concurrency::concurrent_unordered_set<int> mSessions;
	//concurrency::concurrent_unordered_set<int> mSessions;
	unordered_set<int> mSessions;

public:
	explicit Sector(const int indexX, const int indexY);

public:	
	void lock() { mSecLock.lock(); }
	void unlock() { mSecLock.unlock(); }
	void Add(const int id);
	void Remove(const int id);
	shared_ptr<Session> FindSession(const int id);
	int GetID() const noexcept { return mID; }
};

