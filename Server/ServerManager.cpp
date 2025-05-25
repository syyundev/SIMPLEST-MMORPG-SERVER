#include "pch.h"
#include "ServerManager.h"

#include "IOCPCore.h"
#include "Listener.h"
#include "ThreadPool.h"
#include "SessionManager.h"
#include "Board.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "Monster.h"
#include "TaskQueue.h"
#include "Sector.h"

bool ServerManager::Init()
{
	MANAGER(SessionManager)->Init();
	MANAGER(Board)->Make();
	std::uniform_int_distribution<int> random{ 0, 3 };

	for(int i = 0; i < 1; ++i) {
		auto monster = make_shared<Monster>();
		// const Pos pos{ randomPos(dre), randomPos(dre) };
		const Pos pos{ 0, 0 };
		monster->SetPos(pos);
		monster->SetServerState(ST_INGAME);
		auto sector = MANAGER(Board)->GetSector(monster->GetPos());
		sector->Add(monster->GetID());
		MANAGER(ServerObjectManager)->AddServerObject(std::move(monster));
	}

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
	vector<thread> ioThreads;

	for(int i = 0; i < MANAGER(ThreadPool)->GetMaxWorkerThreadCount(); ++i) {
		ioThreads.emplace_back([]() { MANAGER(IOCPCore)->ProcessIO(); });
	}
	
	std::jthread taskThread{ []() { MANAGER(TaskQueue)->ProcessTask();} };

	string str;
	while(true) {
		cin >> str;

		if(str == "EXIT") {
			const HANDLE iocpHandle = MANAGER(IOCPCore)->GetHandle();
			for(int i = 0; i < MANAGER(ThreadPool)->GetMaxWorkerThreadCount(); ++i)
				if(0 == PostQueuedCompletionStatus(iocpHandle, 1, -1, nullptr)) {
					std::cout << "PQCS Failed" << std::endl;
				}
			MANAGER(TaskQueue)->SetFlag(false);
			break;
		}
	}
	
	for(auto& t : ioThreads)
		if(t.joinable())
			t.join();
}

void ServerManager::Destory()
{
	mListener = nullptr;

	MANAGER(SessionManager)->RemoveAllSessions();

	WSACleanup();
}
