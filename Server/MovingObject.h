#pragma once
#include "ServerObject.h"
class MovingObject : public ServerObject {
private:
	Stat				m_stat;
	long long			m_lastMoveTime;
	long long			m_lastAttackTime;
	DIRECTION_TYPE		m_dir;

public:
	explicit		MovingObject(const OBJECT_TYPE type);
	virtual			~MovingObject();

public:
	void			SetLastMoveTime(const long long lastMoveTime) noexcept { m_lastMoveTime = lastMoveTime; }
	void			SetLastAttackTime(const long long lastAttackTime) noexcept { m_lastAttackTime = lastAttackTime; }

	void			SetDir(const DIRECTION_TYPE dir) { m_dir = dir; }

	void			SetHP(const int hp) { m_stat.hp.store(hp); }
	int				GetHP() const noexcept { return m_stat.hp; }
	void			AddHP(const int amount) noexcept { m_stat.hp.fetch_add(amount); }
	void			SubHP(const int amount) noexcept { m_stat.hp.fetch_sub(amount); }

	void			SetMaxHP(const int maxHP) noexcept { m_stat.maxHp = maxHP; }
	int				GetMaxHP() const noexcept { return m_stat.maxHp; }

	void			SetExp(const int exp) noexcept { m_stat.exp = exp; }
	int				GetExp() const noexcept { return m_stat.exp; }
	void			AddExp(const int amount) noexcept { m_stat.exp.fetch_add(amount); }
	void			SubExp(const int amount) noexcept { m_stat.exp.fetch_sub(amount); }

public:
	int				GetLevel() const noexcept { return m_stat.level; }
	long long		GetLastMoveTime() const noexcept { return m_lastMoveTime; }
	long long		GetLastAttackTime() const noexcept { return m_lastAttackTime; }
	char			GetDir() const noexcept { return static_cast<char>(m_dir); }

};

