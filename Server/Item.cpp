#include "pch.h"
#include "Item.h"

Item::Item(const ITEM_TYPE type)
	:StaticObject(OBJECT_TYPE::ITEM), m_itemType(type)
{
}

Item::~Item()
{
}
