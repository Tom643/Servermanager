#pragma once

#include "../event/block/Action.h"
#include "../event/player/PlayerInteractEvent.h"

class Player;
class ItemInstance;

class EventFactory
{
public:
	static std::unique_ptr<PlayerInteractEvent> callPlayerInteractEvent(Player *who, Action action, ItemInstance *itemInHand);
	static std::unique_ptr<PlayerInteractEvent> callPlayerInteractEvent(Player *who, Action action, int clickedX, int clickedY, int clickedZ, int clickedFace, ItemInstance *itemInHand);
};
