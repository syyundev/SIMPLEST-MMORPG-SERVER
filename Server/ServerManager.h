#pragma once

#include "Singleton.hpp"

class Listener;

class ServerManager : public Singleton<ServerManager> {
	SINGLETON(ServerManager)
private:
	SOCKADDR				mSockAddr;
	shared_ptr<Listener>	mListener;

public:
	bool		Init();
	void		ProcessIO();
	void Destory();

private:
	void Work();
};

