#pragma once

#include "SMMob.h"

class Enderman;

class SMEnderman : public SMMob
{
public:
	SMEnderman(Server *server, Enderman *entity);

	Enderman *getHandle() const;
	void setHandle(Enderman *entity);
};
