#include "pch.h"
#include "MovingObject.h"

MovingObject::MovingObject(const OBJECT_TYPE type)
	:ServerObject(type), m_lastMoveTime{ 0 }, m_dir(DIRECTION_TYPE::LEFT), m_lastAttackTime{0}
{
	m_stat.hp = 100;
	m_stat.maxHp = 100;
	m_stat.exp = 0;
	m_stat.level = 0;
}

MovingObject::~MovingObject()
{
}
