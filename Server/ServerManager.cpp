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
#include "DBManager.h"

bool ServerManager::Init()
{
	if(false == MANAGER(DBManager)->Connect(L"2021184022_TP_ODBC"))
		return false;

	MANAGER(SessionManager)->Init();
	MANAGER(Board)->Make();
	std::uniform_int_distribution<int> random{ 0, 3 };

#ifdef MAX_MONSTER	
	constexpr int monsterCount = MAX_NPC;
#else
	constexpr int monsterCount = 1;
#endif
	cout << monsterCount << "개의 몬스터 생성 시작..." << endl;

	for(int i = 0; i < monsterCount; ++i) {
		auto monster = make_shared<Monster>();
		Pos pos{1,1};
		//while(true) {
		//	pos = Pos{randomPos(dre), randomPos(dre) };
		//	
		//	if(MANAGER(Board)->CanGo(pos))
		//		break;
		//}
		monster->SetPos(pos);
		monster->SetServerState(ST_INGAME);
		auto sector = MANAGER(Board)->GetSector(monster->GetPos());
		sector->Add(monster->GetID());
		MANAGER(ServerObjectManager)->AddServerObject(std::move(monster));
	}
	cout << monsterCount << "개의 몬스터 생성 완료!" << endl;

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

	std::cout << "Server Start!" << std::endl;

	return true;
}

void ServerManager::ProcessIO()
{
	vector<thread> ioThreads;

	for(int i = 0; i < MANAGER(ThreadPool)->GetMaxWorkerThreadCount(); ++i)
		ioThreads.emplace_back([]() { MANAGER(IOCPCore)->ProcessIO(); });
	
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

	MANAGER(DBManager)->Disconnect();

	WSACleanup();
}
