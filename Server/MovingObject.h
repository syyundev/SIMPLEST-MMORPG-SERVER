#pragma once
#include "ServerObject.h"
class MovingObject : public ServerObject {
private:
	Stat					m_stat;
	long long				m_lastMoveTime;
	long long				m_lastAttackTime;
	DIRECTION_TYPE			m_dir;
	
	MOVING_OBJECT_STATE		m_state;
protected:
	atomic_bool				m_alive;
public:
	explicit				MovingObject(const OBJECT_TYPE type);
	virtual					~MovingObject();

public:
	void					SetLastMoveTime(const long long lastMoveTime) noexcept { m_lastMoveTime = lastMoveTime; }
	void					SetLastAttackTime(const long long lastAttackTime) noexcept { m_lastAttackTime = lastAttackTime; }
	void					SetDir(const DIRECTION_TYPE dir) { m_dir = dir; }
	void					SetHP(const int hp);
	void					SetMaxHP(const int maxHP) noexcept { m_stat.maxHp = maxHP; }
	void					SetExp(const int exp) noexcept;
	void					SetAlive(const bool alive)  noexcept { m_alive = alive; }
	void					SetLevel(const int level) noexcept { m_stat.level = level; }
	int						GainExp();

	void					AddExp(const int amount) noexcept { m_stat.exp.fetch_add(amount); }
	void					AddHP(const int amount) noexcept { m_stat.hp.fetch_add(amount); }
	void					SubHP(const int amount) noexcept;
	void					SubExp(const int amount) noexcept { m_stat.exp.fetch_sub(amount); }
	int						GetHP() const noexcept { return m_stat.hp; }
	int						GetMaxHP() const noexcept { return m_stat.maxHp; }
	int						GetExp() const noexcept { return m_stat.exp; }
	bool					IsAlive() const noexcept { return m_alive; }

	void					SetState(const MOVING_OBJECT_STATE state) noexcept { m_state = state; }
	MOVING_OBJECT_STATE		GetState() const noexcept { return m_state; }

public:
	int				GetLevel() const noexcept { return m_stat.level; }
	long long		GetLastMoveTime() const noexcept { return m_lastMoveTime; }
	long long		GetLastAttackTime() const noexcept { return m_lastAttackTime; }
	char			GetDir() const noexcept { return static_cast<char>(m_dir); }
	virtual void			Attack(const int targetID) {}
public:
	virtual void Revive() {};

};

