#include "pch.h"
#include "ServerManager.h"

#include "IOCPCore.h"
#include "Listener.h"
#include "ThreadPool.h"
#include "SessionManager.h"
#include "Board.h"

bool ServerManager::Init()
{
	MANAGER(SessionManager)->Init();

#ifdef ENABLE_SPACE_DEVISION
	MANAGER(Board)->MakeSectors();
#endif

	if(false == MANAGER(ThreadPool)->Init())
		return false;

	std::wcout.imbue(std::locale("korean"));

	WSADATA WSAData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &WSAData))
		return false;
	
	if(false == MANAGER(IOCPCore)->Init())
		return false;

	mListener = make_shared<Listener>();

	if(false == mListener->Init())
		return false;

	return true;
}

void ServerManager::ProcessIO()
{
	for(int i =0; i < MANAGER(ThreadPool)->GetMaxWorkerThreadCount(); ++i)
		MANAGER(ThreadPool)->EnqueueJob([this]() {
		Work();
	});
	
	string str;
	while(true) {
		cin >> str;
		
		if(str == "EXIT") {
			const HANDLE iocpHandle = MANAGER(IOCPCore)->GetHandle();
			for(int i = 0; i < MANAGER(ThreadPool)->GetMaxWorkerThreadCount(); ++i)
				if(0 == PostQueuedCompletionStatus(iocpHandle, 0, -1, nullptr)) {
					std::cout << "PQCS Failed" << std::endl;
				}
			break;
		}
	}
}

void ServerManager::Destory()
{
	mListener = nullptr;

	MANAGER(SessionManager)->RemoveAllSessions();
	
	MANAGER(ThreadPool)->Join();

	WSACleanup();
}

void ServerManager::Work()
{
	MANAGER(IOCPCore)->Process();
}
