#include <algorithm>

#include "PluginManager.h"
#include "../Server.h"
#include "../command/CommandMap.h"
#include "../command/PluginCommand.h"
#include "PluginBase.h"
#include "RegisteredListener.h"
#include "PluginDescriptionFile.h"
#include "../event/Event.h"
#include "../event/HandlerList.h"
#include "../event/server/PluginEnableEvent.h"
#include "../event/server/PluginDisableEvent.h"
#include "../util/SMUtil.h"
#include "../version.h"

PluginManager::PluginManager(Server *instance, CommandMap *commandMap)
{
	server = instance;
	this->commandMap = commandMap;
}

void PluginManager::registerPlugin(Plugin *plugin)
{
	prePlugins.push_back(plugin);
}

std::vector<Plugin *> PluginManager::loadPlugins(const std::string &pluginDir)
{
	this->pluginDir = pluginDir;

	std::vector<Plugin *> result;

	std::map<std::string, Plugin *> plugins;
	std::vector<std::string> loadedPlugins;
	std::map<std::string, std::vector<std::string>> dependencies;
	std::map<std::string, std::vector<std::string>> softDependencies;

	for(Plugin *plugin : prePlugins)
	{
		PluginDescriptionFile description(pluginDir + plugin->getPluginDescription());
		std::string name = SMUtil::toLower(description.getName());
		if(name.find(' ') != std::string::npos)
		{
		}
		if(!description.isLoaded() || !name.compare("servermanager") || !name.compare("minecraft") || !name.compare("mojang"))
			continue;

		bool compatible = false;

		for(int smVersion : description.getSMVersions())
			if(smVersion == VERSION_CODE)
				compatible = true;

		if(!compatible)
			continue;

		plugins[description.getName()] = plugin;

		std::vector<std::string> softDependency = description.getSoftDepend();
		if(!softDependency.empty())
		{
			if(softDependencies.find(description.getName()) != softDependencies.end())
				softDependencies[description.getName()].insert(softDependency.end(), softDependency.begin(), softDependency.end());
			else
				softDependencies[description.getName()] = softDependency;
		}

		std::vector<std::string> dependency = description.getDepend();
		if(!dependency.empty())
			dependencies[description.getName()] = dependency;

		std::vector<std::string> loadBefore = description.getLoadBefore();
		if(!loadBefore.empty())
		{
			for(std::string loadBeforeTarget : loadBefore)
			{
				if(softDependencies.find(loadBeforeTarget) != softDependencies.end())
					softDependencies[loadBeforeTarget].push_back(description.getName());
				else
				{
					std::vector<std::string> shortSoftDependency;
					shortSoftDependency.push_back(description.getName());
					softDependencies[loadBeforeTarget] = shortSoftDependency;
				}
			}
		}
	}

	while(!plugins.empty())
	{
		bool missingDependency = true;

		auto pluginIterator = plugins.begin();
		for(; pluginIterator != plugins.end();)
		{
			bool pluginsRemoved = false;
			std::string plugin = pluginIterator->first;

			if(dependencies.find(plugin) != dependencies.end())
			{
				auto dependencyIterator = dependencies[plugin].begin();
				for(; dependencyIterator != dependencies[plugin].end();)
				{
					bool dependencyRemoved = false;
					std::string dependency = *dependencyIterator;

					if(std::find(loadedPlugins.begin(), loadedPlugins.end(), dependency) != loadedPlugins.end())
					{
						dependencyIterator = dependencies[plugin].erase(dependencyIterator);
						dependencyRemoved = true;
					}
					else if(plugins.find(dependency) == plugins.end())
					{
						missingDependency = false;
						pluginIterator = plugins.erase(pluginIterator);
						pluginsRemoved = true;
						softDependencies.erase(plugin);
						dependencies.erase(plugin);
						break;
					}

					if(!dependencyRemoved)
						 ++dependencyIterator;
				}

				if(dependencies.find(plugin) != dependencies.end() && dependencies[plugin].empty())
					dependencies.erase(plugin);
			}
			if(softDependencies.find(plugin) != softDependencies.end())
			{
				auto softDependencyIterator = softDependencies[plugin].begin();
				for(; softDependencyIterator != softDependencies[plugin].end();)
				{
					std::string softDependency = *softDependencyIterator;

					if(plugins.find(softDependency) == plugins.end())
						softDependencyIterator = softDependencies[plugin].erase(softDependencyIterator);
					else
						++softDependencyIterator;
				}

				if(softDependencies[plugin].empty())
					softDependencies.erase(plugin);
			}
			if(!(dependencies.find(plugin) != dependencies.end() ||
					softDependencies.find(plugin) != softDependencies.end()) &&
					plugins.find(plugin) != plugins.end())
			{
				Plugin *p = plugins[plugin];
				pluginIterator = plugins.erase(pluginIterator);
				missingDependency = false;

				result.push_back(loadPlugin(p));
				loadedPlugins.push_back(plugin);
				continue;
			}

			if(!pluginsRemoved)
				++pluginIterator;
		}

		if(missingDependency)
		{
			pluginIterator = plugins.begin();
			for(; pluginIterator != plugins.end(); ++pluginIterator)
			{
				std::string plugin = pluginIterator->first;

				if(dependencies.find(plugin) == dependencies.end())
				{
					softDependencies.erase(plugin);
					missingDependency = false;
					Plugin *p = plugins[plugin];
					pluginIterator = plugins.erase(pluginIterator);

					result.push_back(loadPlugin(p));
					loadedPlugins.push_back(plugin);
					break;
				}
			}

			if(missingDependency)
			{
				softDependencies.clear();
				dependencies.clear();

				auto failedPluginIt = plugins.begin();
				for(; failedPluginIt != plugins.end();)
					failedPluginIt = plugins.erase(failedPluginIt);
			}
		}
	}

	return result;
}

Plugin *PluginManager::loadPlugin(Plugin *plugin)
{
	PluginDescriptionFile *description = new PluginDescriptionFile(pluginDir + plugin->getPluginDescription());
	std::string dataFolder = pluginDir + description->getName() + "/";

	((PluginBase *)plugin)->init(server, description, dataFolder);

	plugins.push_back(plugin);
	lookupNames[plugin->getDescription()->getName()] = plugin;

	return plugin;
}

Plugin *PluginManager::getPlugin(const std::string &name) const
{
	std::string newName = name;
	if(newName.find(' ') != std::string::npos)
	{
		std::string findString = " ";
		newName.replace(newName.find(findString), findString.length(), "_");
	}
	return lookupNames.at(newName);
}

const std::vector<Plugin *> &PluginManager::getPlugins() const
{
	return plugins;
}

bool PluginManager::isPluginEnabled(const std::string &name) const
{
	return isPluginEnabled(getPlugin(name));
}

bool PluginManager::isPluginEnabled(Plugin *plugin) const
{
	auto it = find(plugins.begin(), plugins.end(), plugin);
	if (plugin && it != plugins.end())
		return plugin->isEnabled();
	return false;
}

void PluginManager::enablePlugin(Plugin *plugin)
{
	if(plugin->isEnabled())
		return;

	std::vector<Command *> pluginCommands = parseJsonCommands(plugin);
	if(!pluginCommands.empty())
		commandMap->registerAll(plugin->getDescription()->getName(), pluginCommands);

	((PluginBase *)plugin)->setEnabled(true);

	PluginEnableEvent enableEvent(plugin);
	server->getPluginManager()->callEvent(enableEvent);

	HandlerList::bakeAll();
}

std::vector<Command *> PluginManager::parseJsonCommands(Plugin *plugin)
{
	std::vector<Command *> pluginCmds;

	std::map<std::string, std::map<std::string, PluginDescriptionFile::CommandDescValue>> cmdMap = plugin->getDescription()->getCommands();
	for(auto &it : cmdMap)
	{
		if(it.first.find(':') != std::string::npos)
			continue;

		Command *newCmd = new PluginCommand(it.first, plugin);
		PluginDescriptionFile::CommandDescValue description = it.second["description"];
		PluginDescriptionFile::CommandDescValue usage = it.second["usage"];
		PluginDescriptionFile::CommandDescValue aliases = it.second["aliases"];

		newCmd->setDescription(description.strValue);
		newCmd->setUsage(usage.strValue);

		std::vector<std::string> aliasList;
		if(aliases.isArray)
		{
			for(std::string alias : aliases.arrayValue)
			{
				if(alias.find(':') != std::string::npos)
					continue;

				aliasList.push_back(alias);
			}
		}
		else
		{
			if(aliases.strValue.find(':') == std::string::npos)
				aliasList.push_back(aliases.strValue);
		}
		newCmd->setAliases(aliasList);

		pluginCmds.push_back(newCmd);
	}
	return pluginCmds;
}

void PluginManager::disablePlugins()
{
	std::vector<Plugin *> plugins = getPlugins();
	for(int i = plugins.size() - 1; i >= 0; i--)
		disablePlugin(plugins[i]);
}

void PluginManager::disablePlugin(Plugin *plugin)
{
	if(!plugin->isEnabled())
		return;

	PluginDisableEvent disableEvent(plugin);
	server->getPluginManager()->callEvent(disableEvent);

	((PluginBase *)plugin)->setEnabled(false);

	HandlerList::unregisterAll(plugin);
}

void PluginManager::clearPlugins()
{
	disablePlugins();

	for (int i = 0; i < plugins.size(); ++i)
		delete plugins[i];

	plugins.clear();
	lookupNames.clear();
	HandlerList::unregisterAll();
}

void PluginManager::callEvent(Event &event)
{
	HandlerList *handlers = event.getHandlers();
	std::vector<RegisteredListener *> listeners = handlers->getRegisteredListeners();

	for (RegisteredListener *registration : listeners)
	{
		if (!registration->getPlugin()->isEnabled())
			continue;

		registration->callEvent(event);
	}
}

void PluginManager::registerEvent(EventType type, Listener *listener, std::function<void(Listener *, Event &)> func, Plugin *plugin, EventPriority priority, bool ignoreCancelled)
{
	if(!plugin->isEnabled())
		return;

	auto executor = [func](Listener *listener, Event &event)
	{
		func(listener, event);
	};

	HandlerList *handlerList = getEventListeners(type);
	if(handlerList)
		handlerList->registerListener(new RegisteredListener(listener, executor, priority, plugin, ignoreCancelled));
}

#include "../event/player/PlayerJoinEvent.h"
#include "../event/player/PlayerQuitEvent.h"
#include "../event/player/PlayerPreLoginEvent.h"
#include "../event/player/PlayerLoginEvent.h"
#include "../event/player/PlayerDropItemEvent.h"
#include "../event/player/PlayerPickupItemEvent.h"
#include "../event/player/PlayerGameModeChangeEvent.h"
#include "../event/player/PlayerBedEnterEvent.h"
#include "../event/player/PlayerBedLeaveEvent.h"
#include "../event/player/PlayerMoveEvent.h"
#include "../event/player/PlayerTeleportEvent.h"
#include "../event/player/PlayerChatEvent.h"
#include "../event/player/PlayerCommandPreprocessEvent.h"
#include "../event/player/PlayerInteractEvent.h"
#include "../event/player/PlayerAnimationEvent.h"
#include "../event/block/SignChangeEvent.h"
#include "../event/block/BlockBreakEvent.h"
#include "../event/block/BlockPlaceEvent.h"
#include "../event/block/BlockExpEvent.h"
#include "../event/entity/CreeperPowerEvent.h"

HandlerList *PluginManager::getEventListeners(EventType type)
{
	switch(type)
	{
		case EventType::PLUGIN_ENABLE: return PluginEnableEvent::getHandlerList();
		case EventType::PLUGIN_DISABLE: return PluginDisableEvent::getHandlerList();
		case EventType::PLAYER_PRE_LOGIN: return PlayerPreLoginEvent::getHandlerList();
		case EventType::PLAYER_LOGIN: return PlayerLoginEvent::getHandlerList();
		case EventType::PLAYER_JOIN: return PlayerJoinEvent::getHandlerList();
		case EventType::PLAYER_QUIT: return PlayerQuitEvent::getHandlerList();
		case EventType::PLAYER_DROP_ITEM: return PlayerDropItemEvent::getHandlerList();
		case EventType::PLAYER_PICKUP_ITEM: return PlayerPickupItemEvent::getHandlerList();
		case EventType::PLAYER_GAMEMODE_CHANGE: return PlayerGameModeChangeEvent::getHandlerList();
		case EventType::PLAYER_BED_ENTER: return PlayerBedEnterEvent::getHandlerList();
		case EventType::PLAYER_BED_LEAVE: return PlayerBedLeaveEvent::getHandlerList();
		case EventType::PLAYER_MOVE: return PlayerMoveEvent::getHandlerList();
		case EventType::PLAYER_TELEPORT: return PlayerTeleportEvent::getHandlerList();
		case EventType::PLAYER_CHAT: return PlayerChatEvent::getHandlerList();
		case EventType::PLAYER_COMMAND_PREPROCESS: return PlayerCommandPreprocessEvent::getHandlerList();
		case EventType::PLAYER_INTERACT: return PlayerInteractEvent::getHandlerList();
		case EventType::PLAYER_ANIMATION: return PlayerAnimationEvent::getHandlerList();
		case EventType::SIGN_CHANGE: return SignChangeEvent::getHandlerList();
		case EventType::BLOCK_BREAK: return BlockBreakEvent::getHandlerList();
		case EventType::BLOCK_EXP: return BlockExpEvent::getHandlerList();
		case EventType::BLOCK_PLACE: return BlockExpEvent::getHandlerList();
		case EventType::CREEPER_POWER: return CreeperPowerEvent::getHandlerList();
		default: return NULL;
	}
}
