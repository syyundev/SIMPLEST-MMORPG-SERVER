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
	MANAGER(Board)->MakeSectors();
	// TODO: 阁胶磐 积己 -> ServerObject啊 包府
	for(int i = 0; i < 200'000; ++i) {
		auto monster = make_shared<Monster>(MONSTER_TYPE::DEFAULT);
		monster->SetPos(Pos{ rand() % W_WIDTH, rand() % W_HEIGHT });
		MANAGER(Board)->GetSector(monster->GetPos())->Add(monster->GetID());
		monster->SetState(ST_INGAME);
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
	vector<thread> threads;

	for(int i = 0; i < MANAGER(ThreadPool)->GetMaxWorkerThreadCount(); ++i) {
		threads.emplace_back([this]() { Work(); });
	}
	std::jthread taskThread{ []()
{
	MANAGER(TaskQueue)->DoTask();
} };
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
	for(auto& t : threads)
		t.join();
}

void ServerManager::Destory()
{
	mListener = nullptr;

	MANAGER(SessionManager)->RemoveAllSessions();

	WSACleanup();
}

void ServerManager::Work()
{
	MANAGER(IOCPCore)->Process();
}
