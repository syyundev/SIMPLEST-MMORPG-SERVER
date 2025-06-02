#pragma once

#include "Singleton.hpp"

class DBManager : public Singleton<DBManager> {
	SINGLETON(DBManager)
private:
	SQLHENV		m_henv;
	SQLHDBC		m_hdbc;
	SQLHSTMT	m_hstmt = 0;
	mutex		m_mutex;

public:
	bool Connect();
	bool Disconnect();

public:
	bool SetUserInfo(const int id, const Pos pos);
	bool GetUserInfo(Pos& pos, const int id);
	bool AddUserInfo(const int id);

private:
	void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
};

