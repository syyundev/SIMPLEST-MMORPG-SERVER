#include "pch.h"
#include "Item.h"

Item::Item(const ITEM_TYPE type)
	:StaticObject(OBJECT_TYPE::ITEM), m_itemType(type)
{
	static int idGen = 300'000;
	SetID(idGen);
	idGen++;
}

Item::~Item()
{
}
