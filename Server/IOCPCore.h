#pragma once

#include "Singleton.hpp"

class IOCPRegistrable;

class IOCPCore : public Singleton<IOCPCore> {
	SINGLETON(IOCPCore)

private:
	HANDLE mIocpHandle;
	
public:
	bool Init();
	void ProcessIO();
	void Destory();
	bool Regist(const shared_ptr<IOCPRegistrable>& object);
	bool Regist(const SOCKET socket);
	HANDLE GetHandle() const noexcept { return mIocpHandle; }
};

