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

        float DesyncAng = 0;

        auto Animstate = entity->getAnimstate();

        if (!Animstate)
            return;

        if (!record->FiredUpon || !record->wasTargeted) {

            entity->updateState(Animstate, entity->eyeAngles());
            if ((record->wasUpdated == false) && (entity->eyeAngles().y != record->PreviousEyeAngle)) {
                //record->PreviousEyeAngle = entity->eyeAngles().y;
                record->eyeAnglesOnUpdate = entity->eyeAngles().y;
                record->prevSimTime = entity->simulationTime();
                record->PreviousEyeAngle = entity->eyeAngles().y + record->PreviousDesyncAng;
                record->wasUpdated = true;
            }

            if ((record->wasUpdated == true) && (entity->eyeAngles().y != record->PreviousEyeAngle) && (record->prevSimTime != entity->simulationTime())) {
                //record->PreviousEyeAngle = entity->eyeAngles().y;
                /*
                record->eyeAnglesOnUpdate = entity->eyeAngles().y;
                record->PreviousEyeAngle = entity->eyeAngles().y + record->PreviousDesyncAng;
                record->prevSimTime = entity->simulationTime();
                */
            }

            entity->eyeAngles().y = record->PreviousEyeAngle;
            entity->updateState(Animstate, entity->eyeAngles());

            return;
        }

        entity->updateState(Animstate, entity->eyeAngles());

        int missed = record->missedshots;

        if (record->lastworkingshot != -1)
            missed = record->lastworkingshot;


        switch (missed) {
        case 1:
            DesyncAng += 25.0f;
            break;
        case 2:
            DesyncAng -= 25.0f;
            break;
        case 3:
            DesyncAng += 58.0f;
            break;
        case 4:
            DesyncAng -= 58.0f;
            break;
        case 5:
            DesyncAng = 70;
            break;
        case 6:
            DesyncAng = -70;
            break;
        case 7:
            DesyncAng = Animstate->footYaw;
            break;
        case 8:
            DesyncAng += -116.0f;
            break;
        case 9:
            DesyncAng += +116.0f;
            break;
        case 10:
            DesyncAng = DesyncAng + (entity->getMaxDesyncAngle() * -1);
            break;
        case 11:
            DesyncAng = DesyncAng + entity->getMaxDesyncAngle();
            break;
        case 12:
            DesyncAng = -15;
            break;
        case 13:
            DesyncAng = 15;
            break;
        default:
            DesyncAng = Animstate->footYaw;
            break;
        }

        entity->eyeAngles().y += DesyncAng;
        Animstate->footYaw += DesyncAng;
        entity->updateState(Animstate, entity->eyeAngles());
        record->PreviousEyeAngle = entity->eyeAngles().y;

        if (record->PreviousEyeAngle > 180) {
            record->PreviousEyeAngle -= 180;
        }
        else if (record->PreviousEyeAngle < -180) {
            record->PreviousEyeAngle += 180;
        }

        record->PreviousDesyncAng = DesyncAng;
        record->wasUpdated = true;
        record->FiredUpon = false;
	}
}