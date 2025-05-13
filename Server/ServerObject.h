#pragma once

class ServerObject {
private:
	uint64_t m_id;
	Vec2Int m_pos;

public:
	void SetID(const uint64_t id) { m_id = id; }
	void SetPos(const Vec2Int pos) { m_pos = pos; }

public:
	uint64_t	GetID() const noexcept { return m_id; }
	Vec2Int		GetPos() const noexcept { return m_pos; }

};

