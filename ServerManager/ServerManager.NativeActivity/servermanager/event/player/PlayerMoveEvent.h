#pragma once

#include "PlayerEvent.h"
#include "../Cancellable.h"
#include "../../Location.h"

class PlayerMoveEvent : public PlayerEvent, public Cancellable
{
private:
	static HandlerList *handlers;
	Location from;
	Location to;
	bool cancel;

public:
	PlayerMoveEvent(SMPlayer *player, const Location &from, const Location &to);

	const Location &getFrom() const;
	void setForm(const Location &form);

	const Location &getTo() const;
	void setTo(const Location &to);

	bool isCancelled() const;
	void setCancelled(bool cancel);

	HandlerList *getHandlers() const;
	static HandlerList *getHandlerList();
};
