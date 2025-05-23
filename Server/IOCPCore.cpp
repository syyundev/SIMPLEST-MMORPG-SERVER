#include "pch.h"
#include "IOCPCore.h"

#include "IOContext.h"
#include "IOCPRegistrable.h"
#include "ServerObjectManager.h"
#include "TaskQueue.h"
#include "ServerObject.h"
#include "Monster.h"
#include "Board.h"
#include "Sector.h"
#include "ServerObjectManager.h"

bool IOCPCore::Init()
{
	mIocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	if(mIocpHandle == INVALID_HANDLE_VALUE) [[unlikely]]
		return false;

		MANAGER(TaskQueue)->Init(mIocpHandle);

		return true;
}

void IOCPCore::Process()
{
	while(true) {
		DWORD numOfBytes = 0;
		ULONG_PTR key = 0;
		IOContext* ioContext = nullptr;

		if(::GetQueuedCompletionStatus(mIocpHandle, OUT & numOfBytes, OUT & key, OUT reinterpret_cast<LPOVERLAPPED*>(&ioContext), INFINITE)) {
			if(key == -1) {
				break;
			}
			if(ioContext->contextType == IO_CONTEXT_TYPE::EVENT) {

				EventContext* eventContext = static_cast<EventContext*>(ioContext);

				switch(auto type = eventContext->type) {
					case TASK_TYPE::PLAYER_UPDATE:
					{
						break;
					}
					case TASK_TYPE::MONSTER_MOVE:
					{
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
						if(obj == nullptr) {
							delete ioContext;
							continue;
						}	

						if(obj->GetSeverState() != S_STATE::ST_INGAME) {
							delete ioContext;
							continue;
						}

						if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
							auto monster = std::static_pointer_cast<Monster>(obj);

							const Pos pos = monster->GetPos();

							bool keepAlive{ false };

							auto sectorList = MANAGER(Board)->GetNeighborSectorList(pos);

							for(const int secID : sectorList) {
								auto sector = MANAGER(Board)->GetSector(secID);

								auto objList = sector->GetObjList();

								for(const int objID : objList) {
									auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

									if(obj == nullptr || obj->GetSeverState() != ST_INGAME) continue;

									if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) continue;

									if(MANAGER(Board)->CanSee(monster->GetPos(), obj->GetPos())) {
										keepAlive = true;
										break;
									}
								}
							}

							if(keepAlive) {
								monster->Move();
								MANAGER(TaskQueue)->AddTask(Task{ monster->GetID(), std::chrono::high_resolution_clock::now() + 1s, TASK_TYPE::MONSTER_MOVE, -1 });
							}
							else {
								monster->SetActive(false);
							}
						}
						delete ioContext;
						break;
					}
					case TASK_TYPE::MONSTER_REVIVE:
					{
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);

						if(obj == nullptr)
							return;

						if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
							auto monster = std::static_pointer_cast<Monster>(obj);
							monster->Revive();
						}
						break;
					}
					break;
					default:
						break;
				}
			}
			else {
				shared_ptr<IOCPRegistrable> iocpObject = ioContext->owner;
				if(ioContext->owner == nullptr)
					continue;

				iocpObject->ProcessIOCompletion(ioContext, numOfBytes);
			}
		}
		else {
			int errCode = ::WSAGetLastError();
			switch(errCode) {
				case WAIT_TIMEOUT:
				{
					if(key == -1) {
						int a = 0;
						break;
					}
					return;
				}
				default:
				{
					if(ioContext->contextType == IO_CONTEXT_TYPE::EVENT) {
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
						if(obj == nullptr)
							continue;

						if(obj->GetSeverState() != S_STATE::ST_INGAME)
							continue;

						/*if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
							auto monster = std::static_pointer_cast<Monster>(obj);
							monster->Move();
							MANAGER(TaskQueue)->m_lk.lock();
							bool expected{ true };
							monster->m_isActive.compare_exchange_strong(expected, false);
							MANAGER(TaskQueue)->AddTask(Task{ obj->GetID(), std::chrono::high_resolution_clock::now() + 1s, TASK_TYPE::MONSTER_UPDATE, 0 });
							MANAGER(TaskQueue)->m_lk.unlock();
						}*/
						delete ioContext;
					}
					else {
						shared_ptr<IOCPRegistrable> iocpObject = ioContext->owner;
						iocpObject->ProcessIOCompletion(ioContext, numOfBytes);
					}
					break;
				}
			}
		}
	}
}

void IOCPCore::Destory()
{
	::CloseHandle(mIocpHandle);
}

bool IOCPCore::Regist(const shared_ptr<IOCPRegistrable>& object)
{
	return CreateIoCompletionPort(object->GetHandle(), mIocpHandle, 0, 0);
}

bool IOCPCore::Regist(const SOCKET socket)
{
	return CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), mIocpHandle, 0, 0);
}
