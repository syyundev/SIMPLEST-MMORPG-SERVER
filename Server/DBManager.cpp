#include "pch.h"
#include "DBManager.h"
#include "Player.h"
#include "ServerObjectManager.h"

bool DBManager::Connect(const wstring_view odbcName)
{
	setlocale(LC_ALL, "korean");
	std::wcout.imbue(std::locale("korean"));

	SQLRETURN retCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);

	if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
		retCode = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
		if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
			retCode = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);
			if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(m_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				retCode = SQLConnect(m_hdbc, (SQLWCHAR*)odbcName.data(), SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
					retCode = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);

					std::cout << "SUCCESS\n";
				}
			}
		}
	}

	return true;
}

bool DBManager::Disconnect()
{
	SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
	SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
	SQLDisconnect(m_hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
	return true;
}

bool DBManager::SetUserInfo(const int id)
{
	auto player = std::static_pointer_cast<Player>(MANAGER(ServerObjectManager)->GetGameObject(id));

	if(player == nullptr)
		return false;

	const Pos pos{ player->GetPos() };
	const wstring name{ player->GetName().begin(), player->GetName().end() };
	const int exp{ player->GetExp() };
	const int level{ player->GetLevel() };
	const int hp{ player->GetHP() };


	std::wstring upd = L"EXEC set_user_info " + to_wstring(id) + L"," + name + L", " + to_wstring(pos.x) + L"," + to_wstring(pos.y) + L", " + to_wstring(exp) + L", " + 
		to_wstring(level) + L", " + to_wstring(hp);

	lock_guard<mutex> lk2{ m_mutex };

	SQLRETURN rc2 = SQLExecDirect(m_hstmt, upd.data(), SQL_NTS);
	if(rc2 == SQL_SUCCESS || rc2 == SQL_SUCCESS_WITH_INFO) {
		// 파라미터 바인딩도 해제
		SQLFreeStmt(m_hstmt, SQL_CLOSE);
		SQLFreeStmt(m_hstmt, SQL_UNBIND);
		return true;
	}
	else {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, rc2);
		return false;
	}
}

std::shared_ptr<Player> DBManager::GetUserInfo(const int id)
{
	std::wstring str = L"EXEC get_user_info " + to_wstring(id);
	lock_guard<mutex> lk{ m_mutex };
	SQLRETURN retCode = SQLExecDirect(m_hstmt, str.data(), SQL_NTS);

	if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {

		auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
		if(obj != nullptr) return nullptr;

		auto player = make_shared<Player>();

		int user_id;
		Pos pos;
		string name;
		name.resize(NAME_SIZE);
		int exp;
		int level;
		int hp;

		SQLBindCol(m_hstmt, 1, SQL_INTEGER, &user_id, sizeof(user_id), nullptr);
		SQLBindCol(m_hstmt, 2, SQL_CHAR, name.data(), NAME_SIZE, nullptr);
		SQLBindCol(m_hstmt, 3, SQL_SMALLINT, &pos.x, sizeof(short), nullptr);
		SQLBindCol(m_hstmt, 4, SQL_SMALLINT, &pos.y, sizeof(short), nullptr);
		SQLBindCol(m_hstmt, 5, SQL_INTEGER, &exp, sizeof(int), nullptr);
		SQLBindCol(m_hstmt, 6, SQL_INTEGER, &level, sizeof(int), nullptr);
		SQLBindCol(m_hstmt, 7, SQL_INTEGER, &hp, sizeof(int), nullptr);

		retCode = SQLFetch(m_hstmt);

		player->SetID(user_id);
		player->SetPos(pos);
		player->SetStartPos(pos);
		player->SetName(name);
		player->SetExp(exp);
		player->SetLevel(level);
		player->SetHP(hp);

		if(retCode == SQL_NO_DATA) {
			SQLCloseCursor(m_hstmt);
			return nullptr;	
		}
		// 데이터 읽은 뒤 커서 닫기
		SQLCloseCursor(m_hstmt);
		return player;
	}
	else {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
		return nullptr;
	}

	return nullptr;
}

std::shared_ptr<Player> DBManager::AddUserInfo(const int id, const std::string_view name)
{
	// 기본 게임 데이터들은 데이터 sheet에서 파싱해서 가져온다.
	Pos pos{ randomPos(dre), randomPos(dre) };
	wstring nameStr{ name.begin(), name.end() };
	int exp{};
	int level{1};
	int hp{100};

	std::wstring str = L"EXEC add_user_info " + to_wstring(id) + L", " + nameStr + L", " +  to_wstring(pos.x) + L", " + to_wstring(pos.y) + L", " + to_wstring(exp) + L", " + to_wstring(level) + L", " + to_wstring(hp);
	lock_guard<mutex> lk{ m_mutex };
	SQLRETURN retCode = SQLExecDirect(m_hstmt, str.data(), SQL_NTS);

	if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
		// 파라미터 바인딩도 해제

		auto player = make_shared<Player>();

		player->SetID(id);
		player->SetPos(pos);
		player->SetStartPos(pos);
		player->SetName(name);
		player->SetExp(exp);
		player->SetLevel(level);
		player->SetHP(hp);

		SQLFreeStmt(m_hstmt, SQL_CLOSE);
		SQLFreeStmt(m_hstmt, SQL_UNBIND);
		return player;
	}
	else {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
		return nullptr;
	}
}

void DBManager::HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if(RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while(SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if(wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}
