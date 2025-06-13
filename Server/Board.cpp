#include "pch.h"
#include "Board.h"

#include "Sector.h"

bool Board::CanGo(const Pos pos)
{
	if(pos.x < 0 || pos.x >= BOARD_WIDTH || pos.y < 0 || pos.y >= Board::BOARD_HEIGHT)
		return false;

	if(m_boards[pos.y][pos.x] == TILE_TYPE::OBSTACLE)
		return false;

	return true;
}

void Board::Make()
{
	cout << "甘 积己 矫累..." << endl;
	set<Pos> tempPos;

	std::ifstream ifs{ "Data\\map.bin", std::ios::binary };

	if(!ifs) {
		cout << "File Read Error\n";
		return;
	}

	int count{};
	ifs.read((char*)&count, sizeof(count));
	Pos pos;
	for(int i = 0; i < count; ++i) {
		ifs.read((char*)&pos, sizeof(pos));
		tempPos.insert(pos);
	}

	for(int y = 0; y < m_boards.size(); ++y) {
		for(int x = 0; x < m_boards[y].size(); ++x) {
			Pos obstaclePos{ static_cast<short>(y),static_cast<short>(x) };
			if(tempPos.find(obstaclePos) != tempPos.end()) {
				m_boards[y][x] = TILE_TYPE::OBSTACLE;
				//m_boards[y][x] = TILE_TYPE::ROAD;
			}
			else {
				m_boards[y][x] = TILE_TYPE::ROAD;
			}
		}
	}

	for(int y = 0; y < SECTOR_Y_COUNT; ++y) {
		for(int x = 0; x < SECTOR_X_COUNT; ++x) {
			auto sector = make_shared<Sector>(x, y);
			mSectors[y][x] = sector;
			m_hash[sector->GetID()] = sector;
		}
	}

	cout << "甘 积己 肯丰!" << endl;
}

shared_ptr<Sector> Board::GetSector(const Pos pos)
{
	const short x = pos.x / SECTOR_SIZE;
	const short y = pos.y / SECTOR_SIZE;

	if(x < 0 || x >= SECTOR_X_COUNT)
		return nullptr;
	if(y < 0 || y >= SECTOR_Y_COUNT)
		return nullptr;

	return mSectors[y][x];
}

shared_ptr<Sector> Board::GetSector(const int sectorID)
{
	return m_hash[sectorID];
}

std::unordered_set<int> Board::GetNeighborSectorList(const Pos pos, const int viewRange)
{
	unordered_set<int> neighborSecList;

	const int minViewX{ std::max(0, pos.x - viewRange) };
	const int maxViewX{ std::min(W_WIDTH - 1, pos.x + viewRange) };
	const int minViewY{ std::max(0, pos.y - viewRange) };
	const int maxViewY{ std::min(W_HEIGHT - 1, pos.y + viewRange) };

	int minSecX = minViewX / Board::SECTOR_SIZE;
	int maxSecX = maxViewX / Board::SECTOR_SIZE;
	int minSecY = minViewY / Board::SECTOR_SIZE;
	int maxSecY = maxViewY / Board::SECTOR_SIZE;

	minSecX = std::max(0, minSecX);
	maxSecX = std::min(Board::SECTOR_X_COUNT - 1, maxSecX);
	minSecY = std::max(0, minSecY);
	maxSecY = std::min(Board::SECTOR_Y_COUNT - 1, maxSecY);

	for(int sy = minSecY; sy <= maxSecY; ++sy) {
		for(int sx = minSecX; sx <= maxSecX; ++sx) {
			const int secID = mSectors[sy][sx]->GetID();
			if(neighborSecList.find(secID) == neighborSecList.end()) {
				neighborSecList.insert(secID);
			}
		}
	}

	return neighborSecList;
}

bool Board::CanSee(const Pos from, const Pos to, const int viewRange)
{
	if(abs(from.x - to.x) > viewRange) return false;
	return abs(from.y - to.y) <= viewRange;
}
