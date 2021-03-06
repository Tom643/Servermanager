#pragma once

#include "BlockExpEvent.h"
#include "../Cancellable.h"

class Player;

class BlockBreakEvent : public BlockExpEvent, public Cancellable
{
private:
	Player *player;
	bool cancel;

public:
	BlockBreakEvent(Block *theBlock, Player *player);

	Player *getPlayer() const;

	bool isCancelled() const;
	void setCancelled(bool cancel);
};
