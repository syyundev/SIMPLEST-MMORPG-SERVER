#pragma once

#include "IOCPRegistrable.h"

class AcceptContext;

class Listener : public IOCPRegistrable {
private:
	SOCKET mSocket;
	SOCKADDR_IN mServerAddr;
	unique_ptr<AcceptContext> mAccpetContext;

public:
	Listener();
	~Listener();

public:
	bool Init();

public:
	virtual void ProcessIOCompletion(IOContext* ioContext, const DWORD numOfBytes = 0) override;
	virtual HANDLE GetHandle() const override { return reinterpret_cast<HANDLE>(mSocket); };
	
private:
	void RegistAccept();
	void ProcessAccept(AcceptContext* acceptContext);
};

