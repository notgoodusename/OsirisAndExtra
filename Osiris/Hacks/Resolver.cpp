#include "Resolver.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"

#include "../Interfaces.h"

namespace Resolver
{
	void frameStageUpdate() noexcept
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

            if (!entity || entity->isDormant() || !entity->isAlive() || !entity->isOtherEnemy(localPlayer.get()) || !entity->isPlayer())
                continue;

            if (entity == localPlayer.get())
                continue;

            auto AnimState = entity->getAnimstate();
            if (!AnimState)
                continue;

            auto activeWeapon = localPlayer->getActiveWeapon();
            if (!activeWeapon || !activeWeapon->clip())
                return;

            auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
            if (!weaponIndex)
                return;

            if (config->ragebot[weaponIndex].resolver) {
                Resolver::basic(entity);   
            }
		}
	}

	void basic(Entity* entity) noexcept
	{
        auto record = &records.at(entity->index());

        if (!record || record->invalid)
            return;

        if (entity->velocity().length2D() > 3.0f) {
            record->PreviousEyeAngle = entity->eyeAngles().y;
            return;
        }       

        float desyncAng = 0;

        auto animstate = entity->getAnimstate();

        if (!animstate)
            return;

        if (!record->FiredUpon || !record->wasTargeted) {

            entity->updateState(animstate, entity->eyeAngles());
            if ((record->wasUpdated == false) && (entity->eyeAngles().y != record->PreviousEyeAngle)) {
                //record->PreviousEyeAngle = entity->eyeAngles().y;
                record->eyeAnglesOnUpdate = entity->eyeAngles().y;
                record->prevSimTime = entity->simulationTime();
                record->PreviousEyeAngle = entity->eyeAngles().y + record->PreviousDesyncAng;
                record->wasUpdated = true;
            }

            /*if ((record->wasUpdated == true) && (entity->eyeAngles().y != record->PreviousEyeAngle) && (record->prevSimTime != entity->simulationTime())) {
                record->PreviousEyeAngle = entity->eyeAngles().y;
                
                record->eyeAnglesOnUpdate = entity->eyeAngles().y;
                record->PreviousEyeAngle = entity->eyeAngles().y + record->PreviousDesyncAng;
                record->prevSimTime = entity->simulationTime();
            }*/

            entity->eyeAngles().y = record->PreviousEyeAngle;
            entity->updateState(animstate, entity->eyeAngles());

            return;
        }

        entity->updateState(animstate, entity->eyeAngles());

        int missed = record->missedshots;
        if (record->lastworkingshot != -1)
            missed = record->lastworkingshot;

        switch (missed % 9) {
        case 1:
            desyncAng += 30.0f;
            break;
        case 2:
            desyncAng -= 30.0f;
            break;
        case 3:
            desyncAng += 60.0f;
            break;
        case 4:
            desyncAng -= 60.0f;
            break;
        case 5:
            desyncAng = animstate->footYaw;
            break;
        case 6:
            desyncAng = desyncAng + (entity->getMaxDesyncAngle() * -1);
            break;
        case 7:
            desyncAng = desyncAng + entity->getMaxDesyncAngle();
            break;
        default:
            desyncAng = animstate->footYaw;
            break;
        }

        
        entity->eyeAngles().y += desyncAng;
        animstate->footYaw += desyncAng;
        entity->updateState(animstate, entity->eyeAngles());
        record->PreviousEyeAngle = entity->eyeAngles().y;

        if (record->PreviousEyeAngle > 180) {
            record->PreviousEyeAngle -= 180;
        }
        else if (record->PreviousEyeAngle < -180) {
            record->PreviousEyeAngle += 180;
        }

        record->PreviousDesyncAng = desyncAng;
        record->wasUpdated = true;
        record->FiredUpon = false;
	}
}