#pragma once

#include "SMPathfinderMob.h"

class AgableMob;

class SMAgableMob : public SMPathfinderMob
{
public:
	SMAgableMob(Server *server, AgableMob *entity);

	AgableMob *getHandle() const;
	void setHandle(AgableMob *entity);
};
