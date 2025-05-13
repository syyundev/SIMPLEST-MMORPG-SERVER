#include "pch.h"
#include "Packet.h"
#include "Board.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"

#include "Session.h"
#include "SessionManager.h"
#include "ServerManager.h"

int main()
{
	if(false == MANAGER(ServerManager)->Init())
		return -1;
	MANAGER(ServerManager)->ProcessIO();
	MANAGER(ServerManager)->Destory();
		
	//while(true) {
	//	SOCKET clientSocket = WSAAccept(listenSocket, reinterpret_cast<sockaddr*>(&addr), &addrSize, NULL, NULL);
	//	if(clientSocket == INVALID_SOCKET) {
	//		auto ret = ::WSAGetLastError();
	//		print_error_message(ret);
	//		::WSACleanup();
	//		closesocket(listenSocket);
	//		exit(-1);
	//	}

	//	const unsigned char currentSessionCount = MANAGER(SessionManager)->GetCurrentSessionCount();

	//	if(currentSessionCount >= SessionManager::MAX_SESSION_COUNT) {
	//		closesocket(clientSocket);
	//		continue;
	//	}
	//
	//	// 세션 생성 -> DoRecv
	//	
	//	auto newSession = make_shared<Session>(clientSocket);
	//	newSession->SetPos(Vec2Int{ START_POS_X, START_POS_Y });


	//	// 새로운 클라 입장 -> 새로운 클라 정보 보내준다.
	//	{
	//		S2C_CREATE_MY_PIECE sendPkt;
	//		sendPkt.id = newSession->GetID();
	//		sendPkt.x = newSession->GetPos().x;
	//		sendPkt.y = newSession->GetPos().y;
	//		newSession->Send(sendPkt);
	//	}

	//	{
	//		const auto& sessions = MANAGER(SessionManager)->GetSessions();

	//		for(const auto& [id, p] : sessions) {
	//			shared_ptr<Session> session = p;
	//			if(session == nullptr)
	//				continue;

	//			{
	//				// 기존에 맵에 있던 플레이어들을 새로운 플레이어한테 보내준다.
	//				S2C_ADD_PLAYER sendPkt;
	//				sendPkt.id = id;
	//				sendPkt.x = session->GetPos().x;
	//				sendPkt.y = session->GetPos().y;
	//				newSession->Send(sendPkt);
	//			}

	//			{
	//				// 기존 플레이어들에게 새로운 플레이어 정보를 보내준다.
	//				S2C_ADD_PLAYER sendPkt;
	//				sendPkt.id = newSession->GetID();
	//				sendPkt.x = newSession->GetPos().x;
	//				sendPkt.y = newSession->GetPos().y;
	//				session->Send(sendPkt);
	//			}
	//		}
	//	}

	//	MANAGER(SessionManager)->AddSession(std::move(newSession));

	//}

	//MANAGER(SessionManager)->RemoveAllSessions();
	//closesocket(listenSocket);
	//WSACleanup();
}
