#pragma once

class ServerObject;

class Sector {
private:
	int mIndexX, mIndexY;
	int mID;

public:
	mutex			m_mutex;
	unordered_set<int>		m_serverObjectsList;

public:
	explicit Sector(const int indexX, const int indexY);

public:	
	void Add(const int id);
	void Remove(const int id);
	unordered_set<int> GetObjList()  noexcept;
	int GetID() const noexcept { return mID; }
};

