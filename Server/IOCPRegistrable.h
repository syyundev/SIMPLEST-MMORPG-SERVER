#pragma once

class IOContext;

class IOCPRegistrable : public enable_shared_from_this<IOCPRegistrable> {
public:
	virtual HANDLE GetHandle() const abstract;
	virtual void ProcessIOCompletion(IOContext* ioContext, const DWORD numOfBytes=0) abstract;
};