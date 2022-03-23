#pragma once
#include <string>

#include "../Netvars.h"
#include "VirtualMethod.h"

struct Vector;

class IPlayerResource {
public:
    VIRTUAL_METHOD_V(bool, isAlive, 5, (int index), (this, index))
    VIRTUAL_METHOD_V(const char*, getPlayerName, 8, (int index), (this, index))
    VIRTUAL_METHOD_V(int, getPlayerHealth, 14, (int index), (this, index))
};

class PlayerResource {
public:
    auto getIPlayerResource() noexcept
    {
        return reinterpret_cast<IPlayerResource*>(uintptr_t(this) + 0x9D8);
    }

    NETVAR(bombsiteCenterA, "CCSPlayerResource", "m_bombsiteCenterA", Vector)
    NETVAR(bombsiteCenterB, "CCSPlayerResource", "m_bombsiteCenterB", Vector)
    NETVAR(armor, "CCSPlayerResource", "m_iArmor", int[65])
    NETVAR(competitiveRanking, "CCSPlayerResource", "m_iCompetitiveRanking", int[65])
    NETVAR(competitiveWins, "CCSPlayerResource", "m_iCompetitiveWins", int[65])
    NETVAR(kills, "CCSPlayerResource", "m_iKills", int[65])
    NETVAR(assists, "CCSPlayerResource", "m_iAssists", int[65])
    NETVAR(deaths, "CCSPlayerResource", "m_iDeaths", int[65])
    NETVAR(ping, "CCSPlayerResource", "m_iPing", int[65])
    NETVAR(clanTag, "CCSPlayerResource", "m_szClan", std::string[65])
};
