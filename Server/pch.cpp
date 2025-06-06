#include "pch.h"

void print_error_message(int s_err)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, s_err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
	// exit(-1);
}
std::default_random_engine dre{ std::random_device{}() };
std::uniform_int_distribution<short> randomPos{ 0,W_WIDTH - 1 };
std::uniform_int_distribution<short> playerSpawnPos{ 0, 10 };