#include "pch.h"
#include "IOCPCore.h"

#include "IOContext.h"
#include "IOCPRegistrable.h"

bool IOCPCore::Init()
{
	mIocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	if(mIocpHandle == INVALID_HANDLE_VALUE) [[unlikely]]
		return false;

		return true;
}

bool IOCPCore::Process()
{
	while(true) {
		DWORD numOfBytes = 0;
		ULONG_PTR key = 0;
		IOContext* ioContext = nullptr;

		if(::GetQueuedCompletionStatus(mIocpHandle, OUT & numOfBytes, OUT & key, OUT reinterpret_cast<LPOVERLAPPED*>(&ioContext), INFINITE)) {
			if(key == -1) {
				int a = 0;
				break;
			}
			shared_ptr<IOCPRegistrable> iocpObject = ioContext->owner;
			iocpObject->ProcessIOCompletion(ioContext, numOfBytes);
		}
		else {
			int errCode = ::WSAGetLastError();
			switch(errCode) {
				case WAIT_TIMEOUT:
				{
					if(key == -1){
						int a = 0;
						break;
					}
					return false;
				}
				default:
				{
					if(key == -1) {
						int a = 0;
						break;
					}
					shared_ptr<IOCPRegistrable> iocpObject = ioContext->owner;
					iocpObject->ProcessIOCompletion(ioContext, numOfBytes);
					break;
				}
			}
		}
	}

	return false;
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
