#include "pch.h"
#include "ServerObject.h"

#include "Board.h"
#include "Sector.h"

ServerObject::ServerObject(const OBJECT_TYPE type)
	:m_id{ -1 },m_pos{0,0}, m_state{SERVER_STATE::ST_FREE}, m_type(type)
{
}

ServerObject::~ServerObject()
{
	cout << "~ServerObject" << endl;
}	