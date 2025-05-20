#include "pch.h"
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
}
