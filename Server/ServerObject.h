#pragma once

class ServerObject : public enable_shared_from_this<ServerObject> {
private:
	int					m_id;
	Pos					m_pos;
	S_STATE				m_state;
	long long			m_lastMoveTime;
	string				m_name;
	OBJECT_TYPE			m_type;
	
	Stat				m_stat;
private:

public:
	std::atomic_bool	m_isActive;

public:
	explicit		ServerObject(const OBJECT_TYPE type);
	virtual			~ServerObject();

public:
	void			SetID(const int id) noexcept { m_id = id; }
	void			SetPos(const Pos pos) noexcept { m_pos = pos; }
	void			SetState(const S_STATE state) noexcept { m_state = state; }
	void			SetName(string_view name) { m_name = name.data(); }
	void			SetLastMoveTime(const long long lastMoveTime) noexcept { m_lastMoveTime = lastMoveTime; }
	void			SetActive(bool active) noexcept { m_isActive = active; }
	void			SetStat(const Stat& stat) { m_stat = stat; }

public:
	Pos				GetPos() const noexcept { return m_pos; }
	int				GetID() const noexcept { return m_id; }
	S_STATE			GetState() const noexcept { return m_state; }
	const string&	GetName() const noexcept { return m_name; }
	long long		GetLastMoveTime() const noexcept { return m_lastMoveTime; }

	unsigned char	GetObjType() const noexcept { return static_cast<unsigned char>(m_type); }
	const Stat& GetStat() const noexcept { return m_stat; }
};

