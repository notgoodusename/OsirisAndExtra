#include "EngineTrace.h"

float HitGroup::getDamageMultiplier(int hitGroup, const WeaponInfo* weaponData, bool hasHeavyArmor, int teamNumber) noexcept
{
    static auto mp_damage_scale_ct_head = interfaces->cvar->findVar("mp_damage_scale_ct_head");
    static auto mp_damage_scale_t_head = interfaces->cvar->findVar("mp_damage_scale_t_head");

    static auto mp_damage_scale_ct_body = interfaces->cvar->findVar("mp_damage_scale_ct_body");
    static auto mp_damage_scale_t_body = interfaces->cvar->findVar("mp_damage_scale_t_body");

    auto headScale = teamNumber == 3 ? mp_damage_scale_ct_head->getFloat() : mp_damage_scale_t_head->getFloat();
    const auto bodyScale = teamNumber == 3 ? mp_damage_scale_ct_body->getFloat() : mp_damage_scale_t_body->getFloat();

    if (hasHeavyArmor)
        headScale *= 0.5f;

    switch (hitGroup) {
    case Head:
        return weaponData->headshotMultiplier * headScale;
    case Stomach:
        return 1.25f * bodyScale;
    case LeftLeg:
    case RightLeg:
        return 0.75f * bodyScale;
    default:
        return bodyScale;
    }
}

bool HitGroup::isArmored(int hitGroup, bool helmet, int armorValue, bool hasHeavyArmor) noexcept
{
    if (armorValue <= 0)
        return false;

    switch (hitGroup) {
    case Head:
        return helmet || hasHeavyArmor;

    case Chest:
    case Stomach:
    case LeftArm:
    case RightArm:
        return true;
    default:
        return hasHeavyArmor;
    }
}