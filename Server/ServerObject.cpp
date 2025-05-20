#include "pch.h"
#include "ServerObject.h"

#include "Board.h"
#include "Sector.h"

ServerObject::ServerObject(const OBJECT_TYPE type)
	:m_id{ -1 },m_pos{0,0}, m_state{S_STATE::ST_FREE}, m_isActive(false), m_lastMoveTime{0}, m_type(type)
{
}

ServerObject::~ServerObject()
{
	cout << "~ServerObject" << endl;
}