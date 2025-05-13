#pragma once
class RecvBuffer {
private:
	vector<char>	m_buffer;
	int				m_readCursor;
	int				m_writeCursor;
	int				m_capacity;
	int				m_bufferSize;

public:
	explicit RecvBuffer(const int bufferSize);
	~RecvBuffer();

public:
	bool		MoveReadCursor(const int numBytes) noexcept;
	bool		MoveWriteCursor(const int numBytes) noexcept;
	void		Clean() noexcept;
	const char* GetReadPos() noexcept { return &m_buffer[m_readCursor]; }
	char*		GetWritePos() noexcept { return &m_buffer[m_writeCursor]; }
	int			GetDataSize() const noexcept { return m_writeCursor - m_readCursor; }
	int			GetFreeSize() const noexcept { return m_capacity - m_writeCursor; }
};