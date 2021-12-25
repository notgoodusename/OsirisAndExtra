#include "Resolver.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"

#include "../Interfaces.h"

namespace Resolver
{
	void run(UserCmd* cmd) noexcept
	{
		if (!localPlayer || !localPlayer->isAlive())
			return;

        for (int i = 0; i < records.size(); i++) {
            auto record = &records.at(i);

            if (!record)
                continue;

            auto entity = interfaces->entityList->getEntity(i);

            if (!entity || entity->isDormant() || !entity->isAlive()) {

                if (record->FiredUpon || record->wasTargeted) {

                    record->lastworkingshot = record->missedshots;
                }
                record->invalid = true;

                continue;
            }

            if (record->invalid == true) {
                record->prevhealth = entity->health();
            }
            record->invalid = false;

            if (!record->FiredUpon)
                continue;

            else if (record->prevhealth == entity->health()) {

                record->lastworkingshot = -1;
                record->missedshots = (record->missedshots >= 12 ? 0 : ++record->missedshots);
                record->wasUpdated = false;
            }

            record->prevhealth = entity->health();
        }

		for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {

            auto entity = interfaces->entityList->getEntity(i);

            if (!entity || entity->isDormant() || !entity->isAlive() || !entity->isPlayer())
                continue;

            if (entity == localPlayer.get())
                continue;

            auto AnimState = entity->getAnimstate();
            if (!AnimState)
                continue;

			// add check if resolver feature is enabled
            Resolver::basic(entity);   
		}
	}

	void basic(Entity* entity) noexcept
	{
        auto record = &records.at(entity->index());

        
	}
}