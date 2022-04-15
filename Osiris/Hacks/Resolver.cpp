#include "Resolver.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/GameEvent.h"
#include "../Interfaces.h"
#include "AntiAim.h"
#include <vector>

namespace Resolver
{
    bool universalhurt = false;
    int shots = 0;
    float lastFireTime = -1.f;
    float tickHitWall = .0f;
    float tickHitPlayer = .0f;
    int missed = 0;
    float missedang = 0.f;
    std::vector<float> blacklisted;
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

            if (missed>0 && record->prevhealth == entity->health()) {

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
    float desyncAng = 0;
	void basic(Entity* entity) noexcept
	{
        auto record = &records.at(entity->index());

        if (!record || record->invalid)
            return;

        if (entity->velocity().length2D() > 3.0f) {
            record->PreviousEyeAngle = entity->eyeAngles().y;
            return;
        }       

        desyncAng = 0;

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
        int tickcount = 0;
        float desyncSide = 2 * eye_feet <= 0.0f ? 1 : -1;
        //resolver code
        

        
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
        if (!localPlayer || !localPlayer->isAlive())
            return;
        const auto localUserId = localPlayer->getUserId();
        if (event.getInt("attacker") != localUserId && event.getInt("userid") == localUserId)
        {
            if (config->fakeAngle.enabled && localPlayer->isAlive()){ AntiAim::switchOnHurt(); }

             universalhurt = true;
            Msg("Damage taken\n");

        }
        else if (event.getInt("attacker") == localUserId && event.getInt("userid") != localUserId)
        {
            int tickcount = memory->globalVars->tickCount;
            if (tickcount != tickHitPlayer)
            {
                tickHitPlayer = memory->globalVars->tickCount;
                if (event.getInt("hitgroup") == HitGroup::Head)
                {
                    missed = 0;
                    blacklisted = { -1.f };
                    Msg("Headshot\n");
                }
                else 
                {
                    Msg("baim\n");
                }

            }
         
        }
       
    }
   
    void thru(GameEvent& event) noexcept //bullet_impact
    {
        if (!localPlayer || !localPlayer->isAlive())
            return;
        const auto localUserIdz = localPlayer->getUserId();
        if (event.getInt("userid") == localUserIdz)
        {
            int tickcount = memory->globalVars->tickCount;
            if (tickcount != tickHitPlayer && tickcount != tickHitWall)
            {
                tickHitWall = tickcount;
                missed++;
                missedang = desyncAng;
                if (std::count(blacklisted.begin(), blacklisted.end(), missedang)) {
                    Msg("Missed due to lack of resolver\n");
                }
                else 
                {
                    Msg("Resolver Updated\n");
                    blacklisted.push_back(missedang);
                }
               
            }
        }

    }
    void shott(GameEvent& event) noexcept //weapon_fire
    {
        if (!localPlayer || !localPlayer->isAlive())
            return;
        const auto localUserIds = localPlayer->getUserId();
        if (event.getInt("userid") == localUserIds)
        {
            shots++;
            lastFireTime = memory->globalVars->serverTime();
            
        }

    }
}