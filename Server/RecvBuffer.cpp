#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int bufferSize)
	:m_bufferSize(bufferSize), m_readCursor(0), m_writeCursor(0)
{
	m_capacity = m_bufferSize * 10;
	m_buffer.resize(m_capacity);
}

RecvBuffer::~RecvBuffer()
{
}

bool RecvBuffer::MoveReadCursor(const int numBytes) noexcept
{
	if(numBytes > GetDataSize())
		return false;

	m_readCursor += numBytes;

	return true;
}

bool RecvBuffer::MoveWriteCursor(const int numBytes) noexcept
{ 
	if(numBytes > GetFreeSize())
		return false;

	m_writeCursor += numBytes;
	return true;
}

void RecvBuffer::Clean() noexcept
{
	const int dataSize = GetDataSize();
	if(dataSize == 0) {
		m_readCursor= m_writeCursor= 0;
	}
	else {
		if(GetFreeSize() < m_bufferSize) {
			::memcpy(&m_buffer[0], &m_buffer[m_readCursor], dataSize);
			m_readCursor = 0;
			m_writeCursor = dataSize;
		}
	}
}
