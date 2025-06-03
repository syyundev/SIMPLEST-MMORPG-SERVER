#pragma once

#include "Singleton.hpp"
class Player;

class DBManager : public Singleton<DBManager> {
	SINGLETON(DBManager)
private:
	SQLHENV		m_henv;
	SQLHDBC		m_hdbc;
	SQLHSTMT	m_hstmt = 0;
	mutex		m_mutex;

public:
	bool Connect(const wstring_view odbcName);
	bool Disconnect();

public:
	bool SetUserInfo(const int id);
	std::shared_ptr<Player> GetUserInfo(const int id);
	std::shared_ptr<Player> AddUserInfo(const int id, const std::string_view name);

private:
	void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
};

