#pragma once

//class IOContext {
//public:
//	WSAOVERLAPPED m_sendOverlapped;
//	char m_sendBuffer[4096];
//	WSABUF m_sendWsaBufs[1];
//
//public:
//	IOContext()
//	{
//		memset(&m_sendOverlapped, 0, sizeof(m_sendOverlapped));
//	}
//	
//public:
//	template<typename PacketType>
//	void Append(PacketType&& packet)
//	{
//		const size_t packetSize = sizeof(std::decay_t<PacketType>);
//		memcpy(m_sendBuffer, &packet, packetSize);
//		m_sendWsaBufs[0].len = static_cast<ULONG>(packetSize);
//		m_sendWsaBufs[0].buf = m_sendBuffer;
//	}
//};

class Session;
class IOCPRegistrable;
class SendBuffer;

enum class IO_CONTEXT_TYPE : unsigned char {
	ACCEPT,
	RECV,
	SEND,
	CONNECT,
	DISCONNECT,

	EVENT,
};

class IOContext : public OVERLAPPED {
public:
	IO_CONTEXT_TYPE					contextType;
	shared_ptr<IOCPRegistrable>		owner;

public:
	IOContext(const IO_CONTEXT_TYPE contextType)
		: contextType{ contextType }, owner{ nullptr }	{
		Init();
	}

public:
	void Init();
};

class AcceptContext : public IOContext {
public:
	char buffer[1024];
	SOCKET socket;

public:
	AcceptContext()
		:IOContext(IO_CONTEXT_TYPE::ACCEPT) { }
};

class RecvContext : public IOContext {
public:
	RecvContext()
		:IOContext(IO_CONTEXT_TYPE::RECV) {}
};

class SendContext : public IOContext {
public:
	vector<shared_ptr<SendBuffer>> sendBuffers;

public:
	SendContext()
		:IOContext(IO_CONTEXT_TYPE::SEND) {}
};

class ConnectContext : public IOContext {
public:
	ConnectContext()
		:IOContext(IO_CONTEXT_TYPE::CONNECT) {}
};

class DisconnectContext : public IOContext {
public:
	DisconnectContext()
		:IOContext(IO_CONTEXT_TYPE::DISCONNECT) {}
};

class EventContext : public IOContext {
public:
	TASK_TYPE type;

public:
	EventContext()
		:IOContext(IO_CONTEXT_TYPE::EVENT)
	{
	}
};