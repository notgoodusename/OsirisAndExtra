#include <cmath>
#include <cwctype>
#include <fstream>
#include <tuple>

#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include "Config.h"
#include "ConfigStructs.h"
#include "GameData.h"
#include "Helpers.h"
#include "Memory.h"

#include "SDK/GlobalVars.h"
#include "SDK/Engine.h"

std::array<float, 3U> Helpers::rgbToHsv(float r, float g, float b) noexcept
{
    r = std::clamp(r, 0.0f, 1.0f);
    g = std::clamp(g, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);
    const auto max = std::max({ r, g, b });
    const auto min = std::min({ r, g, b });
    const auto delta = max - min;

    float hue = 0.0f, sat = 0.0f;

    if (delta)
    {
        if (max == r)
            hue = std::fmodf((g - b) / delta, 6.0f) / 6.0f;
        else if (max == g)
            hue = ((b - r) / delta + 2.0f) / 6.0f;
        else if (max == b)
            hue = ((r - g) / delta + 4.0f) / 6.0f;
    }

    if (max)
        sat = delta / max;

    return { hue, sat, max };
}

std::array<float, 3U> Helpers::hsvToRgb(float h, float s, float v) noexcept
{
    h = h < 0.0f ? std::fmodf(h, 1.0f) + 1.0f : std::fmodf(h, 1.0f);
    s = std::clamp(s, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);
    const auto c = s * v;
    const auto x = c * (1.0f - std::fabsf(std::fmodf(h * 6.0f, 2.0f) - 1.0f));
    const auto m = v - c;

    float r = 0.0f, g = 0.0f, b = 0.0f;

    if (0.0f <= h && h < 1.0f / 6.0f)
        r = c, g = x;
    else if (1.0f / 6.0f <= h && h < 1.0f / 3.0f)
        r = x, g = c;
    else if (1.0f / 3.0f <= h && h < 0.5f)
        g = c, b = x;
    else if (0.5f <= h && h < 1.0f / 3.0f * 2.0f)
        g = x, b = c;
    else if (1.0f / 3.0f * 2.0f <= h && h < 1.0f / 6.0f * 5.0f)
        r = x, b = c;
    else if (1.0f / 6.0f * 5.0f <= h && h < 1.0f)
        r = c, b = x;

    return { r + m, g + m, b + m };
}

std::array<float, 3U> Helpers::rainbowColor(float speed) noexcept
{
    return hsvToRgb(speed * memory->globalVars->realtime * 0.1f, 1.0f, 1.0f);
}

std::array<float, 4U> Helpers::rainbowColor(float speed, float alpha) noexcept
{
    auto&& [r, g, b] = hsvToRgb(speed * memory->globalVars->realtime * 0.1f, 1.0f, 1.0f);
    return { r, g, b, alpha };
}

static auto rainbowColor(float time, float speed, float alpha) noexcept
{
    constexpr float pi = std::numbers::pi_v<float>;
    return std::array{ std::sin(speed * time) * 0.5f + 0.5f,
                       std::sin(speed * time + 2 * pi / 3) * 0.5f + 0.5f,
                       std::sin(speed * time + 4 * pi / 3) * 0.5f + 0.5f,
                       alpha };
}

void Helpers::logConsole(std::string_view msg, const std::array<std::uint8_t, 4> color) noexcept
{
    static constexpr int LS_MESSAGE = 0;

    static const auto channelId = memory->findLoggingChannel("Console");

    memory->logDirect(channelId, LS_MESSAGE, color, msg.data());
}

float Helpers::simpleSpline(float value) noexcept
{
    float valueSquared = value * value;

    return (3 * valueSquared - 2 * valueSquared * value);
}

float Helpers::simpleSplineRemapVal(float val, float A, float B, float C, float D) noexcept
{
    if (A == B)
        return val >= B ? D : C;
    float cVal = (val - A) / (B - A);
    return C + (D - C) * simpleSpline(cVal);
}

float Helpers::simpleSplineRemapValClamped(float val, float A, float B, float C, float D) noexcept
{
    if (A == B)
        return val >= B ? D : C;
    float cVal = (val - A) / (B - A);
    cVal = std::clamp(cVal, 0.0f, 1.0f);
    return C + (D - C) * simpleSpline(cVal);
}

Vector Helpers::lerp(float percent, Vector a, Vector b) noexcept
{
    return a + (b - a) * percent;
}

float Helpers::lerp(float percent, float a, float b) noexcept
{
    return a + (b - a) * percent;
}

float Helpers::bias(float x, float biasAmt) noexcept
{
    static float lastAmt = -1;
    static float lastExponent = 0;
    if (lastAmt != biasAmt)
    {
        lastExponent = log(biasAmt) * -1.4427f;
    }
    return pow(x, lastExponent);
}

float Helpers::smoothStepBounds(float edge0, float edge1, float x) noexcept
{
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return x * x * (3 - 2 * x);
}

float Helpers::clampCycle(float clycle) noexcept
{
    clycle -= (float)(int)clycle;

    if (clycle < 0.0f)
    {
        clycle += 1.0f;
    }
    else if (clycle > 1.0f)
    {
        clycle -= 1.0f;
    }

    return clycle;
}

float Helpers::approach(float target, float value, float speed) noexcept
{
    float delta = target - value;

    if (delta > speed)
        value += speed;
    else if (delta < -speed)
        value -= speed;
    else
        value = target;

    return value;
}

float Helpers::approachValueSmooth(float target, float value, float fraction) noexcept
{
    float delta = target - value;
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    delta *= fraction;
    return value + delta;
}

float Helpers::angleDiff(float destAngle, float srcAngle) noexcept
{
    float delta = std::fmodf(destAngle - srcAngle, 360.0f);

    if (destAngle > srcAngle) 
    {
        if (delta >= 180)
            delta -= 360;
    }
    else 
    {
        if (delta <= -180)
            delta += 360;
    }
    return delta;
}

Vector Helpers::approach(Vector target, Vector value, float speed) noexcept
{
    Vector diff = (target - value);

    float delta = diff.length();
    if (delta > speed)
        value += diff.normalized() * speed;
    else if (delta < -speed)
        value -= diff.normalized() * speed;
    else
        value = target;

    return value;
}

float Helpers::angleNormalize(float angle) noexcept
{
    angle = fmodf(angle, 360.0f);

    if (angle > 180.f)
        angle -= 360.f;

    if (angle < -180.f)
        angle += 360.f;

    return angle;
}

float Helpers::approachAngle(float target, float value, float speed) noexcept
{
    auto anglemod = [](float a)
    {
        a = (360.f / 65536) * ((int)(a * (65536.f / 360.0f)) & 65535);
        return a;
    };
    target = anglemod(target);
    value = anglemod(value);

    float delta = target - value;

    if (speed < 0)
        speed = -speed;

    if (delta < -180)
        delta += 360;
    else if (delta > 180)
        delta -= 360;

    if (delta > speed)
        value += speed;
    else if (delta < -speed)
        value -= speed;
    else
        value = target;

    return value;
}

float Helpers::remapValClamped(float val, float A, float B, float C, float D) noexcept
{
    if (A == B)
        return val >= B ? D : C;
    float cVal = (val - A) / (B - A);
    cVal = std::clamp(cVal, 0.0f, 1.0f);

    return C + (D - C) * cVal;
}

float Helpers::normalizeYaw(float yaw) noexcept
{
    if (!std::isfinite(yaw))
        return 0.0f;

    if (yaw >= -180.f && yaw <= 180.f)
        return yaw;

    const float rot = std::round(std::abs(yaw / 360.f));

    yaw = (yaw < 0.f) ? yaw + (360.f * rot) : yaw - (360.f * rot);
    return yaw;
}

bool Helpers::worldToScreen(const Vector& in, ImVec2& out, bool floor) noexcept
{
    const auto& matrix = GameData::toScreenMatrix();

    const auto w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;
    if (w < 0.001f)
        return false;

    out = ImGui::GetIO().DisplaySize / 2.0f;
    out.x *= 1.0f + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w;
    out.y *= 1.0f - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w;
    if (floor)
        out = ImFloor(out);
    return true;
}

static float alphaFactor = 1.0f;

unsigned int Helpers::calculateColor(Color4 color) noexcept
{
    color.color[3] *= (255.0f - GameData::local().flashDuration) / 255.0f;
    color.color[3] *= std::clamp(alphaFactor, 0.0f, 1.0f);
    auto&& [r, g, b, a] = color.rainbow ? rainbowColor(color.rainbowSpeed, color.color[3]) : color.color;
    return ImGui::ColorConvertFloat4ToU32({ r, g, b, a });
}

unsigned int Helpers::calculateColor(Color3 color) noexcept
{
    auto&& [r, g, b] = color.rainbow ? rainbowColor(color.rainbowSpeed) : color.color;
    return ImGui::ColorConvertFloat4ToU32({ r, g, b, 1.0f });
}

unsigned int Helpers::calculateColor(int r, int g, int b, int a) noexcept
{
    a -= static_cast<int>(a * GameData::local().flashDuration / 255.0f);
    return IM_COL32(r, g, b, a * alphaFactor);
}

void Helpers::setAlphaFactor(float newAlphaFactor) noexcept
{
    alphaFactor = newAlphaFactor;
}

float Helpers::getAlphaFactor() noexcept
{
    return alphaFactor;
}

void Helpers::convertHSVtoRGB(float h, float s, float v, float& outR, float& outG, float& outB) noexcept
{
    ImGui::ColorConvertHSVtoRGB(h, s, v, outR, outG, outB);
}

void Helpers::healthColor(float fraction, float& outR, float& outG, float& outB) noexcept
{
    constexpr auto greenHue = 1.0f / 3.0f;
    constexpr auto redHue = 0.0f;
    convertHSVtoRGB(std::lerp(redHue, greenHue, fraction), 1.0f, 1.0f, outR, outG, outB);
}

unsigned int Helpers::healthColor(float fraction) noexcept
{
    float r, g, b;
    healthColor(fraction, r, g, b);
    return calculateColor(static_cast<int>(r * 255.0f), static_cast<int>(g * 255.0f), static_cast<int>(b * 255.0f), 255);
}

ImWchar* Helpers::getFontGlyphRanges() noexcept
{
    static ImVector<ImWchar> ranges;
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        constexpr ImWchar baseRanges[]{
            0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
            0x0300, 0x03FF, // Combining Diacritical Marks + Greek/Coptic
            0x0600, 0x06FF, // Arabic
            0x0E00, 0x0E7F, // Thai
            0
        };
        builder.AddRanges(baseRanges);
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
        builder.AddText("\u9F8D\u738B\u2122");
        builder.BuildRanges(&ranges);
    }
    return ranges.Data;
}

std::wstring Helpers::toWideString(const std::string& str) noexcept
{
    std::wstring upperCase(str.length(), L'\0');
    if (const auto newLen = std::mbstowcs(upperCase.data(), str.c_str(), upperCase.length()); newLen != static_cast<std::size_t>(-1))
        upperCase.resize(newLen);
    return upperCase;
}

std::wstring Helpers::toUpper(std::wstring str) noexcept
{
    std::transform(str.begin(), str.end(), str.begin(), [](wchar_t w) -> wchar_t {
        if (w >= 0 && w <= 127) {
            if (w >= 'a' && w <= 'z')
                return w - ('a' - 'A');
            return w;
        }

        return std::towupper(w);
    });
    return str;
}

bool Helpers::decodeVFONT(std::vector<char>& buffer) noexcept
{
    constexpr std::string_view tag = "VFONT1";
    unsigned char magic = 0xA7;

    if (buffer.size() <= tag.length())
        return false;

    const auto tagIndex = buffer.size() - tag.length();
    if (std::memcmp(tag.data(), &buffer[tagIndex], tag.length()))
        return false;

    unsigned char saltBytes = buffer[tagIndex - 1];
    const auto saltIndex = tagIndex - saltBytes;
    --saltBytes;

    for (std::size_t i = 0; i < saltBytes; ++i)
        magic ^= (buffer[saltIndex + i] + 0xA7) % 0x100;

    for (std::size_t i = 0; i < saltIndex; ++i) {
        unsigned char xored = buffer[i] ^ magic;
        magic = (buffer[i] + 0xA7) % 0x100;
        buffer[i] = xored;
    }

    buffer.resize(saltIndex);
    return true;
}

std::vector<char> Helpers::loadBinaryFile(const std::string& path) noexcept
{
    std::vector<char> result;
    std::ifstream in{ path, std::ios::binary };
    if (!in)
        return result;
    in.seekg(0, std::ios_base::end);
    result.resize(static_cast<std::size_t>(in.tellg()));
    in.seekg(0, std::ios_base::beg);
    in.read(result.data(), result.size());
    return result;
}
