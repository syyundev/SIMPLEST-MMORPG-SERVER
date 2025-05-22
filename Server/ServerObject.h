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

	DIRECTION_TYPE		m_dir;

private:

public:


public:
	explicit		ServerObject(const OBJECT_TYPE type);
	virtual			~ServerObject();

public:	
	void			SetID(const int id) noexcept { m_id = id; }
	void			SetPos(const Pos pos) noexcept { m_pos = pos; }
	void			SetState(const S_STATE state) noexcept { m_state = state; }
	void			SetName(string_view name) { m_name = name.data(); }
	void			SetLastMoveTime(const long long lastMoveTime) noexcept { m_lastMoveTime = lastMoveTime; }

	void			SetDir(const DIRECTION_TYPE dir) { m_dir = dir; }
	
	void			SetHP(const int hp) { m_stat.hp.store(hp); }
	int				GetHP() const noexcept { return m_stat.hp; }
	void			AddHP(const int amount) noexcept { m_stat.hp.fetch_add(amount); }
	void			SubHP(const int amount) noexcept { m_stat.hp.fetch_sub(amount); }

	void			SetMaxHP(const int maxHP) noexcept  { m_stat.maxHp = maxHP; }
	int				GetMaxHP() const noexcept  { return m_stat.maxHp; }
	
	void			SetExp(const int exp) noexcept { m_stat.exp = exp; }
	int				GetExp() const noexcept { return m_stat.exp; }
	void			AddExp(const int amount) noexcept  { m_stat.exp.fetch_add(amount); }
	void			SubExp(const int amount) noexcept  { m_stat.exp.fetch_sub(amount); }

	int				GetLevel() const noexcept { return m_stat.level;}

public:
	Pos				GetPos() const noexcept { return m_pos; }
	int				GetID() const noexcept { return m_id; }
	S_STATE			GetState() const noexcept { return m_state; }
	const string&	GetName() const noexcept { return m_name; }
	long long		GetLastMoveTime() const noexcept { return m_lastMoveTime; }
	char			GetDir() const noexcept { return static_cast<char>(m_dir); }
	unsigned char	GetObjType() const noexcept { return static_cast<unsigned char>(m_type); }
		};

