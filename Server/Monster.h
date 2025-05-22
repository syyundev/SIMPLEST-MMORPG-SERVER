#pragma once

#include "ServerObject.h"

class Monster : public ServerObject {
private:
	MONSTER_TYPE		m_monType;
	std::atomic_bool	m_isActive;

public:
	explicit Monster(const MONSTER_TYPE type);
	virtual ~Monster();

public:
	void		SetActive(bool active) noexcept { m_isActive = active; }
	bool		IsActive() const noexcept { return m_isActive; }

public:
	void		Move();
	void		WakeUp();

public:
	unsigned char GetMonType() const noexcept { return static_cast<unsigned char>(m_monType); }
};

