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
#include "Player.h"
#include "Session.h"

bool IOCPCore::Init()
{
	mIocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	if(mIocpHandle == INVALID_HANDLE_VALUE) [[unlikely]]
		return false;

		MANAGER(TaskQueue)->Init(mIocpHandle);

		return true;
}

void IOCPCore::ProcessIO()
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

				switch(const auto type = eventContext->type) {
					case EVENT_TYPE::HELLO:
					{
#ifdef AI_LUA
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
						if(obj == nullptr) {
							delete ioContext;
							continue;
						}

						if(obj->GetServerState() != SERVER_STATE::ST_INGAME) {
							delete ioContext;
							continue;
						}

						if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
							auto monster = std::static_pointer_cast<Monster>(obj);
							bool expected{ false };


							monster->m_ll.lock();
							auto L = monster->m_luaState;
							lua_getglobal(L, "event_player_move");
							lua_pushnumber(L, eventContext->ai_target_obj);
							lua_pcall(L, 1, 0, 0);
							monster->m_ll.unlock();
						}
						delete ioContext;
#endif
						break;
					}
					case EVENT_TYPE::MOVE:
					{
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
						if(obj == nullptr) {
							delete ioContext;
							continue;
						}

						if(obj->GetServerState() != SERVER_STATE::ST_INGAME) {
							delete ioContext;
							continue;
						}

						if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
							auto monster = std::static_pointer_cast<Monster>(obj);

							const Pos pos = monster->GetPos();

							bool keepAlive{ false };

							const auto sectorList = MANAGER(Board)->GetNeighborSectorList(pos);

							for(const int secID : sectorList) {
								auto sector = MANAGER(Board)->GetSector(secID);

								const auto objList = sector->GetObjList();

								for(const int objID : objList) {
									auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

									if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

									if(static_cast<OBJECT_TYPE>(obj->GetObjType()) != OBJECT_TYPE::PLAYER) continue;

									if(MANAGER(Board)->CanSee(monster->GetPos(), obj->GetPos())) {
										keepAlive = true;
										break;
									}
								}
							}

							if(keepAlive) {
								// TODO: LUA
							/*	monster->m_ll.lock();
								auto L = monster->m_luaState;
								lua_getglobal(L, "random_move");
								lua_pushnumber(L, monster->GetID());
								lua_pushnumber(L, eventContext->ai_target_obj);
								lua_pcall(L, 2, 0, 0);
								monster->AddMoveCount();
								monster->m_ll.unlock();*/
								monster->Move();
								MANAGER(TaskQueue)->AddTask(Task{ monster->GetID(), std::chrono::high_resolution_clock::now() + 1s, EVENT_TYPE::MOVE, -1 });
							}
							else {
								monster->SetActive(false);
							}
						}
						delete ioContext;
						break;
					}
					case EVENT_TYPE::ATTACK:
					{
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
						if(obj == nullptr) {
							delete ioContext;
							continue;
						}

						if(obj->GetServerState() != SERVER_STATE::ST_INGAME) {
							delete ioContext;
							continue;
						}

						if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
							auto monster = std::static_pointer_cast<Monster>(obj);
							monster->Attack(eventContext->ai_target_obj);
						}

						delete ioContext;
						break;
					}
					case EVENT_TYPE::HEAL:
					{
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);

						if(obj == nullptr || obj->GetServerState() != ST_INGAME) {
							delete ioContext;
							continue;
						}

						auto player = std::static_pointer_cast<Player>(obj);

						int hp = player->GetHP();
						if(hp < player->GetMaxHP()) {
							int healAmount = hp / 10;
							hp += healAmount;
							player->SetHP(hp);
							cout << std::format("{}번 플레이어 {}만큼 체력회복!", player->GetID(), healAmount) << endl;
						}
						else {
							MANAGER(TaskQueue)->AddTask(Task{ player->GetID(),std::chrono::high_resolution_clock::now() + 5s ,EVENT_TYPE::HEAL,0 });
							delete ioContext;
							continue;
						}

						SC_OBJECT_STATE_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_OBJECT_STATE;
						sendPkt.id = player->GetID();
						sendPkt.objType = player->GetObjType();
						sendPkt.hp = player->GetHP();
						sendPkt.maxHP = player->GetMaxHP();
						sendPkt.level = player->GetLevel();
						sendPkt.exp = player->GetExp();
						sendPkt.state = static_cast<unsigned char>(player->GetState());
						auto sendBuffer = make_shared<SendBuffer>();
						sendBuffer->Append(sendPkt);
						player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
						const Pos pos = player->GetPos();

						auto sectorList = MANAGER(Board)->GetNeighborSectorList(pos);

						for(const int secID : sectorList) {
							auto sector = MANAGER(Board)->GetSector(secID);

							auto objList = sector->GetObjList();

							for(const int objID : objList) {
								auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

								if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

								if(static_cast<OBJECT_TYPE>(obj->GetObjType()) != OBJECT_TYPE::PLAYER) continue;

								if(MANAGER(Board)->CanSee(player->GetPos(), obj->GetPos())) {
									auto sendBuffer = make_shared<SendBuffer>();
									sendBuffer->Append(sendPkt);
									std::static_pointer_cast<Player>(obj)->GetOwnerSession()->RegistSend(std::move(sendBuffer));
								}
							}
						}
						MANAGER(TaskQueue)->AddTask(Task{ player->GetID(),std::chrono::high_resolution_clock::now() + 5s ,EVENT_TYPE::HEAL,0 });
						delete ioContext;
						break;
					}
					case EVENT_TYPE::REVIVE:
					{
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);

						if(obj == nullptr)
							return;

						if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
							auto monster = std::static_pointer_cast<Monster>(obj);
							monster->Revive();
						}
						else if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::PLAYER) {
							auto player = std::static_pointer_cast<Player>(obj);
							player->Revive();
						}
						break;
					}
					case EVENT_TYPE::MONSTER_RANDOM_MOVE:
					{
						const int id = static_cast<int>(key);
						auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
						if(obj == nullptr) {
							delete ioContext;
							continue;
						}

						if(obj->GetServerState() != SERVER_STATE::ST_INGAME) {
							delete ioContext;
							continue;
						}

						if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
							auto monster = std::static_pointer_cast<Monster>(obj);

							const Pos pos = monster->GetPos();

							bool keepAlive{ false };

							const auto sectorList = MANAGER(Board)->GetNeighborSectorList(pos);

							for(const int secID : sectorList) {
								auto sector = MANAGER(Board)->GetSector(secID);

								const auto objList = sector->GetObjList();

								for(const int objID : objList) {
									auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

									if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

									if(static_cast<OBJECT_TYPE>(obj->GetObjType()) != OBJECT_TYPE::PLAYER) continue;

									if(MANAGER(Board)->CanSee(monster->GetPos(), obj->GetPos())) {
										keepAlive = true;
										break;
									}
								}
							}

							if(keepAlive) {
								// monster->Move();
								// TODO: LUA
								monster->m_ll.lock();
								auto L = monster->m_luaState;
								lua_getglobal(L, "random_move");
								lua_pushnumber(L, monster->GetID());
								lua_pushnumber(L, eventContext->ai_target_obj);
								lua_pcall(L, 2, 0, 0);
								monster->AddMoveCount();
								if(monster->GetMoveCount() < 4)
									MANAGER(TaskQueue)->AddTask(Task{ monster->GetID(), std::chrono::high_resolution_clock::now() + 1s, EVENT_TYPE::MONSTER_RANDOM_MOVE,eventContext->ai_target_obj });
								else {
									bool expected{ true };
									if(monster->m_flag.compare_exchange_strong(expected, false))
										monster->ResetMoveCount();
								}
								monster->m_ll.unlock();
							}
							else {
								monster->SetActive(false);
							}
						}
						delete ioContext;
						break;
					}
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
						return;
					}
					break;
				}
				default:
				{
					if(ioContext->contextType == IO_CONTEXT_TYPE::EVENT) {

						EventContext* eventContext = static_cast<EventContext*>(ioContext);

						switch(const auto type = eventContext->type) {
							case EVENT_TYPE::HELLO:
							{

								break;
							}
							case EVENT_TYPE::MOVE:
							{
								const int id = static_cast<int>(key);
								auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
								if(obj == nullptr) {
									delete ioContext;
									continue;
								}

								if(obj->GetServerState() != SERVER_STATE::ST_INGAME) {
									delete ioContext;
									continue;
								}

								if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {
									auto monster = std::static_pointer_cast<Monster>(obj);

									// TODO: 이부분 뭔가 이상함.

									// PEACE_FIX면 나온다.
									if(static_cast<MONSTER_TYPE>(monster->GetMonType()) == MONSTER_TYPE::PEACE_FIX) {
										cout << "나는 PEACE_FIX야" << endl;
										break;
									}

									const Pos pos = monster->GetPos();


									bool keepAlive{ false };

									auto sectorList = MANAGER(Board)->GetNeighborSectorList(pos);

									for(const int secID : sectorList) {
										auto sector = MANAGER(Board)->GetSector(secID);

										auto objList = sector->GetObjList();

										for(const int objID : objList) {
											auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

											if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

											if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) continue;

											if(MANAGER(Board)->CanSee(monster->GetPos(), obj->GetPos())) {
												keepAlive = true;
												break;
											}
										}
									}

									if(keepAlive) {
										monster->Move();
										MANAGER(TaskQueue)->AddTask(Task{ monster->GetID(), std::chrono::high_resolution_clock::now() + 1s, EVENT_TYPE::MOVE, -1 });
									}
									else {
										monster->SetActive(false);
									}
								}
								delete ioContext;
								break;
							}
							case EVENT_TYPE::ATTACK:
							{
								const int id = static_cast<int>(key);
								auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);
								if(obj == nullptr) {
									delete ioContext;
									continue;
								}

								if(obj->GetServerState() != SERVER_STATE::ST_INGAME) {
									delete ioContext;
									continue;
								}

								if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) {

								}
								else if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::PLAYER) {
									auto player = std::static_pointer_cast<Player>(obj);

									int hp = player->GetHP();
									hp -= 10;
									player->SetHP(hp);

									SC_OBJECT_STATE_PACKET sendPkt;
									sendPkt.size = sizeof(sendPkt);
									sendPkt.type = SC_OBJECT_STATE;
									sendPkt.id = player->GetID();
									sendPkt.objType = player->GetObjType();
									sendPkt.hp = player->GetHP();
									sendPkt.maxHP = player->GetMaxHP();
									sendPkt.level = player->GetLevel();
									sendPkt.exp = player->GetExp();
									sendPkt.state = static_cast<unsigned char>(player->GetState());
									auto sendBuffer = make_shared<SendBuffer>();
									sendBuffer->Append(sendPkt);
									player->GetOwnerSession()->RegistSend(std::move(sendBuffer));

									const Pos pos = player->GetPos();

									auto sectorList = MANAGER(Board)->GetNeighborSectorList(pos);

									for(const int secID : sectorList) {
										auto sector = MANAGER(Board)->GetSector(secID);

										auto objList = sector->GetObjList();

										for(const int objID : objList) {
											auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

											if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

											if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) continue;

											if(MANAGER(Board)->CanSee(player->GetPos(), obj->GetPos())) {
												auto sendBuffer = make_shared<SendBuffer>();
												sendBuffer->Append(sendPkt);
												std::static_pointer_cast<Player>(obj)->GetOwnerSession()->RegistSend(std::move(sendBuffer));
											}
										}
									}

								}
								delete ioContext;
								break;
							}
							case EVENT_TYPE::HEAL:
							{
								const int id = static_cast<int>(key);
								auto obj = MANAGER(ServerObjectManager)->GetGameObject(id);

								if(obj == nullptr || obj->GetServerState() != ST_INGAME) {
									delete ioContext;
									continue;
								}

								auto player = std::static_pointer_cast<Player>(obj);

								int hp = player->GetHP();
								if(hp < player->GetMaxHP()) {
									int healAmount = hp / 10;
									hp += healAmount;
									player->SetHP(hp);
									cout << std::format("{}번 플레이어 {}만큼 체력회복!", player->GetID(), healAmount) << endl;
								}
								else {
									MANAGER(TaskQueue)->AddTask(Task{ player->GetID(),std::chrono::high_resolution_clock::now() + 5s ,EVENT_TYPE::HEAL,0 });
									delete ioContext;
									continue;
								}

								SC_OBJECT_STATE_PACKET sendPkt;
								sendPkt.size = sizeof(sendPkt);
								sendPkt.type = SC_OBJECT_STATE;
								sendPkt.id = player->GetID();
								sendPkt.objType = player->GetObjType();
								sendPkt.hp = player->GetHP();
								sendPkt.maxHP = player->GetMaxHP();
								sendPkt.level = player->GetLevel();
								sendPkt.exp = player->GetExp();
								auto sendBuffer = make_shared<SendBuffer>();
								sendBuffer->Append(sendPkt);
								player->GetOwnerSession()->RegistSend(std::move(sendBuffer));
								const Pos pos = player->GetPos();

								auto sectorList = MANAGER(Board)->GetNeighborSectorList(pos);

								for(const int secID : sectorList) {
									auto sector = MANAGER(Board)->GetSector(secID);

									auto objList = sector->GetObjList();

									for(const int objID : objList) {
										auto obj = MANAGER(ServerObjectManager)->GetGameObject(objID);

										if(obj == nullptr || obj->GetServerState() != ST_INGAME) continue;

										if(static_cast<OBJECT_TYPE>(obj->GetObjType()) == OBJECT_TYPE::MONSTER) continue;

										if(MANAGER(Board)->CanSee(player->GetPos(), obj->GetPos())) {
											auto sendBuffer = make_shared<SendBuffer>();
											sendBuffer->Append(sendPkt);
											std::static_pointer_cast<Player>(obj)->GetOwnerSession()->RegistSend(std::move(sendBuffer));
										}
									}
								}
								MANAGER(TaskQueue)->AddTask(Task{ player->GetID(),std::chrono::high_resolution_clock::now() + 5s ,EVENT_TYPE::HEAL,0 });
								delete ioContext;
								break;
							}
							case EVENT_TYPE::REVIVE:
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
