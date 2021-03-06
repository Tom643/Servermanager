#include "DeopCommand.h"
#include "../../ServerManager.h"
#include "../../entity/SMPlayer.h"

DeopCommand::DeopCommand()
	: VanillaCommand("deop")
{
	description = "Takes the specified player's operator status";
	usageMessage = "%commands.deop.usage";
}

bool DeopCommand::execute(SMPlayer *sender, std::string &label, std::vector<std::string> &args)
{
	if((int)args.size() != 1 || args[0].empty())
	{
		sender->sendTranslation("§c%commands.generic.usage", {usageMessage});
		return false;
	}

	ServerManager::getServer()->removeOp(args[0]);

	SMPlayer *player = ServerManager::getPlayerExact(args[0]);
	if(player)
		player->sendMessage("§7You are no longer op!");

	Command::broadcastCommandTranslation(sender, "commands.deop.success", {args[0]});

	return true;
}
