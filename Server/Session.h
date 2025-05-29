#pragma once

#include "IOCPRegistrable.h"
#include "IOContext.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"

class SendBuffer;
class Player;

class Session : public IOCPRegistrable {
private:
	SOCKET				m_socket;
	int					m_id;
	RecvBuffer			m_recvBuffer;

	SOCKADDR_IN			mSockAddrIn;

	std::atomic_bool	mConnected;
	std::atomic_bool	mSendRegistred;

	RecvContext			mRecvContext;
	SendContext			mSendContext;

	concurrency::concurrent_queue<shared_ptr<SendBuffer>> mSendQueue;
public:
	atomic<SERVER_STATE>		mServerState;

	shared_ptr<Player>	m_player;

public:
	explicit Session(const SOCKET socket);
	~Session();

public:
	void				SetPlayer(const shared_ptr<Player> player) noexcept { m_player = player; }
	void				SetSockAddrIn(const SOCKADDR_IN& sockAddrIn) noexcept { mSockAddrIn = sockAddrIn; }

	int					GetID() const noexcept { return m_id; }
	virtual HANDLE		GetHandle() const noexcept override { return reinterpret_cast<HANDLE>(m_socket); };
	virtual void		ProcessIOCompletion(IOContext* ioContext, const DWORD numOfBytes = 0) override;
	SOCKET				GetSocket() const noexcept { return m_socket; }
	shared_ptr<Player>	GetPlayer() const noexcept { return m_player; }

public:
	void	OnPostRecv(const DWORD numBytes);
	void	OnPostSend(const DWORD numBytes);
	void	RegistSend(std::shared_ptr<SendBuffer> sendBuffer);
	void	ProcessConnect();

	bool	IsConnected() { return mConnected; }

private:
	void	PostSend();
	void	PostRecv();
	void	PostDisconnect();
	int		ProcessData(const char* const buffer, const int len);
	void	ProcessPacket(const char* const buffer, const int packetSize);

};

