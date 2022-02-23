#include "Resolver.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/GameEvent.h"
#include "../Interfaces.h"

namespace Resolver
{
    bool universalhurt = false;
    typedef void(__cdecl* MsgFn)(const char* msg, va_list);
    void Msg(const char* msg, ...)
    {
        if (msg == nullptr)
            return; //If no string was passed, or it was null then don't do anything
        static MsgFn fn = (MsgFn)GetProcAddress(GetModuleHandleA("tier0.dll"), "Msg"); //This gets the address of export "Msg" in the dll "tier0.dll". The static keyword means it's only called once and then isn't called again (but the variable is still there)
        char buffer[989];
        va_list list;
        va_start(list, msg);
        vsprintf(buffer, msg, list);
        va_end(list);
        fn(buffer, list);
    }
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
                    universalhurt = false;
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
                Msg("Missed: %d", record->missedshots);
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
        float eye_feet = (entity->eyeAngles().y - animstate->footYaw);
        record->FiredUpon = universalhurt;
        int missed = record->missedshots;
        if (record->lastworkingshot != -1)
            missed = record->lastworkingshot;

        switch (missed % 9) {
        case 1:
            if (eye_feet == 0)
            {
                desyncAng = animstate->footYaw;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc11\n");
                goto A;
                
            }
            else if (eye_feet > 0.f)
            {
                desyncAng = animstate->footYaw - entity->getMaxDesyncAngle();
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc12\n");
                goto A;
                
            }
            else
            {
                desyncAng = animstate->footYaw + entity->getMaxDesyncAngle();
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc13\n");
                goto A;
            }
            A:
            break;
        case 2:
            if (eye_feet == 0)
            {
                desyncAng = animstate->footYaw;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc21\n");
                goto B;
            }
            else if (eye_feet > 0.f)
            {
                desyncAng = animstate->footYaw - (entity->getMaxDesyncAngle() * 2)-2;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc22\n");
                goto B;
            }
            else
            {
                desyncAng = animstate->footYaw + (entity->getMaxDesyncAngle() * 2)-2;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc23\n");
                goto B;
            }
            B:
            break;
        case 3:
            if (eye_feet == 0)
            {
                desyncAng = animstate->footYaw;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc31\n");
                goto C;
            }
            else if (eye_feet > 0.f)
            {
                desyncAng = animstate->footYaw - (entity->getMaxDesyncAngle() / 2) - 2;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc32\n");
                goto C;
            }
            else
            {
                desyncAng = animstate->footYaw + (entity->getMaxDesyncAngle() / 2) - 2;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missc33\n");
                goto C;
            }
            C:
            break;
        default:
            if (eye_feet == 0)
            {
                desyncAng = animstate->footYaw;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missDF1\n");
                goto D;
            }
            else if (eye_feet > 0.f)
            {
                desyncAng = animstate->footYaw - 60;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missDF2\n");
                goto D;
            }
            else
            {
                desyncAng = animstate->footYaw + 60;
                eye_feet = (entity->eyeAngles().y - animstate->footYaw);
                Msg("missDF3\n");
                goto D;
            }
            D:
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
    
    
    void hurt(GameEvent& event) noexcept
    {
        if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId && event.getInt("userid") == localUserId)
        {
           // universalhurt = true; CHANGEME
            //Msg("Ouch: %d", 8);
            Msg("Ouch\n");
        }
       
    }
}