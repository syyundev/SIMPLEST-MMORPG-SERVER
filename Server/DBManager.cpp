#include "pch.h"
#include "DBManager.h"

bool DBManager::Connect()
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

				retCode = SQLConnect(m_hdbc, (SQLWCHAR*)L"ODBC_2021184022", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

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

bool DBManager::SetUserInfo(const int id, const Pos pos)
{
	//std::wstring str = L"EXEC set_user_info " + to_wstring(id) + L" " + to_wstring(pos.x) + L" " + to_wstring(pos.y);

	//lock_guard<mutex> lk{ m_mutex };
	//SQLRETURN retCode = SQLExecDirect(m_hstmt, str.data(), SQL_NTS);

	//if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
	//	// 실행만 했으니, 추가적인 바인딩/SQLExecute는 필요 없음
	//	SQLFreeStmt(m_hstmt, SQL_CLOSE);
	//	SQLFreeStmt(m_hstmt, SQL_UNBIND);
	//	return true;
	//}
	//else {
	//	HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
	//	return false;
	//}

	// 2) set_user_info 수행부(커서/바인딩 초기화 before)
	std::wstring upd = L"EXEC set_user_info " + to_wstring(id) + L"," + to_wstring(pos.x) + L"," + to_wstring(pos.y);
	lock_guard<mutex> lk2{ m_mutex };
	//// 이전 커서가 있을 경우 닫아두기
	//SQLFreeStmt(m_hstmt, SQL_CLOSE);
	//SQLFreeStmt(m_hstmt, SQL_UNBIND);

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

bool DBManager::GetUserInfo(Pos& pos, const int id)
{
	//std::wstring str = L"EXEC get_user_info " + to_wstring(id);	

	//lock_guard<mutex> lk{ m_mutex };
	//SQLRETURN retCode = SQLExecDirect(m_hstmt, str.data(), SQL_NTS);
	//if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
	//	int user_id;
	//	retCode = SQLBindCol(m_hstmt, 1, SQL_INTEGER, &user_id, sizeof(user_id), nullptr);
	//	HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
	//	retCode = SQLBindCol(m_hstmt, 2, SQL_SMALLINT, &pos.x, sizeof(short), nullptr);
	//	HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
	//	retCode = SQLBindCol(m_hstmt, 3, SQL_SMALLINT, &pos.y, sizeof(short), nullptr);
	//	HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);

	//	retCode = SQLFetch(m_hstmt);
	//	HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
	//	if(retCode == SQL_NO_DATA) {
	//		// 바인딩된 변수는 건드리지 않고, false 리턴
	//		SQLCloseCursor(m_hstmt);
	//		return false;
	//	}
	//	return true;
	//}
	//else {
	//	HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
	//	return false;
	//}'

	std::wstring str = L"EXEC get_user_info " + to_wstring(id);
	lock_guard<mutex> lk{ m_mutex };
	SQLRETURN retCode = SQLExecDirect(m_hstmt, str.data(), SQL_NTS);

	if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
		int user_id;
		SQLBindCol(m_hstmt, 1, SQL_INTEGER, &user_id, sizeof(user_id), nullptr);
		SQLBindCol(m_hstmt, 2, SQL_SMALLINT, &pos.x, sizeof(short), nullptr);
		SQLBindCol(m_hstmt, 3, SQL_SMALLINT, &pos.y, sizeof(short), nullptr);

		retCode = SQLFetch(m_hstmt);
		if(retCode == SQL_NO_DATA) {
			SQLCloseCursor(m_hstmt);
			return false;
		}
		// 데이터 읽은 뒤 커서 닫기
		SQLCloseCursor(m_hstmt);
		return true;
	}
	else {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
		return false;
	}
}

bool DBManager::AddUserInfo(const int id)
{
	Pos pos{ randomPos(dre), randomPos(dre) };

	std::wstring str = L"EXEC add_user_info " + to_wstring(id) + L", " + to_wstring(pos.x) + L", " + to_wstring(pos.y);
	lock_guard<mutex> lk{ m_mutex };
	SQLRETURN retCode = SQLExecDirect(m_hstmt, str.data(), SQL_NTS);

	if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
		// 파라미터 바인딩도 해제
		SQLFreeStmt(m_hstmt, SQL_CLOSE);
		SQLFreeStmt(m_hstmt, SQL_UNBIND);
		return true;
	}
	else {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, retCode);
		return false;
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
