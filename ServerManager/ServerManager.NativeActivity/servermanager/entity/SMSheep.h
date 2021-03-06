#pragma once

#include "SMAnimal.h"

class Sheep;

class SMSheep : public SMAnimal
{
public:
	SMSheep(Server *server, Sheep *entity);

	Sheep *getHandle() const;
	void setHandle(Sheep *entity);
};
