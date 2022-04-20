#pragma once

#include "Server.h"

enum class ClassId {
    BaseCSGrenadeProjectile = 9,
    BreachChargeProjectile = 29,
    BumpMineProjectile = 33,
    C4,
    Chicken = 36,
    CSPlayer = 40,
    CSPlayerResource,
    CSRagdoll = 42,
    Deagle = 46,
    DecoyProjectile = 48,
    Drone,
    Dronegun,
    DynamicProp = 52,
    EconEntity = 53,
    EconWearable,
    Hostage = 97,
    Inferno = 100,
    Healthshot = 104,
    Cash,
    Knife = 107,
    KnifeGG,
    MolotovProjectile = 114,
    AmmoBox = 125,
    LootCrate,
    RadarJammer,
    WeaponUpgrade,
    PlantedC4,
    PropDoorRotating = 143,
    SensorGrenadeProjectile = 153,
    SmokeGrenadeProjectile = 157,
    SnowballPile = 160,
    SnowballProjectile,
    Tablet = 172,
    Aug = 232,
    Awp,
    Elite = 239,
    FiveSeven = 241,
    G3sg1,
    Glock = 245,
    P2000,
    P250 = 258,
    Scar20 = 261,
    Sg553 = 265,
    Ssg08 = 267,
    Tec9 = 269
};

class ClassIdManager
{
private:
    int getClassID(const char* classname) noexcept
    {
        ServerClass* serverclass = interfaces->server->getAllServerClasses();
        int id = 0;
        while (serverclass)
        {
            if (!strcmp(serverclass->networkName, classname))
                return id;
            serverclass = serverclass->next, id++;
        }
        return -1;
    }
public:
    ClassIdManager() noexcept
    {
        fogController = getClassID("CFogController");
    }

    int fogController;
};

inline std::unique_ptr<const ClassIdManager> dynamicClassId;