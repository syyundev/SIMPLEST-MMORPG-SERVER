#include "pch.h"
#include "Session.h"

#include "SessionManager.h"
#include "Board.h"
#include "IOContext.h"
#include "SendBuffer.h"
#include "Board.h"		
#include "Sector.h"
#include "Player.h"
#include "ServerObjectManager.h"

Session::Session(const SOCKET socket)
	:m_socket(socket), m_recvBuffer(65535), mConnected{ false }, mSendRegistred{ false }, mServerState{ ST_FREE }
{
	static int idGen = 1;
	m_id = idGen;
	idGen++;
}

Session::~Session()
{
	std::cout << "~Session, ID: " << m_id << std::endl;
	closesocket(m_socket);
}

void Session::RegistSend(std::shared_ptr<SendBuffer> sendBuffer)
{
	if(false == IsConnected())
		return;

	bool registered{ false };

	mSendQueue.push(std::move(sendBuffer));

	if(mSendRegistred.exchange(true) == false)
		registered = true;

	if(registered)
		PostSend();
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

//void Session::Send(shared_ptr<SendBuffer> sendBuffer)
//{
//	mSendContext.Init();
//	mSendContext.owner = shared_from_this();
//
//	WSABUF wsaBuf;
//	wsaBuf.buf = (char*)sendBuffer->GetBuffer();
//	wsaBuf.len = sendBuffer->GetDataSize();
//
//	WSASend(m_socket, &wsaBuf, 1, 0, 0, &mSendContext, nullptr);
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
	int processLen = ProcessData(m_recvBuffer.GetReadPos(), dataSize); // ÄÁÅÙÃ÷ ÄÚµå¿¡¼­ ÀçÁ¤ÀÇ
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
	m_player->m_viewLock.lock();
	unordered_set<int> vl = m_player->m_viewList;
	m_player->m_viewLock.unlock();

	for(int id : vl) {
		shared_ptr<ServerObject> serverObject = MANAGER(ServerObjectManager)->GetGameObject(id);

		if(serverObject == nullptr)
			continue;

		if(serverObject->GetState() != ST_INGAME)
			continue;

		if(id == m_id)
			continue;

		if(static_cast<OBJECT_TYPE>(serverObject->GetObjType()) != OBJECT_TYPE::PLAYER)
			continue;

		SC_REMOVE_OBJECT_PACKET sendPkt;
		sendPkt.size = sizeof(sendPkt);
		sendPkt.type = SC_REMOVE_OBJECT;
		sendPkt.id = m_id;
		sendPkt.objType = serverObject->GetObjType();
		auto sendBuffer = std::make_shared<SendBuffer>();
		sendBuffer->Append(sendPkt);
		std::static_pointer_cast<Player>(serverObject)->GetOwnerSession()->RegistSend(std::move(sendBuffer));

	}
	shared_ptr<Sector> sector = MANAGER(Board)->GetSector(Pos{ m_player->GetPos().x, m_player->GetPos().y });
	sector->Remove(m_player->GetID());
	MANAGER(ServerObjectManager)->RemoveServerObject(m_id);
	MANAGER(SessionManager)->RemoveSession(m_id);
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

	switch(header.type) {
		case CS_LOGIN:
		{
			Process_CS_LOGIN_PACKET(std::static_pointer_cast<Session>(shared_from_this()), *(reinterpret_cast<const CS_LOGIN_PACKET*>(buffer)));
			break;
		}
		case CS_MOVE:
		{
			Process_CS_MOVE_PACKET(std::static_pointer_cast<Session>(shared_from_this()), *(reinterpret_cast<const CS_MOVE_PACKET*>(buffer)));
			break;
		}
		default:
			break;
	}
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
			continue;
		mSendContext.sendBuffers.push_back(std::move(sendBuffer));
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
