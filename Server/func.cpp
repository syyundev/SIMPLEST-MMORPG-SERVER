#include "pch.h"
#include "func.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "Session.h"
#include "Player.h"
#include "Board.h"

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	auto obj = MANAGER(ServerObjectManager)->GetGameObject(user_id);

	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;
	if(static_cast<OBJECT_TYPE>(obj->GetObjType()) != OBJECT_TYPE::PLAYER) return -1;
	
	SC_CHAT_PACKET sendPkt;
	sendPkt.size = sizeof(sendPkt);
	sendPkt.type = SC_CHAT;
	sendPkt.id = my_id;
	strcpy(sendPkt.chat, mess);
	auto sendBuffer{ make_shared<SendBuffer>() };
	sendBuffer->Append(sendPkt);
	std::static_pointer_cast<Player>(obj)->GetOwnerSession()->RegistSend(std::move(sendBuffer));
	return 0;
}

int API_get_x(lua_State* L)
{
	const int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto obj = MANAGER(ServerObjectManager)->GetGameObject(user_id);
	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;
	int x = obj->GetPos().x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	const int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	auto obj = MANAGER(ServerObjectManager)->GetGameObject(user_id);
	if(obj == nullptr || obj->GetServerState() != ST_INGAME) return -1;
	int y = obj->GetPos().y;
	lua_pushnumber(L, y);
	return 1;
}

int API_get_random_x(lua_State* L)
{
	Pos pos{};
	while(true) {
		pos = Pos{ randomPos(dre), randomPos(dre) };
		if(MANAGER(Board)->CanGo(pos))
			break;
	}
	lua_pushnumber(L, pos.x);
	return 1;
}

int API_get_random_y(lua_State* L)
{
	Pos pos{};
	while(true) {
		pos = Pos{ randomPos(dre), randomPos(dre) };
		if(MANAGER(Board)->CanGo(pos))
			break;
	}
	lua_pushnumber(L, pos.y);
	return 1;
}
