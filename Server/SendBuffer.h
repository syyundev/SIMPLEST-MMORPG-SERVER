#pragma once

class SendBuffer {
private:
	vector<char> mSendBuffer;

public:
	template<typename Packet>
	void Append(Packet&& packet) noexcept
	{
		const int packetSize = sizeof(std::decay_t<Packet>);
		mSendBuffer.resize(packetSize);
		memcpy(mSendBuffer.data(), reinterpret_cast<char*>(&packet), packetSize);
	}

	const char* GetBuffer() const noexcept { return mSendBuffer.data(); }
	const int GetDataSize() const noexcept { return static_cast<int>(mSendBuffer.size()); }
};

