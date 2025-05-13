#include "pch.h"
#include "Board.h"

#include "Sector.h"

bool Board::CanGo(const Vec2Int pos)
{
	if(pos.x < 0 || pos.x >= BOARD_WIDTH || pos.y < 0 || pos.y >= Board::BOARD_HEIGHT)
		return false;

	return true;
}

void Board::MakeSectors()
{
	// Sector »ý¼º
	for(int y = 0; y < SECTOR_COUNT_X; ++y) {
		for(int x = 0; x < SECTOR_COUNT_Y; ++x) {
			mSectors[y][x] = make_shared<Sector>(x, y);
		}
	}
}

shared_ptr<Sector> Board::GetSector(const int posX, const int posY)
{
	const int x =	posX	 / SECTOR_SIZE;
	const int y =	posY	/ SECTOR_SIZE;

	if(x < 0 || x >= SECTOR_COUNT_X)
		return nullptr;
	if(y< 0 || y >= SECTOR_COUNT_Y)
		return nullptr;


	return mSectors[y][x];
}

shared_ptr<Sector> Board::GetSector(const int sectorID)
{
	for(int y = 0; y < SECTOR_COUNT_Y; ++y) {
		for(int x = 0; x < SECTOR_COUNT_X; ++x) {
			if(sectorID == mSectors[y][x]->GetID())
				return mSectors[y][x];
		}
	}
}

std::unordered_set<int> Board::GetNeighborSector(const int x, const int y)
{
	unordered_set<int> neighborSecList;

	pair<int, int> checkX{ x - VIEW_RANGE, x + VIEW_RANGE };
	pair<int, int> checkY{ y - VIEW_RANGE, y + VIEW_RANGE };

	for(int y = checkY.first; y <= checkY.second; ++y) {
		for(int x = checkX.first; x <= checkX.second; ++x) {

			if(x < 0 || y < 0 || x >= Board::BOARD_WIDTH || y >= Board::BOARD_HEIGHT)
				continue;

			const int secX = GetSectorX(x);
			const int secY = GetSectorX(y);

			const int secID = mSectors[secY][secX]->GetID();

			if(neighborSecList.count(secID) == 0) {
				neighborSecList.insert(secID);
			}
		}
	}

	return neighborSecList;
}
