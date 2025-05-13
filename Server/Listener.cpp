#include "pch.h"
#include "Listener.h"

#include "IOContext.h"
#include "IOCPCore.h"
#include "Session.h"
#include "SessionManager.h"

Listener::Listener()
{
}

Listener::~Listener()
{
	closesocket(mSocket);
}

bool Listener::Init()
{
	mSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if(mSocket == INVALID_SOCKET) [[unlikely]]
		return false;

		if(false == MANAGER(IOCPCore)->Regist(shared_from_this()))
			return false;

		memset(&mServerAddr, 0, sizeof(mServerAddr));
		mServerAddr.sin_family = AF_INET;
		mServerAddr.sin_port = htons(PORT_NUM);
		mServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;

		//bool flag{ true };
		//if(SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&flag), sizeof(flag)))
		//	return false;

		//LINGER option;
		//option.l_onoff = 0;
		//option.l_linger = 0;
		//if(SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&option), sizeof(option)))
		//	return false;

		if(SOCKET_ERROR == ::bind(mSocket, reinterpret_cast<sockaddr*>(&mServerAddr), sizeof(mServerAddr)))
			return false;

		listen(mSocket, SOMAXCONN);

		mAccpetContext = make_unique<AcceptContext>();

		RegistAccept();

		return true;
}
void Listener::ProcessIOCompletion(IOContext* ioContext, const DWORD numOfBytes)
{
	assert(IO_CONTEXT_TYPE::ACCEPT == ioContext->contextType);
	AcceptContext*  acceptContext = static_cast<AcceptContext*>(ioContext);
	ProcessAccept(acceptContext);
}

void Listener::RegistAccept()
{
	mAccpetContext->socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	mAccpetContext->owner = shared_from_this();

	DWORD bytesReceived = 0;

	if(false == AcceptEx(mSocket, mAccpetContext->socket, mAccpetContext->buffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(mAccpetContext.get()))) {
		const int errorCode = ::WSAGetLastError();
		if(errorCode != WSA_IO_PENDING) {
			RegistAccept();
		}
	}
}

void Listener::ProcessAccept(AcceptContext* acceptContext)
{
	const SOCKET acceptSocket = acceptContext->socket;

	if(false == MANAGER(IOCPCore)->Regist(acceptSocket))
		assert(nullptr);

	// TODO: SessionPoolø°º≠ ººº« π›≥≥«ÿ¡‡æﬂ«‘.
	shared_ptr<Session> newSession = make_shared<Session>(acceptSocket);

	if(SOCKET_ERROR == setsockopt(newSession->GetSocket(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<const char*>(&mSocket), sizeof(mSocket))) {
		RegistAccept();
		return;
	}

	SOCKADDR_IN sockAddress;
	int sizeOfSockAddr = sizeof(sockAddress);
	if(SOCKET_ERROR == ::getpeername(newSession->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr)){
		RegistAccept();
		return;
	}

	newSession->SetSockAddrIn(sockAddress);
	newSession->ProcessConnect();
	MANAGER(SessionManager)->AddSession(newSession);

	RegistAccept();
}
