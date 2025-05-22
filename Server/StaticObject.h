#pragma once

#include "ServerObject.h"

class StaticObject : public ServerObject {
private:

public:
	explicit StaticObject(const OBJECT_TYPE type);
	virtual ~StaticObject();
};

