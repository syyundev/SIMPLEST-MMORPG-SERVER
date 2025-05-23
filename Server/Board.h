#pragma once

#include "Singleton.hpp"

class Sector;

class Board : public Singleton<Board> {
	SINGLETON(Board)

public:
	static constexpr short			BOARD_WIDTH = 2000;
	static constexpr short			BOARD_HEIGHT = 2000;
	static constexpr unsigned char	SQUARE_SIZE = 80;
	
	static constexpr short			SECTOR_SIZE = 10;
	static constexpr short			SECTOR_X_COUNT = (BOARD_WIDTH + SECTOR_SIZE - 1) / SECTOR_SIZE;
	static constexpr short			SECTOR_Y_COUNT = (BOARD_HEIGHT + SECTOR_SIZE -1) / SECTOR_SIZE;
	
public:
	std::array<array<TILE_TYPE, BOARD_WIDTH>, BOARD_HEIGHT> m_boards;
	std::array<std::array<shared_ptr<Sector>, SECTOR_X_COUNT>, SECTOR_Y_COUNT> mSectors;
	unordered_map<int, shared_ptr<Sector>> m_hash;

public:
	bool CanGo(const Pos pos);
	void Make();
	shared_ptr<Sector> GetSector(const Pos pos);
	shared_ptr<Sector> GetSector(const int sectorID);
	Pos GetSectorPos(const Pos pos) { return Pos{ static_cast<short>(pos.x / SECTOR_SIZE), static_cast<short>(pos.y / SECTOR_SIZE )}; }
	std::unordered_set<int> GetNeighborSectorList(const Pos pos);
	bool CanSee(const Pos from, const Pos to);
};

