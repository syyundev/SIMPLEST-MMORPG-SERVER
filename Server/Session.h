#pragma once
//void CALLBACK  G_RECV_CALLBACK(DWORD, DWORD, LPWSAOVERLAPPED/*Overlapped 구조체 보고, 어느 클라의 오버랩드 구조체인가를 알아야 함.*/, DWORD);
//void CALLBACK G_SEND_CALLBACK(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);

#include "IOCPRegistrable.h"
#include "IOContext.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"

class SendBuffer;

class Session : public IOCPRegistrable {
private:
	SOCKET				m_socket;
	uint64_t			m_id;
	// Vec2Int			m_pos;
	
	short				mX;
	short				mY;

	RecvBuffer			m_recvBuffer;

	SOCKADDR_IN			mSockAddrIn;

	std::atomic_bool	mConnected;
	std::atomic_bool	mSendRegistred;

	RecvContext			mRecvContext;
	SendContext			mSendContext;
	
public:

	char				mName[NAME_SIZE];
	atomic<S_STATE>		mServerState;
	int					mLastMoveTime;

	concurrency::concurrent_queue<shared_ptr<SendBuffer>> mSendQueue;

	mutex				mVM;
	unordered_set<int>	mViewList;

public:
	explicit Session(const SOCKET socket);
	~Session();

public:
	//void		SetPos(const Vec2Int pos) noexcept { m_pos = pos; }
	
	uint64_t	GetID() const noexcept { return m_id; }
	// Vec2Int		GetPos() const noexcept { return m_pos; }

	void SetX(const short x) noexcept { mX = x; }
	void SetY(const short y) noexcept { mX = y; }

	const short GetX() const noexcept { return mX; }
	const short GetY() const noexcept { return mY; }

public:
	template<typename PacketType>
	void RegistSend(PacketType&& sendPkt)
	{
		if(false == IsConnected())
			return;

		bool registered{ false };

		shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>();
		sendBuffer->Append(std::forward<PacketType>(sendPkt));

		mSendQueue.push(sendBuffer);

		if(mSendRegistred.exchange(true) == false)
			registered = true;

		if(registered)
			PostSend();
	}

public:
	virtual HANDLE GetHandle() const noexcept override { return reinterpret_cast<HANDLE>(m_socket); };
	virtual void ProcessIOCompletion(IOContext* ioContext, const DWORD numOfBytes = 0) override;
	SOCKET GetSocket() const noexcept { return m_socket; }

	bool IsConnected() { return mConnected; }
	
public:
	void SetSockAddrIn(const SOCKADDR_IN& sockAddrIn) noexcept { mSockAddrIn = sockAddrIn; }
	// void OnPostRecv(DWORD err, DWORD numBytes, LPWSAOVERLAPPED/*Overlapped 구조체 보고, 어느 클라의 오버랩드 구조체인가를 알아야 함.*/ pOverlapped, DWORD flag);
	void OnPostRecv(const DWORD numBytes);
	void OnPostSend(const DWORD numBytes);

	void ProcessConnect();

private:
	void PostSend();
	void PostRecv();
	void PostDisconnect();

public:
	int		ProcessData(const char* const buffer, const int len);
	void	ProcessPacket(const char* const buffer, const int packetSize);

public:
	bool CanSee(const shared_ptr<Session>& session);
};

