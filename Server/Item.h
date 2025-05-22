#pragma once

#include "StaticObject.h" 

class Item : public StaticObject {
private:
	ITEM_TYPE	m_itemType;

public:
	explicit Item(const ITEM_TYPE type);
	virtual ~Item();

	unsigned char GetItemType() const noexcept { return static_cast<unsigned char>(m_itemType); }
};

