#pragma once

#include <concepts>
#include <mutex>
#include <array>
#include <numbers>
#include <random>
#include <string>
#include <vector>

#include "imgui/imgui.h"
#include "Config.h"

#include "SDK/WeaponId.h"

struct Color4;
struct Vector;

namespace Helpers
{
    float simpleSpline(float value) noexcept;
    float simpleSplineRemapVal(float val, float A, float B, float C, float D) noexcept;
    float simpleSplineRemapValClamped(float val, float A, float B, float C, float D) noexcept;
    float lerp(float percent, float a, float b) noexcept;
    float bias(float x, float biasAmt) noexcept;
    float smoothStepBounds(float edge0, float edge1, float x) noexcept;
    float clampCycle(float clycle) noexcept;
    float approach(float target, float value, float speed) noexcept;
    float approachValueSmooth(float target, float value, float fraction) noexcept;
    float angleDiff(float destAngle, float srcAngle) noexcept;
    Vector approach(Vector target, Vector value, float speed) noexcept;
    float angleNormalize(float angle) noexcept;
    float approachAngle(float target, float value, float speed) noexcept;
    float remapValClamped(float val, float A, float B, float C, float D) noexcept;
    float normalizeYaw(float yaw) noexcept;

    unsigned int calculateColor(Color4 color) noexcept;
    unsigned int calculateColor(Color3 color) noexcept;
    unsigned int calculateColor(int r, int g, int b, int a) noexcept;
    void setAlphaFactor(float newAlphaFactor) noexcept;
    float getAlphaFactor() noexcept;
    void convertHSVtoRGB(float h, float s, float v, float& outR, float& outG, float& outB) noexcept;
    void healthColor(float fraction, float& outR, float& outG, float& outB) noexcept;
    unsigned int healthColor(float fraction) noexcept;

    constexpr auto units2meters(float units) noexcept
    {
        return units * 0.0254f;
    }

    ImWchar* getFontGlyphRanges() noexcept;

    constexpr int utf8SeqLen(char firstByte) noexcept
    {
        return (firstByte & 0x80) == 0x00 ? 1 :
               (firstByte & 0xE0) == 0xC0 ? 2 :
               (firstByte & 0xF0) == 0xE0 ? 3 :
               (firstByte & 0xF8) == 0xF0 ? 4 :
               -1;
    }

    constexpr auto utf8Substr(char* start, char* end, int n) noexcept
    {
        while (start < end && --n)
            start += utf8SeqLen(*start);
        return start;
    }

    std::wstring toWideString(const std::string& str) noexcept;
    std::wstring toUpper(std::wstring str) noexcept;

    bool decodeVFONT(std::vector<char>& buffer) noexcept;
    std::vector<char> loadBinaryFile(const std::string& path) noexcept;

    constexpr auto deg2rad(float degrees) noexcept { return degrees * (std::numbers::pi_v<float> / 180.0f); }
    constexpr auto rad2deg(float radians) noexcept { return radians * (180.0f / std::numbers::pi_v<float>); }

    constexpr auto isKnife(WeaponId id) noexcept
    {
        return (id >= WeaponId::Bayonet && id <= WeaponId::SkeletonKnife) || id == WeaponId::KnifeT || id == WeaponId::Knife;
    }

    constexpr auto isSouvenirToken(WeaponId id) noexcept
    {
        switch (id) {
        case WeaponId::Berlin2019SouvenirToken:
        case WeaponId::Stockholm2021SouvenirToken:
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]] constexpr auto isMP5LabRats(WeaponId weaponID, int paintKit) noexcept
    {
        return weaponID == WeaponId::Mp5sd && paintKit == 800;
    }

    class RandomGenerator {
    public:
        template <std::integral T>
        [[nodiscard]] static T random(T min, T max) noexcept
        {
            std::scoped_lock lock{ mutex };
            return std::uniform_int_distribution{ min, max }(gen);
        }

        template <std::floating_point T>
        [[nodiscard]] static T random(T min, T max) noexcept
        {
            std::scoped_lock lock{ mutex };
            return std::uniform_real_distribution{ min, max }(gen);
        }

        template <typename T>
        [[nodiscard]] static std::enable_if_t<std::is_enum_v<T>, T> random(T min, T max) noexcept
        {
            return static_cast<T>(random(static_cast<std::underlying_type_t<T>>(min), static_cast<std::underlying_type_t<T>>(max)));
        }
    private:
        inline static std::mt19937 gen{ std::random_device{}() };
        inline static std::mutex mutex;
    };

    template <typename T>
    [[nodiscard]] T random(T min, T max) noexcept
    {
        return RandomGenerator::random(min, max);
    }
}
