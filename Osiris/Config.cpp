#include <fstream>
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj.h>

#include "nlohmann/json.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Config.h"
#include "Helpers.h"

#include "Hacks/AntiAim.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Glow.h"
#include "Hacks/Sound.h"

#include "SDK/Platform.h"

int CALLBACK fontCallback(const LOGFONTW* lpelfe, const TEXTMETRICW*, DWORD, LPARAM lParam)
{
    const wchar_t* const fontName = reinterpret_cast<const ENUMLOGFONTEXW*>(lpelfe)->elfFullName;

    if (fontName[0] == L'@')
        return TRUE;

    if (HFONT font = CreateFontW(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName)) {

        DWORD fontData = GDI_ERROR;

        if (HDC hdc = CreateCompatibleDC(nullptr)) {
            SelectObject(hdc, font);
            // Do not use TTC fonts as we only support TTF fonts
            fontData = GetFontData(hdc, 'fctt', 0, NULL, 0);
            DeleteDC(hdc);
        }
        DeleteObject(font);

        if (fontData == GDI_ERROR) {
            if (char buff[1024]; WideCharToMultiByte(CP_UTF8, 0, fontName, -1, buff, sizeof(buff), nullptr, nullptr))
                reinterpret_cast<std::vector<std::string>*>(lParam)->emplace_back(buff);
        }
    }
    return TRUE;
}

Config::Config() noexcept
{
    if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments))) {
        path = pathToDocuments;
        CoTaskMemFree(pathToDocuments);
    }

    path /= "Osiris";
    listConfigs();
    misc.clanTag[0] = '\0';
    misc.name[0] = '\0';
    visuals.playerModel[0] = '\0';

    load(u8"default.json", false);

    LOGFONTW logfont;
    logfont.lfCharSet = ANSI_CHARSET;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    logfont.lfFaceName[0] = L'\0';

    EnumFontFamiliesExW(GetDC(nullptr), &logfont, fontCallback, (LPARAM)&systemFonts, 0);

    std::sort(std::next(systemFonts.begin()), systemFonts.end());
}
#pragma region  Read

static void from_json(const json& j, ColorToggle& ct)
{
    from_json(j, static_cast<Color4&>(ct));
    read(j, "Enabled", ct.enabled);
}

static void from_json(const json& j, Color3& c)
{
    read(j, "Color", c.color);
    read(j, "Rainbow", c.rainbow);
    read(j, "Rainbow Speed", c.rainbowSpeed);
}

static void from_json(const json& j, ColorToggle3& ct)
{
    from_json(j, static_cast<Color3&>(ct));
    read(j, "Enabled", ct.enabled);
}

static void from_json(const json& j, ColorToggleRounding& ctr)
{
    from_json(j, static_cast<ColorToggle&>(ctr));

    read(j, "Rounding", ctr.rounding);
}

static void from_json(const json& j, ColorToggleOutline& cto)
{
    from_json(j, static_cast<ColorToggle&>(cto));

    read(j, "Outline", cto.outline);
}

static void from_json(const json& j, ColorToggleThickness& ctt)
{
    from_json(j, static_cast<ColorToggle&>(ctt));

    read(j, "Thickness", ctt.thickness);
}

static void from_json(const json& j, ColorToggleThicknessRounding& cttr)
{
    from_json(j, static_cast<ColorToggleRounding&>(cttr));

    read(j, "Thickness", cttr.thickness);
}

static void from_json(const json& j, Font& f)
{
    read<value_t::string>(j, "Name", f.name);

    if (!f.name.empty())
        config->scheduleFontLoad(f.name);

    if (const auto it = std::find_if(config->getSystemFonts().begin(), config->getSystemFonts().end(), [&f](const auto& e) { return e == f.name; }); it != config->getSystemFonts().end())
        f.index = std::distance(config->getSystemFonts().begin(), it);
    else
        f.index = 0;
}

static void from_json(const json& j, Snapline& s)
{
    from_json(j, static_cast<ColorToggleThickness&>(s));

    read(j, "Type", s.type);
}

static void from_json(const json& j, Box& b)
{
    from_json(j, static_cast<ColorToggleRounding&>(b));

    read(j, "Type", b.type);
    read(j, "Scale", b.scale);
    read<value_t::object>(j, "Fill", b.fill);
}

static void from_json(const json& j, Shared& s)
{
    read(j, "Enabled", s.enabled);
    read<value_t::object>(j, "Font", s.font);
    read<value_t::object>(j, "Snapline", s.snapline);
    read<value_t::object>(j, "Box", s.box);
    read<value_t::object>(j, "Name", s.name);
    read(j, "Text Cull Distance", s.textCullDistance);
}

static void from_json(const json& j, Weapon& w)
{
    from_json(j, static_cast<Shared&>(w));

    read<value_t::object>(j, "Ammo", w.ammo);
}

static void from_json(const json& j, Trail& t)
{
    from_json(j, static_cast<ColorToggleThickness&>(t));

    read(j, "Type", t.type);
    read(j, "Time", t.time);
}

static void from_json(const json& j, Trails& t)
{
    read(j, "Enabled", t.enabled);
    read<value_t::object>(j, "Local Player", t.localPlayer);
    read<value_t::object>(j, "Allies", t.allies);
    read<value_t::object>(j, "Enemies", t.enemies);
}

static void from_json(const json& j, Projectile& p)
{
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Trails", p.trails);
}

static void from_json(const json& j, HealthBar& o)
{
    from_json(j, static_cast<ColorToggle&>(o));
    read(j, "Type", o.type);
}

static void from_json(const json& j, Player& p)
{
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Weapon", p.weapon);
    read<value_t::object>(j, "Flash Duration", p.flashDuration);
    read(j, "Audible Only", p.audibleOnly);
    read(j, "Spotted Only", p.spottedOnly);
    read<value_t::object>(j, "Health Bar", p.healthBar);
    read<value_t::object>(j, "Skeleton", p.skeleton);
    read<value_t::object>(j, "Head Box", p.headBox);
    read<value_t::object>(j, "Line of sight", p.lineOfSight);
}

static void from_json(const json& j, OffscreenEnemies& o)
{
    from_json(j, static_cast<ColorToggle&>(o));

    read<value_t::object>(j, "Health Bar", o.healthBar);
}

static void from_json(const json& j, BulletTracers& o)
{
    from_json(j, static_cast<ColorToggle&>(o));
}

static void from_json(const json& j, ImVec2& v)
{
    read(j, "X", v.x);
    read(j, "Y", v.y);
}

static void from_json(const json& j, Config::Legitbot& a)
{
    read(j, "Enabled", a.enabled);
    read(j, "Aimlock", a.aimlock);
    read(j, "Silent", a.silent);
    read(j, "Friendly fire", a.friendlyFire);
    read(j, "Visible only", a.visibleOnly);
    read(j, "Scoped only", a.scopedOnly);
    read(j, "Ignore flash", a.ignoreFlash);
    read(j, "Ignore smoke", a.ignoreSmoke);
    read(j, "Auto scope", a.autoScope);
    read(j, "Fov", a.fov);
    read(j, "Smooth", a.smooth);
    read(j, "Reaction time", a.reactionTime);
    read(j, "Hitboxes", a.hitboxes);
    read(j, "Min damage", a.minDamage);
    read(j, "Killshot", a.killshot);
    read(j, "Between shots", a.betweenShots);
}

static void from_json(const json& j, Config::RecoilControlSystem& r)
{
    read(j, "Enabled", r.enabled);
    read(j, "Silent", r.silent);
    read(j, "Ignore Shots", r.shotsFired);
    read(j, "Horizontal", r.horizontal);
    read(j, "Vertical", r.vertical);
}

static void from_json(const json& j, Config::Ragebot& r)
{
    read(j, "Enabled", r.enabled);
    read(j, "Aimlock", r.aimlock);
    read(j, "Silent", r.silent);
    read(j, "Friendly fire", r.friendlyFire);
    read(j, "Visible only", r.visibleOnly);
    read(j, "Scoped only", r.scopedOnly);
    read(j, "Ignore flash", r.ignoreFlash);
    read(j, "Ignore smoke", r.ignoreSmoke);
    read(j, "Auto shot", r.autoShot);
    read(j, "Auto scope", r.autoScope);
    read(j, "Auto stop", r.autoStop);
    read(j, "Between shots", r.betweenShots);
    read(j, "Disable multipoint if low fps", r.disableMultipointIfLowFPS);
    read(j, "Disable backtrack if low fps", r.disableBacktrackIfLowFPS);
    read(j, "Priority", r.priority);
    read(j, "Fov", r.fov);
    read(j, "Hitboxes", r.hitboxes);
    read(j, "Hitchance", r.hitChance);
    read(j, "Multipoint", r.multiPoint);
    read(j, "Min damage", r.minDamage);
    read(j, "Min damage override", r.minDamageOverride);
}

static void from_json(const json& j, Config::Triggerbot& t)
{
    read(j, "Enabled", t.enabled);
    read(j, "Friendly fire", t.friendlyFire);
    read(j, "Scoped only", t.scopedOnly);
    read(j, "Ignore flash", t.ignoreFlash);
    read(j, "Ignore smoke", t.ignoreSmoke);
    read(j, "Hitboxes", t.hitboxes);
    read(j, "Hitchance", t.hitChance);
    read(j, "Shot delay", t.shotDelay);
    read(j, "Min damage", t.minDamage);
    read(j, "Killshot", t.killshot);
}

static void from_json(const json& j, Config::LegitAntiAimConfig& a)
{
    read(j, "Enabled", a.enabled);
    read(j, "Extend", a.extend);
    read(j, "Invert key", a.invert);
}

static void from_json(const json& j, Config::RageAntiAimConfig& a)
{
    read(j, "Enabled", a.enabled);
    read(j, "Pitch", a.pitch);
    read(j, "Yaw base", reinterpret_cast<int&>(a.yawBase));
    read(j, "Manual forward Key", a.manualForward);
    read(j, "Manual backward Key", a.manualBackward);
    read(j, "Manual right Key", a.manualRight);
    read(j, "Manual left Key", a.manualLeft);
    read(j, "Yaw modifier", a.yawModifier);
    read(j, "Yaw add", a.yawAdd);
    read(j, "Jitter Range", a.jitterRange);
    read(j, "Spin base", a.spinBase);
    read(j, "At targets", a.atTargets);
}

static void from_json(const json& j, Config::FakeAngle& a)
{
    read(j, "Enabled", a.enabled);
    read(j, "Invert", a.invert);
    read(j, "Left limit", a.leftLimit);
    read(j, "Right limit", a.rightLimit);
    read(j, "Peek mode", a.peekMode);
    read(j, "Lby mode", a.lbyMode);
}

static void from_json(const json& j, Config::Fakelag& f)
{
    read(j, "Enabled", f.enabled);
    read(j, "Mode", f.mode);
    read(j, "Limit", f.limit);
}

static void from_json(const json& j, Config::Tickbase& t)
{
    read(j, "Doubletap", t.doubletap);
    read(j, "Hideshots", t.hideshots);
    read(j, "Teleport", t.teleport);
}

static void from_json(const json& j, Config::Backtrack& b)
{
    read(j, "Enabled", b.enabled);
    read(j, "Ignore smoke", b.ignoreSmoke);
    read(j, "Ignore flash", b.ignoreFlash);
    read(j, "Time limit", b.timeLimit);
    read(j, "Fake Latency", b.fakeLatency);
    read(j, "Fake Latency Amount", b.fakeLatencyAmount);
}

static void from_json(const json& j, Config::Chams::Material& m)
{
    from_json(j, static_cast<Color4&>(m));

    read(j, "Enabled", m.enabled);
    read(j, "Health based", m.healthBased);
    read(j, "Blinking", m.blinking);
    read(j, "Wireframe", m.wireframe);
    read(j, "Cover", m.cover);
    read(j, "Ignore-Z", m.ignorez);
    read(j, "Material", m.material);
}

static void from_json(const json& j, Config::Chams& c)
{
    read_array_opt(j, "Materials", c.materials);
}

static void from_json(const json& j, Config::GlowItem& g)
{
    from_json(j, static_cast<Color4&>(g));

    read(j, "Enabled", g.enabled);
    read(j, "Health based", g.healthBased);
    read(j, "Style", g.style);
}

static void from_json(const json& j, Config::PlayerGlow& g)
{
    read<value_t::object>(j, "All", g.all);
    read<value_t::object>(j, "Visible", g.visible);
    read<value_t::object>(j, "Occluded", g.occluded);
}

static void from_json(const json& j, Config::StreamProofESP& e)
{
    read(j, "Key", e.key);
    read(j, "Allies", e.allies);
    read(j, "Enemies", e.enemies);
    read(j, "Weapons", e.weapons);
    read(j, "Projectiles", e.projectiles);
    read(j, "Loot Crates", e.lootCrates);
    read(j, "Other Entities", e.otherEntities);
}

static void from_json(const json& j, Config::Visuals::FootstepESP& ft)
{
    read<value_t::object>(j, "Enabled", ft.footstepBeams);
    read(j, "Thickness", ft.footstepBeamThickness);
    read(j, "Radius", ft.footstepBeamRadius);
}

static void from_json(const json& j, Config::Visuals& v)
{
    read(j, "Disable post-processing", v.disablePostProcessing);
    read(j, "Inverse ragdoll gravity", v.inverseRagdollGravity);
    read(j, "No fog", v.noFog);
    read<value_t::object>(j, "Fog controller", v.fog);
    read<value_t::object>(j, "Fog options", v.fogOptions);
    read(j, "No 3d sky", v.no3dSky);
    read(j, "No aim punch", v.noAimPunch);
    read(j, "No view punch", v.noViewPunch);
    read(j, "No view bob", v.noViewBob);
    read(j, "No hands", v.noHands);
    read(j, "No sleeves", v.noSleeves);
    read(j, "No weapons", v.noWeapons);
    read(j, "No smoke", v.noSmoke);
    read(j, "Smoke circle", v.smokeCircle);
    read(j, "Wireframe smoke", v.wireframeSmoke);
    read(j, "No molotov", v.noMolotov);
    read(j, "Wireframe molotov", v.wireframeMolotov);
    read(j, "No blur", v.noBlur);
    read(j, "No scope overlay", v.noScopeOverlay);
    read(j, "No grass", v.noGrass);
    read(j, "No shadows", v.noShadows);
    read<value_t::object>(j, "Motion Blur", v.motionBlur);
    read<value_t::object>(j, "Shadows changer", v.shadowsChanger);
    read(j, "Full bright", v.fullBright);
    read(j, "Zoom", v.zoom);
    read(j, "Zoom key", v.zoomKey);
    read(j, "Thirdperson Transparency", v.thirdpersonTransparency);
    read(j, "Thirdperson", v.thirdperson);
    read(j, "Thirdperson key", v.thirdpersonKey);
    read(j, "Thirdperson distance", v.thirdpersonDistance);
    read(j, "Freecam", v.freeCam);
    read(j, "Freecam key", v.freeCamKey);
    read(j, "Freecam speed", v.freeCamSpeed);
    read(j, "Keep FOV", v.keepFov);
    read(j, "FOV", v.fov);
    read(j, "Far Z", v.farZ);
    read(j, "Flash reduction", v.flashReduction);
    read(j, "Skybox", v.skybox);
    read<value_t::object>(j, "World", v.world);
    read<value_t::object>(j, "Props", v.props);
    read<value_t::object>(j, "Sky", v.sky);
    read<value_t::string>(j, "Custom skybox", v.customSkybox);
    read(j, "Deagle spinner", v.deagleSpinner);
    read(j, "Screen effect", v.screenEffect);
    read(j, "Hit effect", v.hitEffect);
    read(j, "Hit effect time", v.hitEffectTime);
    read(j, "Hit marker", v.hitMarker);
    read(j, "Hit marker time", v.hitMarkerTime);
    read(j, "Playermodel T", v.playerModelT);
    read(j, "Playermodel CT", v.playerModelCT);
    read(j, "Custom Playermodel", v.playerModel, sizeof(v.playerModel));
    read(j, "Disable jiggle bones", v.disableJiggleBones);
    read<value_t::object>(j, "Bullet Tracers", v.bulletTracers);
    read<value_t::object>(j, "Bullet Impacts", v.bulletImpacts);
    read<value_t::object>(j, "Hitbox on Hit", v.onHitHitbox);
    read(j, "Bullet Impacts time", v.bulletImpactsTime);
    read<value_t::object>(j, "Molotov Hull", v.molotovHull);
    read<value_t::object>(j, "Smoke Hull", v.smokeHull);
    read<value_t::object>(j, "Molotov Polygon", v.molotovPolygon);
    read<value_t::object>(j, "Viewmodel", v.viewModel);
    read<value_t::object>(j, "Spread circle", v.spreadCircle);
    read(j, "Asus walls", v.asusWalls);
    read(j, "Asus props", v.asusProps);
    read(j, "Smoke timer", v.smokeTimer);
    read<value_t::object>(j, "Smoke timer BG", v.smokeTimerBG);
    read<value_t::object>(j, "Smoke timer TIMER", v.smokeTimerTimer);
    read<value_t::object>(j, "Smoke timer TEXT", v.smokeTimerText);
    read(j, "Molotov timer", v.molotovTimer);
    read<value_t::object>(j, "Molotov timer BG", v.molotovTimerBG);
    read<value_t::object>(j, "Molotov timer TIMER", v.molotovTimerTimer);
    read<value_t::object>(j, "Molotov timer TEXT", v.molotovTimerText);
    read<value_t::object>(j, "Console Color", v.console);
    read<value_t::object>(j, "Smoke Color", v.smokeColor);
    read<value_t::object>(j, "Molotov Color", v.molotovColor);
    read<value_t::object>(j, "Footstep", v.footsteps);
}

static void from_json(const json& j, sticker_setting& s)
{
    read(j, "Kit", s.kit);
    read(j, "Wear", s.wear);
    read(j, "Scale", s.scale);
    read(j, "Rotation", s.rotation);

    s.onLoad();
}

static void from_json(const json& j, item_setting& i)
{
    read(j, "Enabled", i.enabled);
    read(j, "Definition index", i.itemId);
    read(j, "Quality", i.quality);
    read(j, "Paint Kit", i.paintKit);
    read(j, "Definition override", i.definition_override_index);
    read(j, "Seed", i.seed);
    read(j, "StatTrak", i.stat_trak);
    read(j, "Wear", i.wear);
    read(j, "Custom name", i.custom_name, sizeof(i.custom_name));
    read(j, "Stickers", i.stickers);

    i.onLoad();
}

static void from_json(const json& j, PurchaseList& pl)
{
    read(j, "Enabled", pl.enabled);
    read(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
    read(j, "Show Prices", pl.showPrices);
    read(j, "No Title Bar", pl.noTitleBar);
    read(j, "Mode", pl.mode);
    read<value_t::object>(j, "Pos", pl.pos);
}

static void from_json(const json& j, Config::Misc::SpectatorList& sl)
{
    read(j, "Enabled", sl.enabled);
    read(j, "No Title Bar", sl.noTitleBar);
    read<value_t::object>(j, "Pos", sl.pos);
}

static void from_json(const json& j, Config::Misc::KeyBindList& sl)
{
    read(j, "Enabled", sl.enabled);
    read(j, "No Title Bar", sl.noTitleBar);
    read<value_t::object>(j, "Pos", sl.pos);
}

static void from_json(const json& j, Config::Misc::PlayerList& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Steam ID", o.steamID);
    read(j, "Rank", o.rank);
    read(j, "Wins", o.wins);
    read(j, "Money", o.money);
    read(j, "Health", o.health);
    read(j, "Armor", o.armor);
    read<value_t::object>(j, "Pos", o.pos);
}

static void from_json(const json& j, Config::Misc::JumpStats& js)
{
    read(j, "Enabled", js.enabled);
    read(j, "Show fails", js.showFails);
    read(j, "Show color on fail", js.showColorOnFail);
    read(j, "Simplify naming", js.simplifyNaming);
}

static void from_json(const json& j, Config::Misc::Velocity& v)
{
    read(j, "Enabled", v.enabled);
    read(j, "Position", v.position);
    read(j, "Alpha", v.alpha);
    read<value_t::object>(j, "Color", v.color);
}

static void from_json(const json& j, Config::Misc::KeyBoardDisplay& kbd)
{
    read(j, "Enabled", kbd.enabled);
    read(j, "Position", kbd.position);
    read(j, "Show key Tiles", kbd.showKeyTiles);
    read<value_t::object>(j, "Color", kbd.color);
}

static void from_json(const json& j, Config::Misc::Watermark& o)
{
    read(j, "Enabled", o.enabled);
    read<value_t::object>(j, "Pos", o.pos);
}

static void from_json(const json& j, PreserveKillfeed& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Only Headshots", o.onlyHeadshots);
}

static void from_json(const json& j, KillfeedChanger& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Headshot", o.headshot);
    read(j, "Dominated", o.dominated);
    read(j, "Revenge", o.revenge);
    read(j, "Penetrated", o.penetrated);
    read(j, "Noscope", o.noscope);
    read(j, "Thrusmoke", o.thrusmoke);
    read(j, "Attackerblind", o.attackerblind);
}

static void from_json(const json& j, AutoBuy& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Primary weapon", o.primaryWeapon);
    read(j, "Secondary weapon", o.secondaryWeapon);
    read(j, "Armor", o.armor);
    read(j, "Utility", o.utility);
    read(j, "Grenades", o.grenades);
}

static void from_json(const json& j, Config::Misc::Logger& o)
{
    read(j, "Modes", o.modes);
    read(j, "Events", o.events);
}

static void from_json(const json& j, Config::Visuals::MotionBlur& mb)
{
    read(j, "Enabled", mb.enabled);
    read(j, "Forward", mb.forwardEnabled);
    read(j, "Falling min", mb.fallingMin);
    read(j, "Falling max", mb.fallingMax);
    read(j, "Falling intensity", mb.fallingIntensity);
    read(j, "Rotation intensity", mb.rotationIntensity);
    read(j, "Strength", mb.strength);
}

static void from_json(const json& j, Config::Visuals::Fog& f)
{
    read(j, "Start", f.start);
    read(j, "End", f.end);
    read(j, "Density", f.density);
}

static void from_json(const json& j, Config::Visuals::ShadowsChanger& sw)
{
    read(j, "Enabled", sw.enabled);
    read(j, "X", sw.x);
    read(j, "Y", sw.y);
}

static void from_json(const json& j, Config::Visuals::Viewmodel& vxyz)
{
    read(j, "Enabled", vxyz.enabled);
    read(j, "Fov", vxyz.fov);
    read(j, "X", vxyz.x);
    read(j, "Y", vxyz.y);
    read(j, "Z", vxyz.z);
    read(j, "Roll", vxyz.roll);
}

static void from_json(const json& j, Config::Visuals::MolotovPolygon& mp)
{
    read(j, "Enabled", mp.enabled);
    read<value_t::object>(j, "Self", mp.self);
    read<value_t::object>(j, "Team", mp.team);
    read<value_t::object>(j, "Enemy", mp.enemy);
}

static void from_json(const json& j, Config::Visuals::OnHitHitbox& h)
{
    read<value_t::object>(j, "Color", h.color);
    read(j, "Duration", h.duration);
}

static void from_json(const json& j, Config::Misc& m)
{
    read(j, "Menu key", m.menuKey);
    read(j, "Anti AFK kick", m.antiAfkKick);
    read(j, "Adblock", m.adBlock);
    read(j, "Force relay", m.forceRelayCluster);
    read(j, "Auto strafe", m.autoStrafe);
    read(j, "Bunny hop", m.bunnyHop);
    read(j, "Custom clan tag", m.customClanTag);
    read(j, "Clock tag", m.clocktag);
    read(j, "Clan tag", m.clanTag, sizeof(m.clanTag));
    read(j, "Animated clan tag", m.animatedClanTag);
    read(j, "Fast duck", m.fastDuck);
    read(j, "Moonwalk", m.moonwalk);
    read(j, "Knifebot", m.knifeBot);
    read(j, "Knifebot mode", m.knifeBotMode);
    read(j, "Block bot", m.blockBot);
    read(j, "Block bot Key", m.blockBotKey);
    read(j, "Edge Jump", m.edgeJump);
    read(j, "Edge Jump Key", m.edgeJumpKey);
    read(j, "Mini Jump", m.miniJump);
    read(j, "Mini Jump Crouch lock", m.miniJumpCrouchLock);
    read(j, "Mini Jump Key", m.miniJumpKey);
    read(j, "Jump Bug", m.jumpBug);
    read(j, "Jump Bug Key", m.jumpBugKey);
    read(j, "Edge Bug", m.edgeBug);
    read(j, "Edge Bug Key", m.edgeBugKey);
    read(j, "Pred Amnt", m.edgeBugPredAmnt);
    read(j, "Auto pixel surf", m.autoPixelSurf);
    read(j, "Auto pixel surf Pred Amnt", m.autoPixelSurfPredAmnt);
    read(j, "Auto pixel surf Key", m.autoPixelSurfKey);
    read<value_t::object>(j, "Velocity", m.velocity);
    read<value_t::object>(j, "Keyboard display", m.keyBoardDisplay);
    read(j, "Slowwalk", m.slowwalk);
    read(j, "Slowwalk key", m.slowwalkKey);
    read(j, "Slowwalk Amnt", m.slowwalkAmnt);
    read(j, "Fake duck", m.fakeduck);
    read(j, "Fake duck key", m.fakeduckKey);
    read<value_t::object>(j, "Auto peek", m.autoPeek);
    read(j, "Auto peek key", m.autoPeekKey);
    read<value_t::object>(j, "Noscope crosshair", m.noscopeCrosshair);
    read<value_t::object>(j, "Recoil crosshair", m.recoilCrosshair); 
    read<value_t::object>(j, "Headshot line", m.headshotLine);
    read(j, "Auto pistol", m.autoPistol);
    read(j, "Auto reload", m.autoReload);
    read(j, "Auto accept", m.autoAccept);
    read(j, "Radar hack", m.radarHack);
    read(j, "Reveal ranks", m.revealRanks);
    read(j, "Reveal money", m.revealMoney);
    read(j, "Reveal suspect", m.revealSuspect);
    read(j, "Reveal votes", m.revealVotes);
    read(j, "Chat revealer", m.chatRevealer);
    read<value_t::object>(j, "Spectator list", m.spectatorList);
    read<value_t::object>(j, "Keybind list", m.keybindList);
    read<value_t::object>(j, "Player list", m.playerList);
    read<value_t::object>(j, "Jump stats", m.jumpStats);
    read<value_t::object>(j, "Watermark", m.watermark);
    read<value_t::object>(j, "Offscreen Enemies", m.offscreenEnemies);
    read(j, "Disable model occlusion", m.disableModelOcclusion);
    read(j, "Aspect Ratio", m.aspectratio);
    read(j, "Kill message", m.killMessage);
    read<value_t::string>(j, "Kill message string", m.killMessageString);
    read(j, "Name stealer", m.nameStealer);
    read(j, "Disable HUD blur", m.disablePanoramablur);
    read(j, "Fast plant", m.fastPlant);
    read(j, "Fast Stop", m.fastStop);
    read<value_t::object>(j, "Bomb timer", m.bombTimer);
    read<value_t::object>(j, "Hurt indicator", m.hurtIndicator);
    read(j, "Prepare revolver", m.prepareRevolver);
    read(j, "Prepare revolver key", m.prepareRevolverKey);
    read(j, "Hit sound", m.hitSound);
    read(j, "Quick healthshot key", m.quickHealthshotKey);
    read(j, "Grenade predict", m.nadePredict);
    read<value_t::object>(j, "Grenade predict Damage", m.nadeDamagePredict);
    read<value_t::object>(j, "Grenade predict Trail", m.nadeTrailPredict);
    read<value_t::object>(j, "Grenade predict Circle", m.nadeCirclePredict);
    read(j, "Max angle delta", m.maxAngleDelta);
    read(j, "Fix tablet signal", m.fixTabletSignal);
    read<value_t::string>(j, "Custom Hit Sound", m.customHitSound);
    read(j, "Kill sound", m.killSound);
    read<value_t::string>(j, "Custom Kill Sound", m.customKillSound);
    read<value_t::object>(j, "Purchase List", m.purchaseList);
    read<value_t::object>(j, "Reportbot", m.reportbot);
    read(j, "Opposite Hand Knife", m.oppositeHandKnife);
    read<value_t::object>(j, "Preserve Killfeed", m.preserveKillfeed);
    read<value_t::object>(j, "Killfeed changer", m.killfeedChanger);
    read(j, "Sv pure bypass", m.svPureBypass);
    read(j, "Inventory Unlocker", m.inventoryUnlocker);
    read(j, "Unlock hidden cvars", m.unhideConvars);
    read<value_t::object>(j, "Autobuy", m.autoBuy);
    read<value_t::object>(j, "Logger", m.logger);
    read<value_t::object>(j, "Logger options", m.loggerOptions);
    read(j, "Name", m.name, sizeof(m.name));
    read(j, "Custom name", m.customName);
}

static void from_json(const json& j, Config::Misc::Reportbot& r)
{
    read(j, "Enabled", r.enabled);
    read(j, "Target", r.target);
    read(j, "Delay", r.delay);
    read(j, "Rounds", r.rounds);
    read(j, "Abusive Communications", r.textAbuse);
    read(j, "Griefing", r.griefing);
    read(j, "Wall Hacking", r.wallhack);
    read(j, "Aim Hacking", r.aimbot);
    read(j, "Other Hacking", r.other);
}

void Config::load(size_t id, bool incremental) noexcept
{
    load((const char8_t*)configs[id].c_str(), incremental);
}

void Config::load(const char8_t* name, bool incremental) noexcept
{
    json j;

    if (std::ifstream in{ path / name }; in.good()) {
        j = json::parse(in, nullptr, false);
        if (j.is_discarded())
            return;
    } else {
        return;
    }

    if (!incremental)
        reset();

    read(j, "Legitbot", legitbot);
    read(j, "Legitbot Key", legitbotKey);
    read<value_t::object>(j, "Draw legitbot fov", legitbotFov);

    read<value_t::object>(j, "Recoil control system", recoilControlSystem);

    read(j, "Ragebot", ragebot);
    read(j, "Ragebot Key", ragebotKey);
    read(j, "Min damage override Key", minDamageOverrideKey);

    read(j, "Triggerbot", triggerbot);
    read(j, "Triggerbot Key", triggerbotKey);

    read<value_t::object>(j, "Legit Anti aim", legitAntiAim);
    read<value_t::object>(j, "Rage Anti aim", rageAntiAim);
    read(j, "Disable in freezetime", disableInFreezetime);
    read<value_t::object>(j, "Fake angle", fakeAngle);
    read<value_t::object>(j, "Fakelag", fakelag);
    read<value_t::object>(j, "Tickbase", tickbase);
    read<value_t::object>(j, "Backtrack", backtrack);

    read(j["Glow"], "Items", glow);
    read(j["Glow"], "Players", playerGlow);
    read(j["Glow"], "Key", glowKey);

    read(j, "Chams", chams);
    read(j["Chams"], "Key", chamsKey);
    read<value_t::object>(j, "ESP", streamProofESP);
    read<value_t::object>(j, "Visuals", visuals);
    read(j, "Skin changer", skinChanger);
    ::Sound::fromJson(j["Sound"]);
    read<value_t::object>(j, "Misc", misc);
}

#pragma endregion

#pragma region  Write

static void to_json(json& j, const ColorToggle& o, const ColorToggle& dummy = {})
{
    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const Color3& o, const Color3& dummy = {})
{
    WRITE("Color", color);
    WRITE("Rainbow", rainbow);
    WRITE("Rainbow Speed", rainbowSpeed);
}

static void to_json(json& j, const ColorToggle3& o, const ColorToggle3& dummy = {})
{
    to_json(j, static_cast<const Color3&>(o), dummy);
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const ColorToggleRounding& o, const ColorToggleRounding& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Rounding", rounding);
}

static void to_json(json& j, const ColorToggleThickness& o, const ColorToggleThickness& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Thickness", thickness);
}

static void to_json(json& j, const ColorToggleOutline& o, const ColorToggleOutline& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Outline", outline);
}

static void to_json(json& j, const ColorToggleThicknessRounding& o, const ColorToggleThicknessRounding& dummy = {})
{
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Thickness", thickness);
}

static void to_json(json& j, const Font& o, const Font& dummy = {})
{
    WRITE("Name", name);
}

static void to_json(json& j, const Snapline& o, const Snapline& dummy = {})
{
    to_json(j, static_cast<const ColorToggleThickness&>(o), dummy);
    WRITE("Type", type);
}

static void to_json(json& j, const Box& o, const Box& dummy = {})
{
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Type", type);
    WRITE("Scale", scale);
    WRITE("Fill", fill);
}

static void to_json(json& j, const Shared& o, const Shared& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Font", font);
    WRITE("Snapline", snapline);
    WRITE("Box", box);
    WRITE("Name", name);
    WRITE("Text Cull Distance", textCullDistance);
}

static void to_json(json& j, const HealthBar& o, const HealthBar& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Type", type);
}

static void to_json(json& j, const Player& o, const Player& dummy = {})
{
    to_json(j, static_cast<const Shared&>(o), dummy);
    WRITE("Weapon", weapon);
    WRITE("Flash Duration", flashDuration);
    WRITE("Audible Only", audibleOnly);
    WRITE("Spotted Only", spottedOnly);
    WRITE("Health Bar", healthBar);
    WRITE("Skeleton", skeleton);
    WRITE("Head Box", headBox);
    WRITE("Line of sight", lineOfSight);
}

static void to_json(json& j, const Weapon& o, const Weapon& dummy = {})
{
    to_json(j, static_cast<const Shared&>(o), dummy);
    WRITE("Ammo", ammo);
}

static void to_json(json& j, const Trail& o, const Trail& dummy = {})
{
    to_json(j, static_cast<const ColorToggleThickness&>(o), dummy);
    WRITE("Type", type);
    WRITE("Time", time);
}

static void to_json(json& j, const Trails& o, const Trails& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Local Player", localPlayer);
    WRITE("Allies", allies);
    WRITE("Enemies", enemies);
}

static void to_json(json& j, const OffscreenEnemies& o, const OffscreenEnemies& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);

    WRITE("Health Bar", healthBar);
}

static void to_json(json& j, const BulletTracers& o, const BulletTracers& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
}

static void to_json(json& j, const Projectile& o, const Projectile& dummy = {})
{
    j = static_cast<const Shared&>(o);

    WRITE("Trails", trails);
}

static void to_json(json& j, const ImVec2& o, const ImVec2& dummy = {})
{
    WRITE("X", x);
    WRITE("Y", y);
}

static void to_json(json& j, const Config::Legitbot& o, const Config::Legitbot& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Aimlock", aimlock);
    WRITE("Silent", silent);
    WRITE("Friendly fire", friendlyFire);
    WRITE("Visible only", visibleOnly);
    WRITE("Scoped only", scopedOnly);
    WRITE("Ignore flash", ignoreFlash);
    WRITE("Ignore smoke", ignoreSmoke);
    WRITE("Auto scope", autoScope);
    WRITE("Hitboxes", hitboxes);
    WRITE("Fov", fov);
    WRITE("Smooth", smooth);
    WRITE("Reaction time", reactionTime);
    WRITE("Min damage", minDamage);
    WRITE("Killshot", killshot);
    WRITE("Between shots", betweenShots);
}

static void to_json(json& j, const Config::RecoilControlSystem& o, const Config::RecoilControlSystem& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Silent", silent);
    WRITE("Ignore Shots", shotsFired);
    WRITE("Horizontal", horizontal);
    WRITE("Vertical", vertical);
}

static void to_json(json& j, const Config::Ragebot& o, const Config::Ragebot& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Aimlock", aimlock);
    WRITE("Silent", silent);
    WRITE("Friendly fire", friendlyFire);
    WRITE("Visible only", visibleOnly);
    WRITE("Scoped only", scopedOnly);
    WRITE("Ignore flash", ignoreFlash);
    WRITE("Ignore smoke", ignoreSmoke);
    WRITE("Auto shot", autoShot);
    WRITE("Auto scope", autoScope);
    WRITE("Auto stop", autoStop);
    WRITE("Between shots", betweenShots);
    WRITE("Disable multipoint if low fps", disableMultipointIfLowFPS);
    WRITE("Disable backtrack if low fps", disableMultipointIfLowFPS);
    WRITE("Priority", priority);
    WRITE("Fov", fov);
    WRITE("Hitboxes", hitboxes);
    WRITE("Hitchance", hitChance);
    WRITE("Multipoint", multiPoint);
    WRITE("Min damage", minDamage);
    WRITE("Min damage override", minDamageOverride);
}

static void to_json(json& j, const Config::Triggerbot& o, const Config::Triggerbot& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Friendly fire", friendlyFire);
    WRITE("Scoped only", scopedOnly);
    WRITE("Ignore flash", ignoreFlash);
    WRITE("Ignore smoke", ignoreSmoke);
    WRITE("Hitboxes", hitboxes);
    WRITE("Hitchance", hitChance);
    WRITE("Shot delay", shotDelay);
    WRITE("Min damage", minDamage);
    WRITE("Killshot", killshot);
}

static void to_json(json& j, const Config::Chams::Material& o)
{
    const Config::Chams::Material dummy;

    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
    WRITE("Health based", healthBased);
    WRITE("Blinking", blinking);
    WRITE("Wireframe", wireframe);
    WRITE("Cover", cover);
    WRITE("Ignore-Z", ignorez);
    WRITE("Material", material);
}

static void to_json(json& j, const Config::Chams& o)
{
    j["Materials"] = o.materials;
}

static void to_json(json& j, const Config::GlowItem& o, const  Config::GlowItem& dummy = {})
{
    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
    WRITE("Health based", healthBased);
    WRITE("Style", style);
}

static void to_json(json& j, const  Config::PlayerGlow& o, const  Config::PlayerGlow& dummy = {})
{
    WRITE("All", all);
    WRITE("Visible", visible);
    WRITE("Occluded", occluded);
}

static void to_json(json& j, const Config::StreamProofESP& o, const Config::StreamProofESP& dummy = {})
{
    WRITE("Key", key);
    j["Allies"] = o.allies;
    j["Enemies"] = o.enemies;
    j["Weapons"] = o.weapons;
    j["Projectiles"] = o.projectiles;
    j["Loot Crates"] = o.lootCrates;
    j["Other Entities"] = o.otherEntities;
}

static void to_json(json& j, const Config::Misc::Reportbot& o, const Config::Misc::Reportbot& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Target", target);
    WRITE("Delay", delay);
    WRITE("Rounds", rounds);
    WRITE("Abusive Communications", textAbuse);
    WRITE("Griefing", griefing);
    WRITE("Wall Hacking", wallhack);
    WRITE("Aim Hacking", aimbot);
    WRITE("Other Hacking", other);
}

static void to_json(json& j, const Config::LegitAntiAimConfig& o, const Config::LegitAntiAimConfig& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Extend", extend);
    WRITE("Invert key", invert);
}

static void to_json(json& j, const Config::RageAntiAimConfig& o, const Config::RageAntiAimConfig& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Pitch", pitch);
    WRITE_ENUM("Yaw base", yawBase);
    to_json(j["Manual forward Key"], o.manualForward, KeyBind::NONE);
    to_json(j["Manual backward Key"], o.manualBackward, KeyBind::NONE);
    to_json(j["Manual right Key"], o.manualRight, KeyBind::NONE);
    to_json(j["Manual left Key"], o.manualLeft, KeyBind::NONE);
    WRITE("Yaw modifier", yawModifier);
    WRITE("Yaw add", yawAdd);
    WRITE("Jitter Range", jitterRange);
    WRITE("Spin base", spinBase);
    WRITE("At targets", atTargets);
}

static void to_json(json& j, const Config::FakeAngle& o, const Config::FakeAngle& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Invert", invert);
    WRITE("Left limit", leftLimit);
    WRITE("Right limit", rightLimit);
    WRITE("Peek mode", peekMode);
    WRITE("Lby mode", lbyMode);
}

static void to_json(json& j, const Config::Fakelag& o, const Config::Fakelag& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Mode", mode);
    WRITE("Limit", limit);
}

static void to_json(json& j, const Config::Tickbase& o, const Config::Tickbase& dummy = {})
{
    WRITE("Doubletap", doubletap);
    WRITE("Hideshots", hideshots);
    WRITE("Teleport", teleport);
}

static void to_json(json& j, const Config::Backtrack& o, const Config::Backtrack& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Ignore smoke", ignoreSmoke);
    WRITE("Ignore flash", ignoreFlash);
    WRITE("Time limit", timeLimit);
    WRITE("Fake Latency", fakeLatency);
    WRITE("Fake Latency Amount", fakeLatencyAmount);
}

static void to_json(json& j, const PurchaseList& o, const PurchaseList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Only During Freeze Time", onlyDuringFreezeTime);
    WRITE("Show Prices", showPrices);
    WRITE("No Title Bar", noTitleBar);
    WRITE("Mode", mode);

    if (const auto window = ImGui::FindWindowByName("Purchases")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::SpectatorList& o, const Config::Misc::SpectatorList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);

    if (const auto window = ImGui::FindWindowByName("Spectator list")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::KeyBindList& o, const Config::Misc::KeyBindList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);

    if (const auto window = ImGui::FindWindowByName("Keybind list")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::PlayerList& o, const Config::Misc::PlayerList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Steam ID", steamID);
    WRITE("Rank", rank);
    WRITE("Wins", wins);
    WRITE("Money", money);
    WRITE("Health", health);
    WRITE("Armor", armor);

    if (const auto window = ImGui::FindWindowByName("Player List")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::JumpStats& o, const Config::Misc::JumpStats& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Show fails", showFails);
    WRITE("Show color on fail", showColorOnFail);
    WRITE("Simplify naming", simplifyNaming);
}

static void to_json(json& j, const Config::Misc::Velocity& o, const Config::Misc::Velocity& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Position", position);
    WRITE("Alpha", alpha);
    WRITE("Color", color);
}

static void to_json(json& j, const Config::Misc::KeyBoardDisplay& o, const Config::Misc::KeyBoardDisplay& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Position", position);
    WRITE("Show key Tiles", showKeyTiles);
    WRITE("Color", color);
}

static void to_json(json& j, const Config::Misc::Watermark& o, const Config::Misc::Watermark& dummy = {})
{
    WRITE("Enabled", enabled);

    if (const auto window = ImGui::FindWindowByName("Watermark")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const PreserveKillfeed& o, const PreserveKillfeed& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Only Headshots", onlyHeadshots);
}

static void to_json(json& j, const KillfeedChanger& o, const KillfeedChanger& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Headshot", headshot);
    WRITE("Dominated", dominated);
    WRITE("Revenge", revenge);
    WRITE("Penetrated", penetrated);
    WRITE("Noscope", noscope);
    WRITE("Thrusmoke", thrusmoke);
    WRITE("Attackerblind", attackerblind);
}

static void to_json(json& j, const AutoBuy& o, const AutoBuy& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Primary weapon", primaryWeapon);
    WRITE("Secondary weapon", secondaryWeapon);
    WRITE("Armor", armor);
    WRITE("Utility", utility);
    WRITE("Grenades", grenades);
}

static void to_json(json& j, const Config::Misc::Logger& o, const Config::Misc::Logger& dummy = {})
{
    WRITE("Modes", modes);
    WRITE("Events", events);
}

static void to_json(json& j, const Config::Visuals::FootstepESP& o, const Config::Visuals::FootstepESP& dummy)
{
    WRITE("Enabled", footstepBeams);
    WRITE("Thickness", footstepBeamThickness);
    WRITE("Radius", footstepBeamRadius);
}

static void to_json(json& j, const Config::Visuals::MotionBlur& o, const Config::Visuals::MotionBlur& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("Forward", forwardEnabled);
    WRITE("Falling min", fallingMin);
    WRITE("Falling max", fallingMax);
    WRITE("Falling intensity", fallingIntensity);
    WRITE("Rotation intensity", rotationIntensity);
    WRITE("Strength", strength);
}

static void to_json(json& j, const Config::Visuals::Fog& o, const Config::Visuals::Fog& dummy)
{
    WRITE("Start", start);
    WRITE("End", end);
    WRITE("Density", density);
}

static void to_json(json& j, const Config::Visuals::ShadowsChanger& o, const Config::Visuals::ShadowsChanger& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("X", x);
    WRITE("Y", y);
}

static void to_json(json& j, const Config::Visuals::Viewmodel& o, const Config::Visuals::Viewmodel& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("Fov", fov);
    WRITE("X", x);
    WRITE("Y", y);
    WRITE("Z", z);
    WRITE("Roll", roll);
}

static void to_json(json& j, const Config::Visuals::MolotovPolygon& o, const Config::Visuals::MolotovPolygon& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("Self", self);
    WRITE("Team", team);
    WRITE("Enemy", enemy);
}

static void to_json(json& j, const Config::Visuals::OnHitHitbox& o, const Config::Visuals::OnHitHitbox& dummy)
{
    WRITE("Color", color);
    WRITE("Duration", duration);
}

static void to_json(json& j, const Config::Misc& o)
{
    const Config::Misc dummy;

    WRITE("Menu key", menuKey);
    WRITE("Anti AFK kick", antiAfkKick);
    WRITE("Adblock", adBlock);
    WRITE("Force relay", forceRelayCluster);
    WRITE("Auto strafe", autoStrafe);
    WRITE("Bunny hop", bunnyHop);
    WRITE("Custom clan tag", customClanTag);
    WRITE("Clock tag", clocktag);

    if (o.clanTag[0])
        j["Clan tag"] = o.clanTag;

    WRITE("Animated clan tag", animatedClanTag);
    WRITE("Fast duck", fastDuck);
    WRITE("Moonwalk", moonwalk);
    WRITE("Knifebot", knifeBot);
    WRITE("Knifebot mode", knifeBotMode);
    WRITE("Block bot", blockBot);
    WRITE("Block bot Key", blockBotKey);
    WRITE("Edge Jump", edgeJump);
    WRITE("Edge Jump Key", edgeJumpKey);
    WRITE("Mini Jump", miniJump);
    WRITE("Mini Jump Crouch lock", miniJumpCrouchLock);
    WRITE("Mini Jump Key", miniJumpKey);
    WRITE("Jump Bug", jumpBug);
    WRITE("Jump Bug Key", jumpBugKey);
    WRITE("Edge Bug", edgeBug);
    WRITE("Edge Bug Key", edgeBugKey);
    WRITE("Pred Amnt", edgeBugPredAmnt);
    WRITE("Auto pixel surf", autoPixelSurf);
    WRITE("Auto pixel surf Pred Amnt", autoPixelSurfPredAmnt);
    WRITE("Auto pixel surf Key", autoPixelSurfKey);
    WRITE("Velocity", velocity);
    WRITE("Keyboard display", keyBoardDisplay);
    WRITE("Slowwalk", slowwalk);
    WRITE("Slowwalk key", slowwalkKey);
    WRITE("Slowwalk Amnt", slowwalkAmnt);
    WRITE("Fake duck", fakeduck);
    WRITE("Fake duck key", fakeduckKey);
    WRITE("Auto peek", autoPeek);
    WRITE("Auto peek key", autoPeekKey);
    WRITE("Noscope crosshair", noscopeCrosshair);
    WRITE("Recoil crosshair", recoilCrosshair);
    WRITE("Headshot line", headshotLine);
    WRITE("Auto pistol", autoPistol);
    WRITE("Auto reload", autoReload);
    WRITE("Auto accept", autoAccept);
    WRITE("Radar hack", radarHack);
    WRITE("Reveal ranks", revealRanks);
    WRITE("Reveal money", revealMoney);
    WRITE("Reveal suspect", revealSuspect);
    WRITE("Reveal votes", revealVotes);
    WRITE("Chat revealer", chatRevealer);
    WRITE("Spectator list", spectatorList);
    WRITE("Keybind list", keybindList);
    WRITE("Player list", playerList);
    WRITE("Jump stats", jumpStats);
    WRITE("Watermark", watermark);
    WRITE("Offscreen Enemies", offscreenEnemies);
    WRITE("Disable model occlusion", disableModelOcclusion);
    WRITE("Aspect Ratio", aspectratio);
    WRITE("Kill message", killMessage);
    WRITE("Kill message string", killMessageString);
    WRITE("Name stealer", nameStealer);
    WRITE("Disable HUD blur", disablePanoramablur);
    WRITE("Fast plant", fastPlant);
    WRITE("Fast Stop", fastStop);
    WRITE("Bomb timer", bombTimer);
    WRITE("Hurt indicator", hurtIndicator);
    WRITE("Prepare revolver", prepareRevolver);
    WRITE("Prepare revolver key", prepareRevolverKey);
    WRITE("Hit sound", hitSound);
    WRITE("Quick healthshot key", quickHealthshotKey);
    WRITE("Grenade predict", nadePredict);
    WRITE("Grenade predict Damage", nadeDamagePredict);
    WRITE("Grenade predict Trail", nadeTrailPredict);
    WRITE("Grenade predict Circle", nadeCirclePredict);
    WRITE("Max angle delta", maxAngleDelta);
    WRITE("Fix tablet signal", fixTabletSignal);
    WRITE("Custom Hit Sound", customHitSound);
    WRITE("Kill sound", killSound);
    WRITE("Custom Kill Sound", customKillSound);
    WRITE("Purchase List", purchaseList);
    WRITE("Reportbot", reportbot);
    WRITE("Opposite Hand Knife", oppositeHandKnife);
    WRITE("Preserve Killfeed", preserveKillfeed);
    WRITE("Killfeed changer", killfeedChanger);
    WRITE("Sv pure bypass", svPureBypass);
    WRITE("Inventory Unlocker", inventoryUnlocker);
    WRITE("Unlock hidden cvars", unhideConvars);
    WRITE("Autobuy", autoBuy);
    WRITE("Logger", logger);
    WRITE("Logger options", loggerOptions);

    WRITE("Custom name", customClanTag);

    if (o.clanTag[0])
        j["Name"] = o.name;
}

static void to_json(json& j, const Config::Visuals& o)
{
    const Config::Visuals dummy;

    WRITE("Disable post-processing", disablePostProcessing);
    WRITE("Inverse ragdoll gravity", inverseRagdollGravity);
    WRITE("No fog", noFog);
    WRITE("Fog controller", fog);
    WRITE("Fog options", fogOptions);
    WRITE("No 3d sky", no3dSky);
    WRITE("No aim punch", noAimPunch);
    WRITE("No view punch", noViewPunch);
    WRITE("No view bob", noViewBob);
    WRITE("No hands", noHands);
    WRITE("No sleeves", noSleeves);
    WRITE("No weapons", noWeapons);
    WRITE("No smoke", noSmoke);
    WRITE("Smoke circle", smokeCircle);
    WRITE("Wireframe smoke", wireframeSmoke);
    WRITE("No molotov", noMolotov);
    WRITE("Wireframe molotov", wireframeMolotov);
    WRITE("No blur", noBlur);
    WRITE("No scope overlay", noScopeOverlay);
    WRITE("No grass", noGrass);
    WRITE("No shadows", noShadows);
    WRITE("Shadows changer", shadowsChanger);
    WRITE("Motion Blur", motionBlur);
    WRITE("Full bright", fullBright);
    WRITE("Zoom", zoom);
    WRITE("Zoom key", zoomKey);
    WRITE("Thirdperson Transparency", thirdpersonTransparency);
    WRITE("Thirdperson", thirdperson);
    WRITE("Thirdperson key", thirdpersonKey);
    WRITE("Thirdperson distance", thirdpersonDistance);
    WRITE("Freecam", freeCam);
    WRITE("Freecam key", freeCamKey);
    WRITE("Freecam speed", freeCamSpeed);
    WRITE("Keep FOV", keepFov);
    WRITE("FOV", fov);
    WRITE("Far Z", farZ);
    WRITE("Flash reduction", flashReduction);
    WRITE("Skybox", skybox);
    WRITE("World", world);
    WRITE("Props", props);
    WRITE("Sky", sky);
    WRITE("Custom skybox", customSkybox);
    WRITE("Deagle spinner", deagleSpinner);
    WRITE("Screen effect", screenEffect);
    WRITE("Hit effect", hitEffect);
    WRITE("Hit effect time", hitEffectTime);
    WRITE("Hit marker", hitMarker);
    WRITE("Hit marker time", hitMarkerTime);
    WRITE("Playermodel T", playerModelT);
    WRITE("Playermodel CT", playerModelCT);
    if (o.playerModel[0])
        j["Custom Playermodel"] = o.playerModel;
    WRITE("Disable jiggle bones", disableJiggleBones);
    WRITE("Bullet Tracers", bulletTracers);
    WRITE("Bullet Impacts", bulletImpacts);
    WRITE("Hitbox on Hit", onHitHitbox);
    WRITE("Bullet Impacts time", bulletImpactsTime);
    WRITE("Molotov Hull", molotovHull);
    WRITE("Smoke Hull", smokeHull);
    WRITE("Molotov Polygon", molotovPolygon);
    WRITE("Viewmodel", viewModel);
    WRITE("Spread circle", spreadCircle);
    WRITE("Asus walls", asusWalls);
    WRITE("Asus props", asusProps);
    WRITE("Smoke timer", smokeTimer);
    WRITE("Smoke timer BG", smokeTimerBG);
    WRITE("Smoke timer TIMER", smokeTimerTimer);
    WRITE("Smoke timer TEXT", smokeTimerText);
    WRITE("Molotov timer", molotovTimer);
    WRITE("Molotov timer BG", molotovTimerBG);
    WRITE("Molotov timer TIMER", molotovTimerTimer);
    WRITE("Molotov timer TEXT", molotovTimerText);
    WRITE("Console Color", console);
    WRITE("Smoke Color", smokeColor);
    WRITE("Molotov Color", molotovColor);
    WRITE("Footstep", footsteps);
}

static void to_json(json& j, const ImVec4& o)
{
    j[0] = o.x;
    j[1] = o.y;
    j[2] = o.z;
    j[3] = o.w;
}

static void to_json(json& j, const sticker_setting& o)
{
    const sticker_setting dummy;

    WRITE("Kit", kit);
    WRITE("Wear", wear);
    WRITE("Scale", scale);
    WRITE("Rotation", rotation);
}

static void to_json(json& j, const item_setting& o)
{
    const item_setting dummy;

    WRITE("Enabled", enabled);
    WRITE("Definition index", itemId);
    WRITE("Quality", quality);
    WRITE("Paint Kit", paintKit);
    WRITE("Definition override", definition_override_index);
    WRITE("Seed", seed);
    WRITE("StatTrak", stat_trak);
    WRITE("Wear", wear);
    if (o.custom_name[0])
        j["Custom name"] = o.custom_name;
    WRITE("Stickers", stickers);
}

#pragma endregion

void removeEmptyObjects(json& j) noexcept
{
    for (auto it = j.begin(); it != j.end();) {
        auto& val = it.value();
        if (val.is_object() || val.is_array())
            removeEmptyObjects(val);
        if (val.empty() && !j.is_array())
            it = j.erase(it);
        else
            ++it;
    }
}

void Config::save(size_t id) const noexcept
{
    createConfigDir();

    if (std::ofstream out{ path / (const char8_t*)configs[id].c_str() }; out.good()) {
        json j;

        j["Legitbot"] = legitbot;
        to_json(j["Legitbot Key"], legitbotKey, KeyBind::NONE);
        j["Draw legitbot fov"] = legitbotFov;

        j["Recoil control system"] = recoilControlSystem;

        j["Ragebot"] = ragebot;
        to_json(j["Ragebot Key"], ragebotKey, KeyBind::NONE);
        to_json(j["Min damage override Key"], minDamageOverrideKey, KeyBind::NONE);

        j["Triggerbot"] = triggerbot;
        to_json(j["Triggerbot Key"], triggerbotKey, KeyBind::NONE);

        j["Legit Anti aim"] = legitAntiAim;
        j["Rage Anti aim"] = rageAntiAim;
        j["Disable in freezetime"] = disableInFreezetime;
        j["Fake angle"] = fakeAngle;
        j["Fakelag"] = fakelag;
        j["Tickbase"] = tickbase;
        j["Backtrack"] = backtrack;

        j["Glow"]["Items"] = glow;
        j["Glow"]["Players"] = playerGlow;
        to_json(j["Glow"]["Key"], glowKey, KeyBind::NONE);

        j["Chams"] = chams;
        to_json(j["Chams"]["Key"], chamsKey, KeyBind::NONE);
        j["ESP"] = streamProofESP;
        j["Sound"] = ::Sound::toJson();
        j["Visuals"] = visuals;
        j["Misc"] = misc;
        j["Skin changer"] = skinChanger;

        removeEmptyObjects(j);
        out << std::setw(2) << j;
    }
}

void Config::add(const char* name) noexcept
{
    if (*name && std::find(configs.cbegin(), configs.cend(), name) == configs.cend()) {
        configs.emplace_back(name);
        save(configs.size() - 1);
    }
}

void Config::remove(size_t id) noexcept
{
    std::error_code ec;
    std::filesystem::remove(path / (const char8_t*)configs[id].c_str(), ec);
    configs.erase(configs.cbegin() + id);
}

void Config::rename(size_t item, const char* newName) noexcept
{
    std::error_code ec;
    std::filesystem::rename(path / (const char8_t*)configs[item].c_str(), path / (const char8_t*)newName, ec);
    configs[item] = newName;
}

void Config::reset() noexcept
{
    legitbot = { };
    recoilControlSystem = { };
    legitAntiAim = { };
    ragebot = { };
    rageAntiAim = { };
    disableInFreezetime = true;
    fakeAngle = { };
    fakelag = { };
    tickbase = { };
    backtrack = { };
    triggerbot = { };
    Glow::resetConfig();
    chams = { };
    streamProofESP = { };
    visuals = { };
    skinChanger = { };
    Sound::resetConfig();
    misc = { };
}

void Config::listConfigs() noexcept
{
    configs.clear();

    std::error_code ec;
    std::transform(std::filesystem::directory_iterator{ path, ec },
        std::filesystem::directory_iterator{ },
        std::back_inserter(configs),
        [](const auto& entry) { return std::string{ (const char*)entry.path().filename().u8string().c_str() }; });
}

void Config::createConfigDir() const noexcept
{
    std::error_code ec; std::filesystem::create_directory(path, ec);
}

void Config::openConfigDir() const noexcept
{
    createConfigDir();
    ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Config::scheduleFontLoad(const std::string& name) noexcept
{
    scheduledFonts.push_back(name);
}

static auto getFontData(const std::string& fontName) noexcept
{
    HFONT font = CreateFontA(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName.c_str());

    std::unique_ptr<std::byte[]> data;
    DWORD dataSize = GDI_ERROR;

    if (font) {
        HDC hdc = CreateCompatibleDC(nullptr);

        if (hdc) {
            SelectObject(hdc, font);
            dataSize = GetFontData(hdc, 0, 0, nullptr, 0);

            if (dataSize != GDI_ERROR) {
                data = std::make_unique<std::byte[]>(dataSize);
                dataSize = GetFontData(hdc, 0, 0, data.get(), dataSize);

                if (dataSize == GDI_ERROR)
                    data.reset();
            }
            DeleteDC(hdc);
        }
        DeleteObject(font);
    }
    return std::make_pair(std::move(data), dataSize);
}

bool Config::loadScheduledFonts() noexcept
{
    bool result = false;

    for (const auto& fontName : scheduledFonts) {
        if (fontName == "Default") {
            if (fonts.find("Default") == fonts.cend()) {
                ImFontConfig cfg;
                cfg.OversampleH = cfg.OversampleV = 1;
                cfg.PixelSnapH = true;
                cfg.RasterizerMultiply = 1.7f;

                Font newFont;

                cfg.SizePixels = 13.0f;
                newFont.big = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                cfg.SizePixels = 10.0f;
                newFont.medium = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                cfg.SizePixels = 8.0f;
                newFont.tiny = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                fonts.emplace(fontName, newFont);
                result = true;
            }
            continue;
        }

        const auto [fontData, fontDataSize] = getFontData(fontName);
        if (fontDataSize == GDI_ERROR)
            continue;

        if (fonts.find(fontName) == fonts.cend()) {
            const auto ranges = Helpers::getFontGlyphRanges();
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            cfg.RasterizerMultiply = 1.7f;

            Font newFont;
            newFont.tiny = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 8.0f, &cfg, ranges);
            newFont.medium = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 10.0f, &cfg, ranges);
            newFont.big = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 13.0f, &cfg, ranges);
            fonts.emplace(fontName, newFont);
            result = true;
        }
    }
    scheduledFonts.clear();
    return result;
}
