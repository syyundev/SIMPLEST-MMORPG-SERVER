#pragma once

#include "MovingObject.h"

class Player;

struct PosHash {
	size_t operator()(const Pos& p) const noexcept { return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1); }
};

struct Node {
	Pos pos;
	int g; // 시작부터 이 노드까지의 실제 비용
	int f; // g + 휴리스틱 비용
	auto operator<=> (const Node& o) const noexcept { return f <=> o.f; }
};

inline int heuristic(const Pos& a, const Pos& b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }

class Monster : public MovingObject {
private:
	MONSTER_TYPE			m_monType;

	std::atomic_bool		m_isActive;
	weak_ptr<Player>		m_target;
	std::vector<Pos>		m_path;

	atomic_bool				m_hurt;

public:
	Monster();
	virtual ~Monster();

public:
	void SetTarget(weak_ptr<Player> target) { m_target = target; }
	void SetActive(bool active) noexcept { m_isActive = active; }
	bool IsActive() const noexcept { return m_isActive; }

public:
	void Move();
	void WakeUp();

public:
	unsigned char GetMonType() const noexcept { return static_cast<unsigned char>(m_monType); }
	virtual void Attack(const int targetID) override;
public:
	virtual void Revive() override;

private:
	std::vector<Pos> DoAstar();
	std::vector<Pos> reconstructPath(const std::unordered_map<Pos, Pos, PosHash>& cameFrom, Pos current);

private:
	void			Trace();
	void			RandomMove();
	bool			SearchTarget();
	DIRECTION_TYPE	GetLookDir(const Pos prev, const Pos next);
};