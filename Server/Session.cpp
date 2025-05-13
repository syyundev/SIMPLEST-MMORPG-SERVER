#include "pch.h"
#include "Session.h"

#include "SessionManager.h"
#include "Packet.h"
#include "Board.h"
#include "IOContext.h"
#include "SendBuffer.h"
#include "Board.h"
#include "Sector.h"

//void CALLBACK G_RECV_CALLBACK(DWORD err, DWORD numBytes, LPWSAOVERLAPPED pOverapped, DWORD flag)
//{
//	auto myID = reinterpret_cast<unsigned long long>(pOverapped->hEvent);
//	MANAGER(SessionManager)->GetSession(myID)->OnPostRecv(err, numBytes, pOverapped, flag);
//}
//
//void CALLBACK G_SEND_CALLBACK(DWORD err, DWORD numBytes, LPWSAOVERLAPPED pOverlapped, DWORD flags)
//{
//	IOContext* sendContext = reinterpret_cast<IOContext*>(pOverlapped);
//	delete sendContext;
//}

Session::Session(const SOCKET socket)
	:m_socket(socket), m_recvBuffer(65535), mConnected{ false }, mSendRegistred{ false }, mX{ 0 }, mY{ 0 }, mServerState{ ST_FREE }, mLastMoveTime{ 0 }
{
	static uint64_t idGen = 1;
	m_id = idGen;
	idGen++;
}

Session::~Session()
{
	std::cout << "~Session, ID: " << m_id << std::endl;
	closesocket(m_socket);
}

void Session::ProcessIOCompletion(IOContext* ioContext, const DWORD numOfBytes)
{
	switch(const IO_CONTEXT_TYPE type = ioContext->contextType) {
		case IO_CONTEXT_TYPE::RECV:
		{
			OnPostRecv(numOfBytes);
			break;
		}
		case IO_CONTEXT_TYPE::SEND:
		{
			OnPostSend(numOfBytes);
			break;
		}
		case IO_CONTEXT_TYPE::DISCONNECT:
		{
			PostDisconnect();
			break;
		}
		default:
			break;
	}
}

//void Session::OnPostRecv(DWORD err, DWORD numBytes, LPWSAOVERLAPPED pOverlapped, DWORD flag)
//{
//	if(0 == numBytes) {
//		Disconnect();
//		return;
//	}
//
//	// 데이터는 이미 RecvBuffer에 들어와 있음.
//	// WriteCursor만 옮긴다.
//	if(false == m_recvBuffer.MoveWriteCursor(numBytes)) {
//		Disconnect();
//		return;
//	}
//
//	// 현재 버퍼에 있는 데이터 크기 
//	const int recvBufferDataSize = m_recvBuffer.GetDataSize();
//
//	// 읽어야 할 데이터의 시작 위치 
//	const char* const readPos = m_recvBuffer.GetReadPos();
//
//	int processLen = 0;
//
//	while(true) {
//		int dataSize = recvBufferDataSize - processLen;
//		
//		const char* const packetData = &readPos[processLen];
//
//		const PacketHeader* const packetHeader = reinterpret_cast<const PacketHeader* const>(packetData);
//
//		const unsigned short packetSize = packetHeader->size;
//		
//		if(packetHeader->type < 0)
//			break;
//
//		if(packetSize == 0)
//			break;
//
//		if(packetSize < sizeof(PacketHeader))
//			break;
//			
//		if(dataSize < packetSize)
//			break;
//
//		switch(const PACKET_TYPE packetType = static_cast<PACKET_TYPE>(packetHeader->type)) {
//			case PACKET_TYPE::C2S_MOVE:
//			{
//				C2S_MOVE_PACKET recvPkt;
//				memcpy(&recvPkt, packetData, packetSize);
//
//				const uint64_t sessionID = recvPkt.id;
//				
//				const auto& session  = MANAGER(SessionManager)->GetSession(sessionID);
//
//				const Vec2Int curPos = session->GetPos();
//				Vec2Int nextPos{};
//				switch(const auto keyInput = recvPkt.dir) {
//					case KEY_INPUT::UP:
//					{
//						nextPos = Vec2Int{ curPos.x, curPos.y - 1 };
//						break;
//					}
//					case KEY_INPUT::DOWN:
//					{
//						nextPos = Vec2Int{ curPos.x, curPos.y+ 1 };
//						break;
//					}
//					case KEY_INPUT::LEFT:
//					{
//						nextPos = Vec2Int{ curPos.x - 1, curPos.y };
//						break;
//					}
//					case KEY_INPUT::RIGHT:
//					{
//						nextPos = Vec2Int{ curPos.x + 1, curPos.y };
//						break;
//					}
//					default:
//						break;
//				}
//	
//				session->SetPos(nextPos);
//
//				S2C_MOVE_PACKET sendPkt;
//				sendPkt.id = sessionID;
//				sendPkt.x = nextPos.x;
//				sendPkt.y = nextPos.y;
//
//				MANAGER(SessionManager)->Broadcast(sendPkt);
//
//				//if(MANAGER(Board)->CanGo(nextPos)) {
//			
//				//}
//				break;
//			}
//			break;
//		}
//
//		processLen += packetSize;
//	}
//	
//	// 데이터 처리 한 만큼 ReadCursor 옮기기
//	if(false == m_recvBuffer.MoveReadCursor(processLen)) {
//		Disconnect();
//		return;
//	}
//
//	m_recvBuffer.Clean();
//	
//	PostRecv();
//}

void Session::OnPostRecv(const DWORD numBytes)
{
	mRecvContext.owner = nullptr;

	if(numBytes == 0) {
		PostDisconnect();
		return;
	}

	if(m_recvBuffer.MoveWriteCursor(numBytes) == false) {
		PostDisconnect();
		return;
	}

	int dataSize = m_recvBuffer.GetDataSize();
	int processLen = ProcessData(m_recvBuffer.GetReadPos(), dataSize); // 컨텐츠 코드에서 재정의
	if(processLen < 0 || dataSize < processLen || m_recvBuffer.MoveReadCursor(processLen) == false) {
		PostDisconnect();
		return;
	}

	m_recvBuffer.Clean();

	PostRecv();
}

void Session::OnPostSend(const DWORD numBytes)
{
	mSendContext.owner = nullptr;
	mSendContext.sendBuffers.clear();

	if(numBytes == 0) {
		PostDisconnect();
	}

	if(mSendQueue.empty())
		mSendRegistred.store(false);
	else
		PostSend();
}

void Session::ProcessConnect()
{
	mConnected = true;
	mServerState = ST_ALLOC;

	std::cout << std::format("Session ID:{}, Server Connected!", m_id) << std::endl;

	PostRecv();
}

void Session::PostDisconnect()
{
#ifdef ENABLE_VIEW_PROCESSING
#ifdef ENABLE_SPACE_DEVISION	// 시야 처리 O, 공간 분할 O
	mVM.lock();
	unordered_set<int> vl = mViewList;
	mVM.unlock();

	for(int id : vl) {
		shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);

		if(session == nullptr)
			continue;

		if(session->mServerState != ST_INGAME)
			continue;

		if(id == m_id)
			continue;

		SC_REMOVE_PLAYER_PACKET sendPkt;
		sendPkt.size = sizeof(sendPkt);
		sendPkt.type = SC_REMOVE_PLAYER;
		sendPkt.id = m_id;
		session->RegistSend(sendPkt);
	}

	//{
	//	pair<int, int> checkX{ mX - VIEW_RANGE, mX + VIEW_RANGE };
	//	pair<int, int> checkY{ mY - VIEW_RANGE, mY + VIEW_RANGE };

	//	for(int y = checkY.first; y <= checkY.second; ++y) {
	//		for(int x = checkX.first; x <= checkX.second; ++x) {
	//			if(x < 0 || y < 0
	//				|| x >= Board::BOARD_WIDTH
	//				|| y >= Board::BOARD_HEIGHT) continue;

	//			auto sec = MANAGER(Board)->GetSector(x, y);

	//			if(nullptr == sec) {
	//				continue;
	//			}

	//			sec->lock();
	//			auto others = sec->mSessions;
	//			sec->unlock();

	//			for(int oid : others) {
	//				if(oid == m_id) continue;
	//				auto other = MANAGER(SessionManager)->GetSession(oid);
	//				if(!other || other->mServerState != ST_INGAME) continue;

	//				other->mVM.lock();
	//				if(other->mViewList.erase(m_id) > 0) {
	//					// remove 통지
	//					SC_REMOVE_PLAYER_PACKET pkt;
	//					pkt.size = sizeof(pkt);
	//					pkt.type = SC_REMOVE_PLAYER;
	//					pkt.id = m_id;
	//					other->RegistSend(pkt);
	//				}
	//				other->mVM.unlock();
	//			}
	//		}
	//	}
	//}
	shared_ptr<Sector> sector = MANAGER(Board)->GetSector(mX, mY);
	sector->Remove(m_id);
	MANAGER(SessionManager)->RemoveSession(m_id);
#else	// 시야처리 O, 공간 분할 X.
	mVM.lock();
	unordered_set<int> vl = mViewList;
	mVM.unlock();

	for(int id : vl) {
		shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);

		if(session == nullptr)
			continue;

		if(session->mServerState != ST_INGAME)
			continue;

		if(id == m_id)
			continue;

		SC_REMOVE_PLAYER_PACKET sendPkt;
		sendPkt.size = sizeof(sendPkt);
		sendPkt.type = SC_REMOVE_PLAYER;
		sendPkt.id = m_id;
		session->RegistSend(sendPkt);
	}
	MANAGER(SessionManager)->RemoveSession(m_id);
#endif
#else  // 시야처리 X, 공간분할 X , TODO: 여기서 공간분할 한 버전 안 한 버전 나눠야 함.
	SC_REMOVE_PLAYER_PACKET sendPkt;
	sendPkt.size = sizeof(sendPkt);
	sendPkt.type = SC_REMOVE_PLAYER;
	sendPkt.id = m_id;

	const auto& sessions = MANAGER(SessionManager)->GetSessions();

	for(const auto& [id, p] : sessions) {
		shared_ptr<Session> session = p;

		if(session == nullptr)
			continue;

		if(id == m_id)
			continue;

		if(session != nullptr && session->mServerState == ST_INGAME)
			session->RegistSend(sendPkt);
	}

	MANAGER(SessionManager)->RemoveSession(m_id);
#endif
}

int Session::ProcessData(const char* const buffer, const int len)
{
	int processLen = 0;

	while(true) {
		int dataSize = len - processLen;

		if(dataSize < sizeof(PacketHeader))
			break;

		const PacketHeader header = *(reinterpret_cast<const PacketHeader*>(&buffer[processLen]));

		if(dataSize < header.size)
			break;

		ProcessPacket(&buffer[processLen], header.size);

		processLen += header.size;
	}

	return processLen;

}

void Session::ProcessPacket(const char* const buffer, const int packetSize)
{
	const PacketHeader header = *(reinterpret_cast<const PacketHeader*>(buffer));
	// TODO: header 한 번 더 체크!

	switch(header.type) {
		case CS_LOGIN:
		{
			const CS_LOGIN_PACKET recvPkt = *(reinterpret_cast<const CS_LOGIN_PACKET*>(buffer));

			// 로그인 정보 세션에 저장
			strcpy_s(mName, strlen(recvPkt.name) + 1, recvPkt.name);

			static int k{ 39 }, w{ 39 };
			mX = rand() % W_WIDTH;
			mY = rand() % W_HEIGHT;

			//mX = k++;
			//mY = w++;

			mServerState = ST_INGAME;

#ifdef ENABLE_SPACE_DEVISION
			// Sector에 넣어주기.
			MANAGER(Board)->GetSector(mX, mY)->Add(m_id);
#endif // ENABLE_SECTOR_DEVISION

			// 나에게 로그인 정보 보내주기
			{
				SC_LOGIN_INFO_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_LOGIN_INFO;
				sendPkt.id = static_cast<short>(m_id);
				sendPkt.x = mX;
				sendPkt.y = mY;
				RegistSend(sendPkt);
			}

#ifdef ENABLE_VIEW_PROCESSING
#ifdef ENABLE_SPACE_DEVISION	// 시야처리 O, 공간분할 O
			// NEW
			// 현재 플레이어가 위치한 섹터 + 인접섹터 섹터리스트들을 모두 들고온다.
			unordered_set<int> neighborSec = MANAGER(Board)->GetNeighborSector(mX, mY);

			for(int secID : neighborSec) {
				shared_ptr<Sector> sec = MANAGER(Board)->GetSector(secID);

				// Sector안에 있는 플레이어들을 갖고온다.
				sec->lock();
				unordered_set<int> sessions = sec->mSessions;
				sec->unlock();

				for(int id : sessions) {
					if(m_id == id)
						continue;

					shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);

					if(session == nullptr)
						continue;

					if(session->mServerState != ST_INGAME)
						continue;

					// 같은 섹터에서도 시야각 안에 있는 애들만
					if(CanSee(session)) {
						// 내 시야각 안에 있는 상대방이고, 현재 내가 속해있는 섹터에 있는 세션이면
						// 상대방에게 나의 정보를 보내준다.
						SC_ADD_PLAYER_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_ADD_PLAYER;
						sendPkt.id = m_id;
						sendPkt.x = mX;
						sendPkt.y = mY;
						strcpy_s(sendPkt.name, strlen(mName) + 1, mName);

						session		->mVM.lock();
						if(session->mViewList.count(m_id) == 0)
							session->mViewList.insert(m_id);
						session->mVM.unlock();

						session->RegistSend(sendPkt);
					}

					// 나에게 상대방 정보 보내주기.
					{
						SC_ADD_PLAYER_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_ADD_PLAYER;
						sendPkt.id = id;
						sendPkt.x = session->mX;
						sendPkt.y = session->mY;
						strcpy_s(sendPkt.name, strlen(session->mName) + 1, session->mName);

						mVM.lock();
						if(mViewList.count(id) == 0)
							mViewList.insert(id);
						mVM.unlock();

						RegistSend(sendPkt);
					}
				}

			}
#else // 시야처리 O, 공간 분할 X
			const auto& sessions = MANAGER(SessionManager)->GetSessions();

			for(auto& [id, s] : sessions) {

				shared_ptr<Session> session = s;

				if(m_id == id || session == nullptr || session->mServerState != ST_INGAME)
					continue;

				if(CanSee(session)) {
					SC_ADD_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_PLAYER;
					sendPkt.id = m_id;
					sendPkt.x = mX;
					sendPkt.y = mY;
					strcpy_s(sendPkt.name, strlen(mName) + 1, mName);

					session->mVM.lock();
					if(session->mViewList.count(m_id) == 0)
						session->mViewList.insert(m_id);
					session->mVM.unlock();

					session->RegistSend(sendPkt);

					{

						SC_ADD_PLAYER_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_ADD_PLAYER;
						sendPkt.id = id;
						sendPkt.x = session->mX;
						sendPkt.y = session->mY;
						strcpy_s(sendPkt.name, strlen(session->mName) + 1, session->mName);

						mVM.lock();
						if(mViewList.count(id) == 0)
							mViewList.insert(id);
						mVM.unlock();

						RegistSend(sendPkt);
					}
				}
			}

#endif
#else // 시야처리 X, 공간분할 X , TODO: 여기서 공간분할 한 버전 안 한 버전 나눠야 함.
			const auto& sessions = MANAGER(SessionManager)->GetSessions();

			for(auto& [id, session] : sessions) {
				if(id == m_id)
					continue;

				shared_ptr<Session> s = session;

				// 상대방에게 내 정보 전달
				if(s != nullptr && s->mServerState == ST_INGAME) {
					SC_ADD_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_PLAYER;
					sendPkt.id = static_cast<short>(m_id);
					sendPkt.x = mX;
					sendPkt.y = mY;
					strcpy_s(sendPkt.name, strlen(mName) + 1, mName);
					s->RegistSend(sendPkt);
				}
			}

			for(auto& [id, session] : sessions) {
				if(id == m_id)
					continue;

				shared_ptr<Session> s = session;

				// 나에게 상대방 정보 전달
				if(s != nullptr && s->mServerState == ST_INGAME) {
					SC_ADD_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_PLAYER;
					sendPkt.id = static_cast<short>(s->m_id);
					sendPkt.x = s->mX;
					sendPkt.y = s->mY;
					strcpy_s(sendPkt.name, strlen(s->mName) + 1, s->mName);
					RegistSend(sendPkt);
				}
			}
#endif
			break;
		}
		case CS_MOVE:
		{
			const CS_MOVE_PACKET recvPkt = *(reinterpret_cast<const CS_MOVE_PACKET*>(buffer));

			mLastMoveTime = recvPkt.move_time;

			short prevMX = mX;
			short prevMY = mY;

			short destX = mX;
			short destY = mY;

			switch(recvPkt.direction) {
				case MOVE_UP:
					destY--;
					break;
				case MOVE_DOWN:
					destY++;
					break;
				case MOVE_LEFT:
					destX--;
					break;
				case MOVE_RIGHT:
					destX++;
					break;
			}

			if(MANAGER(Board)->CanGo(Vec2Int{ destX, destY })) {
				// 좌표 이동
				mX = destX;
				mY = destY;
			}

			if(prevMX == mX && prevMY == mY)
				return;

#ifdef ENABLE_VIEW_PROCESSING
#ifdef ENABLE_SPACE_DEVISION	// 시야처리 O, 공간분할 O

			// 이동 전 섹터
			shared_ptr<Sector> oldSector = MANAGER(Board)->GetSector(prevMX, prevMY);

			// 이동후 섹터
			shared_ptr<Sector> sector = MANAGER(Board)->GetSector(destX, destY);

			if(oldSector == nullptr || sector == nullptr)
				return;

			// 이동 전 섹터에서 제거
			// 이동 후 섹터에 추가
			if(oldSector != sector) {
				oldSector->Remove(m_id);
				sector->Add(m_id);
			}

			// 현재 내 인접섹터에 있는 애들을 모은다.
			// 
			unordered_set<int> neighborSec = MANAGER(Board)->GetNeighborSector(mX, mY);

			unordered_set<int> nearList;

			// 인접섹터에 있고, 볼 수 있으면 nearList에 다 넣논다.
			for(int secID : neighborSec) {
				auto sec = MANAGER(Board)->GetSector(secID);
				sec->lock();
				auto others = sec->mSessions;
				sec->unlock();
				// Sector안에 오브젝트들 탐색하면서 내 시야각 안에 있는 오브젝트면 near_list에 삽입
				for(int id : others) {
					if(m_id == id)
						continue;

					shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);

					if(session == nullptr || session->mServerState != ST_INGAME)
						continue;

					// 내 시야에 있으면 시야각에 넣어준다.
					if(CanSee(session)) {
						if(nearList.count(id) == 0)
							nearList.insert(id);
					}
				}
			}

			mVM.lock();
			unordered_set<int> oldViewList = mViewList;
			mVM.unlock();

			//// 나에게 이동했다는 정보 보내줌
			{
				SC_MOVE_PLAYER_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_MOVE_PLAYER;
				sendPkt.id = m_id;
				sendPkt.x = mX;
				sendPkt.y = mY;
				sendPkt.move_time = mLastMoveTime;
				RegistSend(sendPkt);
			}

			for(int id : nearList) {
				shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);

				if(m_id == id)
					continue;
				
				if(session == nullptr)
					continue;

				if(session->mServerState != ST_INGAME)
					continue;

				session->mVM.lock();
				if(session->mViewList.count(m_id)) {
					session->mVM.unlock();
					SC_MOVE_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_MOVE_PLAYER;
					sendPkt.id = m_id;
					sendPkt.x = mX;
					sendPkt.y = mY;
					sendPkt.move_time = mLastMoveTime;
					session->RegistSend(sendPkt);
				}
				else {
					session->mVM.unlock();
					SC_ADD_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_PLAYER;
					sendPkt.id = m_id;
					sendPkt.x = mX;
					sendPkt.y = mY;
					strcpy_s(sendPkt.name, strlen(mName) + 1, mName);
					session->mVM.lock();
					if(session->mViewList.count(m_id) == 0)
						session->mViewList.insert(m_id);
					session->mVM.unlock();

					session->RegistSend(sendPkt);
				}

				if(oldViewList.count(id) == 0) {
					SC_ADD_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_PLAYER;
					sendPkt.id = id;
					sendPkt.x = session->mX;
					sendPkt.y = session->mY;
					strcpy_s(sendPkt.name, strlen(session->mName) + 1, session->mName);

					mVM.lock();
					if(mViewList.count(id) == 0)
						mViewList.insert(id);
					mVM.unlock();

					RegistSend(sendPkt);
				}
			}

			for(int id : oldViewList) {
				shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);
				if(m_id == id)
					continue;

				if(session == nullptr)
					continue;

				if(session->mServerState != ST_INGAME)
					continue;

				if(0 == nearList.count(id)) {
					{
						mVM.lock();
						if(mViewList.count(id) != 0)
							mViewList.erase(id);
						mVM.unlock();
						SC_REMOVE_PLAYER_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_REMOVE_PLAYER;
						sendPkt.id = id;
						RegistSend(sendPkt);
					}

					{
						session->mVM.lock();
						if(session->mViewList.count(m_id) != 0)
							session->mViewList.erase(m_id);
						session->mVM.unlock();
						SC_REMOVE_PLAYER_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_REMOVE_PLAYER;
						sendPkt.id = m_id;
						session->RegistSend(sendPkt);
					}
			
				}
			}

#else		// 시야처리 O, 공간분할 X

			mX = destX;
			mY = destY;

			unordered_set<int> nearList;
			const auto& sessions = MANAGER(SessionManager)->GetSessions();

			for(auto& [id, s] : sessions) {
				shared_ptr<Session> session = s;

				if(m_id == id || session == nullptr || session->mServerState != ST_INGAME)
					continue;

				if(CanSee(session)) {
					if(nearList.count(id) == 0)
						nearList.insert(id);
				}
			}

			mVM.lock();
			unordered_set<int> oldViewList = mViewList;
			mVM.unlock();

			{
				SC_MOVE_PLAYER_PACKET sendPkt;
				sendPkt.size = sizeof(sendPkt);
				sendPkt.type = SC_MOVE_PLAYER;
				sendPkt.id = m_id;
				sendPkt.x = mX;
				sendPkt.y = mY;
				sendPkt.move_time = mLastMoveTime;
				RegistSend(sendPkt);
			}

			for(int id : nearList) {
				shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);

				//if(m_id == id || session == nullptr || session->mServerState != ST_INGAME)
				//	continue;

				if(session == nullptr)
					continue;

				session->mVM.lock();
				if(session->mViewList.count(m_id)) {
					session->mVM.unlock();
					SC_MOVE_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_MOVE_PLAYER;
					sendPkt.id = m_id;
					sendPkt.x = mX;
					sendPkt.y = mY;
					sendPkt.move_time = mLastMoveTime;
					session->RegistSend(sendPkt);
				}
				else {
					session->mVM.unlock();
					SC_ADD_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_PLAYER;
					sendPkt.id = m_id;
					sendPkt.x = mX;
					sendPkt.y = mY;
					strcpy_s(sendPkt.name, strlen(mName) + 1, mName);
					session->mVM.lock();
					if(session->mViewList.count(m_id) == 0)
						session->mViewList.insert(m_id);
					session->mVM.unlock();
					session->RegistSend(sendPkt);
				}

				if(oldViewList.count(id) == 0) {
					SC_ADD_PLAYER_PACKET sendPkt;
					sendPkt.size = sizeof(sendPkt);
					sendPkt.type = SC_ADD_PLAYER;
					sendPkt.id = id;
					sendPkt.x = session->mX;
					sendPkt.y = session->mY;
					strcpy_s(sendPkt.name, strlen(session->mName) + 1, session->mName);
					mVM.lock();
					if(mViewList.count(id) == 0)
						mViewList.insert(id);
					mVM.unlock();
					RegistSend(sendPkt);
				}
			}

			for(int id : oldViewList) {
				if(nearList.count(id) == 0) {
					shared_ptr<Session> session = MANAGER(SessionManager)->GetSession(id);

					//if(m_id == id || session == nullptr || session->mServerState != ST_INGAME)
					//	continue;

					if(session == nullptr)
						continue;

					{
						mVM.lock();
						if(mViewList.count(id)) {
							mViewList.erase(id);
						}
						else {
							mVM.unlock();
							return;
						}
						mVM.unlock();

						SC_REMOVE_PLAYER_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_REMOVE_PLAYER;
						sendPkt.id = id;

						RegistSend(sendPkt);
					}
					{
						session->mVM.lock();
						if(session->mViewList.count(m_id))
							session->mViewList.erase(m_id);
						else {
							session->mVM.unlock();
							return;
						}
						session->mVM.unlock();

						SC_REMOVE_PLAYER_PACKET sendPkt;
						sendPkt.size = sizeof(sendPkt);
						sendPkt.type = SC_REMOVE_PLAYER;
						sendPkt.id = m_id;
						session->RegistSend(sendPkt);
					}
				}
			}
#endif
#else	// 시야처리 X, 공간분할 X, TODO: 여기서 공간분할 한 버전 안 한 버전 나눠야 함.
			{
				SC_MOVE_PLAYER_PACKET sendPkt;
				sendPkt.size = sizeof(SC_MOVE_PLAYER_PACKET);
				sendPkt.type = SC_MOVE_PLAYER;
				sendPkt.id = m_id;
				sendPkt.x = mX;
				sendPkt.y = mY;
				sendPkt.move_time = mLastMoveTime;
				MANAGER(SessionManager)->Broadcast(sendPkt);
			}
#endif
			break;
		}
		default:
			break;
	}
}

bool Session::CanSee(const shared_ptr<Session>& session)
{
	if(abs(mX - session->mX) > VIEW_RANGE) return false;
	return abs(mY - session->mY) <= VIEW_RANGE;

	return false;
}

void Session::PostSend()
{
	if(false == IsConnected())
		return;

	mSendContext.Init();
	mSendContext.owner = shared_from_this();

	vector<WSABUF> wsaBufs;

	while(mSendQueue.empty() == false) {
		shared_ptr<SendBuffer> sendBuffer{ nullptr };
		if(false == mSendQueue.try_pop(sendBuffer))
			break;
		mSendContext.sendBuffers.push_back(sendBuffer);
	}

	for(const shared_ptr<SendBuffer>& sendBuffer : mSendContext.sendBuffers) {
		WSABUF wsaBuf;
		wsaBuf.buf = const_cast<char*>(sendBuffer->GetBuffer());
		wsaBuf.len = sendBuffer->GetDataSize();
		wsaBufs.push_back(wsaBuf);
	}

	DWORD sizeSent{};
	if(SOCKET_ERROR == WSASend(m_socket, wsaBufs.data(), wsaBufs.size(), &sizeSent, 0, &mSendContext, nullptr)) {
		int errorCode = ::WSAGetLastError();
		if(errorCode != WSA_IO_PENDING) {
			print_error_message(errorCode);
			mSendContext.owner = nullptr;
			mSendContext.sendBuffers.clear();
			mSendRegistred.store(false);
		}
	}
}

void Session::PostRecv()
{
	if(false == IsConnected())
		return;

	mRecvContext.Init();
	mRecvContext.owner = shared_from_this();

	WSABUF wsaBuf;
	wsaBuf.buf = m_recvBuffer.GetWritePos();
	wsaBuf.len = m_recvBuffer.GetFreeSize();

	DWORD numOfBytes{};
	DWORD flags{};

	int ret = WSARecv(m_socket, &wsaBuf, 1, NULL, &flags, &mRecvContext, nullptr);

	if(0 != ret) {
		auto err_no = ::WSAGetLastError();
		if(WSA_IO_PENDING != err_no) {
			print_error_message(err_no);
			mRecvContext.owner = nullptr;
			PostDisconnect();
		}
	}
}
