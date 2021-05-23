#pragma once
#include "Vector.h"
#include "Utils.h"

class Angle
{
    float sp{ 0.0f }, cp{ 0.0f };
    float sy{ 0.0f }, cy{ 0.0f };
    float sr{ 0.0f }, cr{ 0.0f };

    constexpr Vector get(unsigned int i) noexcept
    {
        switch (i)
        {
        case 0:
            return { (cp * cy), (cp * sy), (-sp) };
            break;
        case 1:
            return { -1 * sr * sp * cy + -1 * cr * -sy, -1 * sr * sp * sy + -1 * cr * cy, -1 * sr * cp };
            break;
        case 2:
            return { cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp };
            break;
        default:
            return { };
            break;
        }
    }

public:
    Vector forward{ };
    Vector right{ };
    Vector up{ };

    Angle(const Vector& v) noexcept :sp{ sinf(Helpers::deg2rad(v.x)) }, cp{ cosf(Helpers::deg2rad(v.x)) }, sy{ sinf(Helpers::deg2rad(v.y)) }, cy{ cosf(Helpers::deg2rad(v.y)) }, sr{ sinf(Helpers::deg2rad(v.z)) }, cr{ cosf(Helpers::deg2rad(v.z)) },
        forward{ get(0) }, right{ get(1) }, up{ get(2) } { }
};