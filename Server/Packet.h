#pragma once

//enum class PACKET_TYPE : unsigned short {
//	S2C_CREATE_MY_PIECE,
//
//	C2S_MOVE,
//	S2C_MOVE,
//
//	S2C_ADD_PLAYER,
//	S2C_REMOVE_PLAYER,
//
//};
#pragma pack(push, 1)
struct PacketHeader {
	unsigned char size;
	char	type;
};

///// <summary>
///// S2C_CRETAE_MY_PIECE
///// </summary>
//struct S2C_CREATE_MY_PIECE : public PacketHeader {
//public:
//	uint64_t id;
//	int x, y;
//
//public:
//	S2C_CREATE_MY_PIECE() 
//		:PacketHeader{ sizeof(S2C_CREATE_MY_PIECE), static_cast<unsigned short>(PACKET_TYPE::S2C_CREATE_MY_PIECE) } {}
//};
//
///// <summary>
/////	 C2S_MOVE
///// </summary>
//struct C2S_MOVE_PACKET : public PacketHeader {
//public:
//	uint64_t		id;
//	KEY_INPUT		dir;
//
//public:
//	C2S_MOVE_PACKET()
//		:PacketHeader{ sizeof(C2S_MOVE_PACKET), static_cast<unsigned short>(PACKET_TYPE::C2S_MOVE) } { }
//};
//
///// <summary>
///// S2C_MOVE
///// </summary>
//struct S2C_MOVE_PACKET : public PacketHeader {
//public:
//	uint64_t id;
//	int x, y;
//
//public:
//	S2C_MOVE_PACKET()
//		:PacketHeader{ sizeof(S2C_MOVE_PACKET), static_cast<unsigned short>(PACKET_TYPE::S2C_MOVE) } { }
//};
//
//
///// <summary>
/////  S2C_ADD
///// </summary>
//struct S2C_ADD_PLAYER : public PacketHeader {
//public:
//	uint64_t id;
//	int x, y;
//
//public:
//	S2C_ADD_PLAYER()
//		:PacketHeader{ sizeof(S2C_ADD_PLAYER), static_cast<unsigned short>(PACKET_TYPE::S2C_ADD_PLAYER) } { }
//};
//
///// <summary>
///// S2C_REMOVE
///// </summary>
//struct S2C_REMOVE_PLAYER : public PacketHeader {
//public:
//	uint64_t id;
//
//public:
//	S2C_REMOVE_PLAYER()
//		:PacketHeader{ sizeof(S2C_REMOVE_PLAYER), static_cast<unsigned short>(PACKET_TYPE::S2C_REMOVE_PLAYER) }{}
//};
#pragma pack(pop)

