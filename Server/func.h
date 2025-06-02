#pragma once

int API_SendMessage(lua_State* L);
int API_get_x(lua_State* L);
int API_get_y(lua_State* L);
int API_get_monster_move_count(lua_State* L);
int API_MonsterRandomMove(lua_State* L);
int API_ResetMonsterMoveCount(lua_State* L);