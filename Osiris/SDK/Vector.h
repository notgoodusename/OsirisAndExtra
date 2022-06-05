#pragma once

#include <cmath>

#include "../Helpers.h"
#include "Utils.h"

class matrix3x4;

struct Vector {
    constexpr auto notNull() const noexcept
    {
        return x || y || z;
    }

    constexpr auto null() const noexcept
    {
        return !notNull();
    }
    
    constexpr float operator[](int i) const noexcept
    {
        return ((float*)this)[i];
    }

    constexpr float& operator[](int i) noexcept
    {
        return ((float*)this)[i];
    }

    constexpr auto operator==(const Vector& v) const noexcept
    {
        return x == v.x && y == v.y && z == v.z;
    }

    constexpr auto operator!=(const Vector& v) const noexcept
    {
        return !(*this == v);
    }

    constexpr Vector& operator=(const float array[3]) noexcept
    {
        x = array[0];
        y = array[1];
        z = array[2];
        return *this;
    }

    constexpr Vector& operator+=(const Vector& v) noexcept
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    constexpr Vector& operator+=(float f) noexcept
    {
        x += f;
        y += f;
        z += f;
        return *this;
    }

    constexpr Vector& operator-=(const Vector& v) noexcept
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    constexpr Vector& operator-=(float f) noexcept
    {
        x -= f;
        y -= f;
        z -= f;
        return *this;
    }

    constexpr auto operator-(const Vector& v) const noexcept
    {
        return Vector{ x - v.x, y - v.y, z - v.z };
    }

    constexpr auto operator+(const Vector& v) const noexcept
    {
        return Vector{ x + v.x, y + v.y, z + v.z };
    }
    
    constexpr auto operator*(const Vector& v) const noexcept
    {
        return Vector{ x * v.x, y * v.y, z * v.z };
    }

    constexpr auto operator*=(float mul) noexcept
    {
        x *= mul;
        y *= mul;
        z *= mul;
        return *this;
    }

    constexpr Vector& operator/=(float div) noexcept
    {
        x /= div;
        y /= div;
        z /= div;
        return *this;
    }

    constexpr auto operator/(float div) const noexcept
    {
        return Vector{ x / div, y / div, z / div };
    }

    constexpr auto operator*(float mul) const noexcept
    {
        return Vector{ x * mul, y * mul, z * mul };
    }

    constexpr auto operator-(float sub) const noexcept
    {
        return Vector{ x - sub, y - sub, z - sub };
    }

    constexpr auto operator+(float add) const noexcept
    {
        return Vector{ x + add, y + add, z + add };
    }

    Vector normalized() noexcept
    {
        auto vectorNormalize = [](Vector& vector)
        {
            float radius = std::sqrtf(std::powf(vector.x, 2) + std::powf(vector.y, 2) + std::powf(vector.z, 2));
            radius = 1.f / (radius + FLT_EPSILON);

            vector *= radius;

            return radius;
        };
        Vector v = *this;
        vectorNormalize(v);
        return v;
    }

    Vector clamp() noexcept
    {
        this->x = std::clamp(this->x, -89.f, 89.f);
        this->y = std::clamp(this->y, -180.f, 180.f);
        this->z = std::clamp(this->z, -50.f, 50.f);
        return *this;
    }

    Vector& normalize() noexcept
    {
        x = std::isfinite(x) ? std::remainder(x, 360.0f) : 0.0f;
        y = std::isfinite(y) ? std::remainder(y, 360.0f) : 0.0f;
        z = 0.0f;
        return *this;
    }

    auto length() const noexcept
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    auto length2D() const noexcept
    {
        return std::sqrt(x * x + y * y);
    }

    constexpr auto squareLength() const noexcept
    {
        return x * x + y * y + z * z;
    }

    constexpr auto dotProduct(const Vector& v) const noexcept
    {
        return x * v.x + y * v.y + z * v.z;
    }

    constexpr auto dotProduct(const float* other) const noexcept
    {
        return x * other[0] + y * other[1] + z * other[2];
    }

    constexpr auto transform(const matrix3x4& mat) const noexcept;

    auto distTo(const Vector& v) const noexcept
    {
        return (*this - v).length();
    }

    auto toAngle() const noexcept
    {
        return Vector{ Helpers::rad2deg(std::atan2(-z, std::hypot(x, y))),
                       Helpers::rad2deg(std::atan2(y, x)),
                       0.0f };
    }

    auto snapTo4() const noexcept
    {
        const float l = length2D();
        bool xp = x >= 0.0f;
        bool yp = y >= 0.0f;
        bool xy = std::fabsf(x) >= std::fabsf(y);
        if (xp && xy)
            return Vector{ l, 0.0f, 0.0f };
        if (!xp && xy)
            return Vector{ -l, 0.0f, 0.0f };
        if (yp && !xy)
            return Vector{ 0.0f, l, 0.0f };
        if (!yp && !xy)
            return Vector{ 0.0f, -l, 0.0f };

        return Vector{};
    }

    static auto fromAngle(const Vector& angle) noexcept
    {
        return Vector{ std::cos(Helpers::deg2rad(angle.x)) * std::cos(Helpers::deg2rad(angle.y)),
                       std::cos(Helpers::deg2rad(angle.x)) * std::sin(Helpers::deg2rad(angle.y)),
                      -std::sin(Helpers::deg2rad(angle.x)) };
    }

    static auto fromAngle2D(float angle) noexcept
    {
        return Vector{ std::cos(Helpers::deg2rad(angle)),
                      std::sin(Helpers::deg2rad(angle)),
                      0.0f };
    }

    static auto fromAngle(const Vector& angle, Vector* out) noexcept
    {
        if (out)
        {
            out->x = std::cos(Helpers::deg2rad(angle.x)) * std::cos(Helpers::deg2rad(angle.y));
            out->y = std::cos(Helpers::deg2rad(angle.x)) * std::sin(Helpers::deg2rad(angle.y));
            out->z = -std::sin(Helpers::deg2rad(angle.x));
        }
    }

    static auto fromAngleAll(const Vector& angle, Vector* forward, Vector* right, Vector* up) noexcept
    {
        float sr = std::sin(Helpers::deg2rad(angle.z))
            , sp = std::sin(Helpers::deg2rad(angle.x))
            , sy = std::sin(Helpers::deg2rad(angle.y))
            , cr = std::cos(Helpers::deg2rad(angle.z))
            , cp = std::cos(Helpers::deg2rad(angle.x))
            , cy = std::cos(Helpers::deg2rad(angle.y));

        if (forward)
        {
            forward->x = cp * cy;
            forward->y = cp * sy;
            forward->z = -sp;
        }

        if (right)
        {
            right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
            right->y = (-1 * sr * sp * sy + -1 * cr * cy);
            right->z = -1 * sr * cp;
        }

        if (up)
        {
            up->x = (cr * sp * cy + -sr * -sy);
            up->y = (cr * sp * sy + -sr * cy);
            up->z = cr * cp;
        }
    }

    constexpr auto dotProduct2D(const Vector& v) const noexcept
    {
        return x * v.x + y * v.y;
    }

    constexpr auto crossProduct(const Vector& v) const noexcept
    {
        return Vector{ y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
    }

    void vectorCrossProduct(const Vector& a, const Vector& b, Vector& result) noexcept
    {
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
    }

    static void vectorSubtract(const Vector& a, const Vector& b, Vector& c) noexcept
    {
        c.x = a.x - b.x;
        c.y = a.y - b.y;
        c.z = a.z - b.z;
    }

    Vector cross(const Vector& other) noexcept
    {
        Vector v;
        vectorCrossProduct(*this, other, v);
        return v;
    }

    constexpr static auto up() noexcept
    {
        return Vector{ 0.0f, 0.0f, 1.0f };
    }

    constexpr static auto down() noexcept
    {
        return Vector{ 0.0f, 0.0f, -1.0f };
    }

    constexpr static auto forward() noexcept
    {
        return Vector{ 1.0f, 0.0f, 0.0f };
    }

    constexpr static auto back() noexcept
    {
        return Vector{ -1.0f, 0.0f, 0.0f };
    }

    constexpr static auto left() noexcept
    {
        return Vector{ 0.0f, 1.0f, 0.0f };
    }

    constexpr static auto right() noexcept
    {
        return Vector{ 0.0f, -1.0f, 0.0f };
    }

    float x, y, z;
};

#include "matrix3x4.h"

constexpr auto Vector::transform(const matrix3x4& mat) const noexcept
{
    return Vector{ dotProduct({ mat[0][0], mat[0][1], mat[0][2] }) + mat[0][3],
                   dotProduct({ mat[1][0], mat[1][1], mat[1][2] }) + mat[1][3],
                   dotProduct({ mat[2][0], mat[2][1], mat[2][2] }) + mat[2][3] };
}
