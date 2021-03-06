#pragma once

#include "SMAgableMob.h"

class WaterAnimal;

class SMWaterMob : public SMAgableMob
{
public:
	SMWaterMob(Server *server, WaterAnimal *entity);

	WaterAnimal *getHandle() const;
	void setHandle(WaterAnimal *entity);
};
