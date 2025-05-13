#pragma once

#include "Singleton.hpp"

class Sector;

class Board : public Singleton<Board> {
	SINGLETON(Board)

public:
	static constexpr int			BOARD_WIDTH = 400;
	static constexpr int			BOARD_HEIGHT = 400;
	static constexpr unsigned char	SQUARE_SIZE = 80;
	
	static constexpr int			SECTOR_SIZE = 10;
	static constexpr int			SECTOR_COUNT_X = (BOARD_WIDTH + SECTOR_SIZE - 1) / SECTOR_SIZE;
	static constexpr int			SECTOR_COUNT_Y = (BOARD_HEIGHT + SECTOR_SIZE -1) / SECTOR_SIZE;

public:
	std::array<std::array<int, BOARD_WIDTH>, BOARD_HEIGHT>	m_boards;
	
	// 읽기만 할거니까 shared
	std::array<std::array<shared_ptr<Sector>, SECTOR_COUNT_X>, SECTOR_COUNT_Y> mSectors;

public:
	bool CanGo(const Vec2Int pos);
	void MakeSectors();
	shared_ptr<Sector> GetSector(const int posX, const int posY);
	shared_ptr<Sector> GetSector(const int sectorID);
	int GetSectorX(const int posX) { return posX / SECTOR_SIZE; }
	int GetSectorY(const int posY) { return  posY / SECTOR_SIZE; }
	// 시야반경 안에 있는 인접 Sector들, 나의 섹터 포함
	std::unordered_set<int> GetNeighborSector(const int x, const int y);
};

