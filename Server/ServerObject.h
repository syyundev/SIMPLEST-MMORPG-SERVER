#pragma once

class ServerObject : public enable_shared_from_this<ServerObject> {
private:
	// ServerObject
	int					m_id;
	Pos					m_pos;
	SERVER_STATE		m_state;
	string				m_name;
	OBJECT_TYPE			m_type;


public:
	explicit		ServerObject(const OBJECT_TYPE type);
	virtual			~ServerObject();

public:	
	void			SetID(const int id) noexcept { m_id = id; }
	void			SetPos(const Pos pos) noexcept { m_pos = pos; }
	void			SetServerState(const SERVER_STATE state) noexcept { m_state = state; }
	void			SetName(string_view name) { m_name = name.data(); }



public:
	Pos				GetPos() const noexcept { return m_pos; }
	int				GetID() const noexcept { return m_id; }
	SERVER_STATE			GetServerState() const noexcept { return m_state; }
	const string&	GetName() const noexcept { return m_name; }
	unsigned char	GetObjType() const noexcept { return static_cast<unsigned char>(m_type); }
};

