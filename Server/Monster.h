#pragma once

#include "ServerObject.h"

class Monster : public ServerObject {
private:
	MONSTER_TYPE m_monType;

public:
	explicit Monster(const MONSTER_TYPE type);
	virtual ~Monster();

public:
	void Move();
	void WakeUp();
	
public:
	unsigned char GetMonType() const noexcept { return static_cast<unsigned char>(m_monType); }
};

