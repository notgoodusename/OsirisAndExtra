#include <array>
#include <cstring>
#include <string.h>
#include <deque>
#include <sys/stat.h>

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include "../fnv.h"
#include "../GameData.h"
#include "../Helpers.h"

#include "AimbotFunctions.h"
#include "Animations.h"
#include "Backtrack.h"
#include "Visuals.h"

#include "../SDK/ConVar.h"
#include "../SDK/DebugOverlay.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/NetworkStringTable.h"
#include "../SDK/RenderContext.h"
#include "../SDK/Surface.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/ViewRenderBeams.h"
#include "../SDK/ViewSetup.h"

void Visuals::shadowChanger() noexcept
{
    static auto cl_csm_rot_override = interfaces->cvar->findVar("cl_csm_rot_override");
    static auto cl_csm_max_shadow_dist = interfaces->cvar->findVar("cl_csm_max_shadow_dist");
    static auto cl_csm_rot_x = interfaces->cvar->findVar("cl_csm_rot_x");
    static auto cl_csm_rot_y = interfaces->cvar->findVar("cl_csm_rot_y");
    
    if (config->visuals.noShadows || !config->visuals.shadowsChanger.enabled)
    {
        cl_csm_rot_override->setValue(0);
        return;
    }

    cl_csm_max_shadow_dist->setValue(800);
    cl_csm_rot_override->setValue(1);
    cl_csm_rot_x->setValue(config->visuals.shadowsChanger.x);
    cl_csm_rot_y->setValue(config->visuals.shadowsChanger.y);
}

#define SMOKEGRENADE_LIFETIME 17.5f

struct smokeData
{
    float destructionTime;
    Vector pos;
};

static std::vector<smokeData> smokes;

void Visuals::drawSmokeTimerEvent(GameEvent* event) noexcept
{
    if (!event)
        return;

    smokeData data{};
    const auto time = memory->globalVars->realtime + SMOKEGRENADE_LIFETIME;
    const auto pos = Vector(event->getFloat("x"), event->getFloat("y"), event->getFloat("z"));
    data.destructionTime = time;
    data.pos = pos;
    smokes.push_back(data);
}

void Visuals::drawSmokeTimer(ImDrawList* drawList) noexcept
{
    if (!config->visuals.smokeTimer)
        return;

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return;

    for (size_t i = 0; i < smokes.size(); i++) {
        const auto& smoke = smokes[i];

        auto time = smoke.destructionTime - memory->globalVars->realtime;
        std::ostringstream text; text << std::fixed << std::showpoint << std::setprecision(1) << time << " sec.";
        auto textSize = ImGui::CalcTextSize(text.str().c_str());

        ImVec2 pos;

        if (time >= 0.0f) {
            if (Helpers::worldToScreen(smoke.pos, pos)) {
                ImRect rect_out(
                    pos.x + (textSize.x / 2) + 2.f,
                    pos.y + (textSize.y / 2) + 10.f,
                    pos.x - (textSize.x / 2) - 2.f,
                    pos.y - (textSize.y / 2) - 2.f);

                ImRect rect_in(
                    (pos.x + (textSize.x / 2)) - (textSize.x * (1.0f - (time / SMOKEGRENADE_LIFETIME))),
                    pos.y + (textSize.y / 2),
                    pos.x - (textSize.x / 2),
                    pos.y + (textSize.y));

                drawList->AddRectFilled(rect_out.Min, rect_out.Max, Helpers::calculateColor(config->visuals.smokeTimerBG));
                drawList->AddRectFilled(rect_in.Min, rect_in.Max, Helpers::calculateColor(config->visuals.smokeTimerTimer));
                drawList->AddText({ pos.x - (textSize.x / 2), pos.y - (textSize.y / 2) }, Helpers::calculateColor(config->visuals.smokeTimerText), text.str().c_str());
            }
        }
        else
            smokes.erase(smokes.begin() + i);
    }
}

#define MOLOTOV_LIFETIME 7.0f

struct molotovData
{
    float destructionTime;
    Vector pos;
};

static std::vector<molotovData> molotovs;

void Visuals::drawMolotovTimerEvent(GameEvent* event) noexcept
{
    if (!event)
        return;

    molotovData data{};
    const auto time = memory->globalVars->realtime + MOLOTOV_LIFETIME;
    const auto pos = Vector(event->getFloat("x"), event->getFloat("y"), event->getFloat("z"));
    data.destructionTime = time;
    data.pos = pos;
    molotovs.push_back(data);
}

void Visuals::molotovExtinguishEvent(GameEvent* event) noexcept {

    if (!event)
        return;

    if (molotovs.empty())
        return;

    molotovs.erase(molotovs.begin());
}

void Visuals::drawMolotovTimer(ImDrawList* drawList) noexcept
{
    if (!config->visuals.molotovTimer)
        return;

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return;

    for (size_t i = 0; i < molotovs.size(); i++) {
        const auto& molotov = molotovs[i];

        auto time = molotov.destructionTime - memory->globalVars->realtime;
        std::ostringstream text; text << std::fixed << std::showpoint << std::setprecision(1) << time << " sec.";
        auto textSize = ImGui::CalcTextSize(text.str().c_str());

        ImVec2 pos;

        if (time >= 0.0f) {
            if (Helpers::worldToScreen(molotov.pos, pos)) {
                ImRect rect_out(
                    pos.x + (textSize.x / 2) + 2.f,
                    pos.y + (textSize.y / 2) + 10.f,
                    pos.x - (textSize.x / 2) - 2.f,
                    pos.y - (textSize.y / 2) - 2.f);

                ImRect rect_in(
                    (pos.x + (textSize.x / 2)) - (textSize.x * (1.0f - (time / MOLOTOV_LIFETIME))),
                    pos.y + (textSize.y / 2),
                    pos.x - (textSize.x / 2),
                    pos.y + (textSize.y));

                drawList->AddRectFilled(rect_out.Min, rect_out.Max, Helpers::calculateColor(config->visuals.molotovTimerBG));
                drawList->AddRectFilled(rect_in.Min, rect_in.Max, Helpers::calculateColor(config->visuals.molotovTimerTimer));
                drawList->AddText({ pos.x - (textSize.x / 2), pos.y - (textSize.y / 2) }, Helpers::calculateColor(config->visuals.molotovTimerText), text.str().c_str());
            }
        }
    }
}

void Visuals::visualizeSpread(ImDrawList* drawList) noexcept
{
    if (!config->visuals.spreadCircle.enabled)
        return;

    GameData::Lock lock;
    const auto& local = GameData::local();

    if (!local.exists || !local.alive || local.inaccuracy.null())
        return;

    if (memory->input->isCameraInThirdPerson)
        return;

    if (ImVec2 edge; Helpers::worldToScreen(local.inaccuracy, edge))
    {
        const auto& displaySize = ImGui::GetIO().DisplaySize;
        const auto radius = std::sqrtf(ImLengthSqr(edge - displaySize / 2.0f));

        if (radius > displaySize.x || radius > displaySize.y || !std::isfinite(radius))
            return;

        const auto color = Helpers::calculateColor(config->visuals.spreadCircle);
        drawList->AddCircleFilled(displaySize / 2.0f, radius, color);
        if (config->visuals.spreadCircle.outline)
            drawList->AddCircle(displaySize / 2.0f, radius, color | IM_COL32_A_MASK);
    }
}

void Visuals::drawAimbotFov(ImDrawList* drawList) noexcept
{
    if (!config->legitbotFov.enabled || !config->legitbotKey.isActive())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    GameData::Lock lock;
    const auto& local = GameData::local();

    if (!local.exists || !local.alive || local.aimPunch.null())
        return;

    if (memory->input->isCameraInThirdPerson)
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    const auto& cfg = config->legitbot;

    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!cfg[weaponIndex].enabled)
        weaponIndex = weaponClass;

    if (!cfg[weaponIndex].enabled)
        weaponIndex = 0;

    if (ImVec2 pos; Helpers::worldToScreen(local.aimPunch, pos))
    {
        const auto& displaySize = ImGui::GetIO().DisplaySize;
        const auto radius = std::tan(Helpers::deg2rad(cfg[weaponIndex].fov) / (16.0f/6.0f)) / std::tan(Helpers::deg2rad(localPlayer->isScoped() ? localPlayer->fov() : (config->visuals.fov + 90.0f)) / 2.0f) * displaySize.x;
        if (radius > displaySize.x || radius > displaySize.y || !std::isfinite(radius))
            return;

        const auto color = Helpers::calculateColor(config->legitbotFov);
        drawList->AddCircleFilled(localPlayer->shotsFired() > 1 ? pos : displaySize / 2.0f, radius, color);
        if (config->legitbotFov.outline)
            drawList->AddCircle(localPlayer->shotsFired() > 1 ? pos : displaySize / 2.0f, radius, color | IM_COL32_A_MASK, 360);
    }
}

void Visuals::fullBright() noexcept
{
    static auto bright = interfaces->cvar->findVar("mat_fullbright");
    bright->setValue(config->visuals.fullBright);
}

void Visuals::playerModel(FrameStage stage) noexcept
{
    if (stage != FrameStage::NET_UPDATE_POSTDATAUPDATE_START && stage != FrameStage::RENDER_END)
        return;

    static int originalIdx = 0;

    if (!localPlayer) {
        originalIdx = 0;
        return;
    }

    constexpr auto getModel = [](Team team) constexpr noexcept -> const char* {
        constexpr std::array models{
        "models/player/custom_player/legacy/ctm_fbi_variantb.mdl",
        "models/player/custom_player/legacy/ctm_fbi_variantf.mdl",
        "models/player/custom_player/legacy/ctm_fbi_variantg.mdl",
        "models/player/custom_player/legacy/ctm_fbi_varianth.mdl",
        "models/player/custom_player/legacy/ctm_sas_variantf.mdl",
        "models/player/custom_player/legacy/ctm_st6_variante.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantg.mdl",
        "models/player/custom_player/legacy/ctm_st6_varianti.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantk.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantm.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantf.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantg.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianth.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianti.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantj.mdl",
        "models/player/custom_player/legacy/tm_leet_variantf.mdl",
        "models/player/custom_player/legacy/tm_leet_variantg.mdl",
        "models/player/custom_player/legacy/tm_leet_varianth.mdl",
        "models/player/custom_player/legacy/tm_leet_varianti.mdl",
        "models/player/custom_player/legacy/tm_phoenix_variantf.mdl",
        "models/player/custom_player/legacy/tm_phoenix_variantg.mdl",
        "models/player/custom_player/legacy/tm_phoenix_varianth.mdl",
        
        "models/player/custom_player/legacy/tm_pirate.mdl",
        "models/player/custom_player/legacy/tm_pirate_varianta.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantb.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantc.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantd.mdl",
        "models/player/custom_player/legacy/tm_anarchist.mdl",
        "models/player/custom_player/legacy/tm_anarchist_varianta.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantb.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantc.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantd.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianta.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantb.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantc.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantd.mdl",
        "models/player/custom_player/legacy/tm_balkan_variante.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_varianta.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_variantb.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_variantc.mdl",
        "models/player/custom_player/legacy/ctm_gign.mdl",
        "models/player/custom_player/legacy/ctm_gign_varianta.mdl",
        "models/player/custom_player/legacy/ctm_gign_variantb.mdl",
        "models/player/custom_player/legacy/ctm_gign_variantc.mdl",
        "models/player/custom_player/legacy/ctm_gign_variantd.mdl",
        "models/player/custom_player/legacy/tm_phoenix_varianti.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantj.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantl.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantk.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantl.mdl",
        "models/player/custom_player/legacy/ctm_swat_variante.mdl",
        "models/player/custom_player/legacy/ctm_swat_variantf.mdl",
        "models/player/custom_player/legacy/ctm_swat_variantg.mdl",
        "models/player/custom_player/legacy/ctm_swat_varianth.mdl",
        "models/player/custom_player/legacy/ctm_swat_varianti.mdl",
        "models/player/custom_player/legacy/ctm_swat_variantj.mdl",
        "models/player/custom_player/legacy/tm_professional_varf.mdl",
        "models/player/custom_player/legacy/tm_professional_varf1.mdl",
        "models/player/custom_player/legacy/tm_professional_varf2.mdl",
        "models/player/custom_player/legacy/tm_professional_varf3.mdl",
        "models/player/custom_player/legacy/tm_professional_varf4.mdl",
        "models/player/custom_player/legacy/tm_professional_varg.mdl",
        "models/player/custom_player/legacy/tm_professional_varh.mdl",
        "models/player/custom_player/legacy/tm_professional_vari.mdl",
        "models/player/custom_player/legacy/tm_professional_varj.mdl"
        };

        switch (team) {
        case Team::TT: return static_cast<std::size_t>(config->visuals.playerModelT - 1) < models.size() ? models[config->visuals.playerModelT - 1] : nullptr;
        case Team::CT: return static_cast<std::size_t>(config->visuals.playerModelCT - 1) < models.size() ? models[config->visuals.playerModelCT - 1] : nullptr;
        default: return nullptr;
        }
    };

    auto isValidModel = [](std::string name) noexcept -> bool
    {
        if (name.empty() || name.front() == ' ' || name.back() == ' ' || !name.ends_with(".mdl"))
            return false;

        if (!name.starts_with("models") && !name.starts_with("/models") && !name.starts_with("\\models"))
            return false;

        //Check if file exists within directory
        std::string path = interfaces->engine->getGameDirectory();
        if (config->visuals.playerModel[0] != '\\' && config->visuals.playerModel[0] != '/')
            path += "/";
        path += config->visuals.playerModel;

        struct stat buf;
        if (stat(path.c_str(), &buf) != -1)
            return true;

        return false;
    };
    
    const bool custom = isValidModel(static_cast<std::string>(config->visuals.playerModel));

    if (const auto model = custom ? config->visuals.playerModel : getModel(localPlayer->getTeamNumber())) {
        if (stage == FrameStage::NET_UPDATE_POSTDATAUPDATE_START) {
            originalIdx = localPlayer->modelIndex();
            if (const auto modelprecache = interfaces->networkStringTableContainer->findTable("modelprecache")) {
                const auto index = modelprecache->addString(false, model);
                if (index == -1)
                    return;

                const auto viewmodelArmConfig = memory->getPlayerViewmodelArmConfigForPlayerModel(model);
                modelprecache->addString(false, viewmodelArmConfig[2]);
                modelprecache->addString(false, viewmodelArmConfig[3]);
            }
        }

        const auto idx = stage == FrameStage::RENDER_END && originalIdx ? originalIdx : interfaces->modelInfo->getModelIndex(model);

        localPlayer->setModelIndex(idx);

        if (const auto ragdoll = interfaces->entityList->getEntityFromHandle(localPlayer->ragdoll()))
            ragdoll->setModelIndex(idx);
    }
}

void Visuals::modifySmoke(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    constexpr std::array smokeMaterials{
        "particle/vistasmokev1/vistasmokev1_emods",
        "particle/vistasmokev1/vistasmokev1_emods_impactdust",
        "particle/vistasmokev1/vistasmokev1_fire",
        "particle/vistasmokev1/vistasmokev1_smokegrenade"
    };

    for (const auto mat : smokeMaterials) {
        const auto material = interfaces->materialSystem->findMaterial(mat);
        material->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noSmoke);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, stage == FrameStage::RENDER_START && config->visuals.wireframeSmoke);
    }
}

void Visuals::modifyMolotov(FrameStage stage) noexcept
{
    constexpr std::array fireMaterials{
        "decals/molotovscorch.vmt",
        "particle/fire_burning_character/fire_env_fire.vmt",
        "particle/fire_burning_character/fire_env_fire_depthblend.vmt",
        "particle/particle_flares/particle_flare_001.vmt",
        "particle/particle_flares/particle_flare_004.vmt",
        "particle/particle_flares/particle_flare_004b_mod_ob.vmt",
        "particle/particle_flares/particle_flare_004b_mod_z.vmt",
        "particle/fire_explosion_1/fire_explosion_1_bright.vmt",
        "particle/fire_explosion_1/fire_explosion_1b.vmt",
        "particle/fire_particle_4/fire_particle_4.vmt",
        "particle/fire_explosion_1/fire_explosion_1_oriented.vmt",
        "particle/vistasmokev1/vistasmokev1_nearcull_nodepth.vmt"
    };
    
    for (const auto mat : fireMaterials) {
        const auto material = interfaces->materialSystem->findMaterial(mat);
        material->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noMolotov);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, stage == FrameStage::RENDER_START && config->visuals.wireframeMolotov);        
    }
}

void Visuals::thirdperson() noexcept
{
    if (!config->visuals.thirdperson && !config->visuals.freeCam && !memory->input->isCameraInThirdPerson)
        return;

    const bool freeCamming = config->visuals.freeCam && config->visuals.freeCamKey.isActive() && localPlayer && localPlayer->isAlive();
    const bool thirdPerson = config->visuals.thirdperson && config->visuals.thirdpersonKey.isActive() && localPlayer && localPlayer->isAlive();

    static auto distVar = interfaces->cvar->findVar("cam_idealdist");
    static auto curDist = 0.0f;

    memory->input->isCameraInThirdPerson = freeCamming || thirdPerson;
    if (!freeCamming && thirdPerson)
        curDist = Helpers::approachValueSmooth(static_cast<float>(config->visuals.thirdpersonDistance), curDist, memory->globalVars->frametime * 7.0f);
    if (freeCamming || !thirdPerson)
        curDist = 0.0f;

    distVar->setValue(curDist);
}

void Visuals::removeVisualRecoil(FrameStage stage) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    static Vector aimPunch;
    static Vector viewPunch;

    if (stage == FrameStage::RENDER_START) {
        aimPunch = localPlayer->aimPunchAngle();
        viewPunch = localPlayer->viewPunchAngle();

        if (config->visuals.noAimPunch)
            localPlayer->aimPunchAngle() = Vector{ };

        if (config->visuals.noViewPunch)
            localPlayer->viewPunchAngle() = Vector{ };

    } else if (stage == FrameStage::RENDER_END) {
        localPlayer->aimPunchAngle() = aimPunch;
        localPlayer->viewPunchAngle() = viewPunch;
    }
}

void Visuals::removeBlur(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static auto blur = interfaces->materialSystem->findMaterial("dev/scope_bluroverlay");
    blur->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noBlur);
}

void Visuals::removeGrass(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    constexpr auto getGrassMaterialName = []() noexcept -> const char* {
        switch (fnv::hashRuntime(interfaces->engine->getLevelName())) {
        case fnv::hash("dz_blacksite"): return "detail/detailsprites_survival";
        case fnv::hash("dz_sirocco"): return "detail/dust_massive_detail_sprites";
        case fnv::hash("coop_autumn"): return "detail/autumn_detail_sprites";
        case fnv::hash("dz_frostbite"): return "ski/detail/detailsprites_overgrown_ski";
        // dz_junglety has been removed in 7/23/2020 patch
        // case fnv::hash("dz_junglety"): return "detail/tropical_grass";
        default: return nullptr;
        }
    };

    if (const auto grassMaterialName = getGrassMaterialName())
        interfaces->materialSystem->findMaterial(grassMaterialName)->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noGrass);
}

void Visuals::remove3dSky() noexcept
{
    static auto sky = interfaces->cvar->findVar("r_3dsky");
    sky->setValue(!config->visuals.no3dSky);
}

void Visuals::removeShadows() noexcept
{
    static auto shadows = interfaces->cvar->findVar("cl_csm_enabled");
    shadows->setValue(!config->visuals.noShadows);
}

void Visuals::applyZoom(FrameStage stage) noexcept
{
    if (config->visuals.zoom && localPlayer) {
        if (stage == FrameStage::RENDER_START && (localPlayer->fov() == 90 || localPlayer->fovStart() == 90)) {
            if (config->visuals.zoomKey.isActive()) {
                localPlayer->fov() = 40;
                localPlayer->fovStart() = 40;
            }
        }
    }
}

#undef xor
#define DRAW_SCREEN_EFFECT(material) \
{ \
    const auto drawFunction = memory->drawScreenEffectMaterial; \
    int w, h; \
    interfaces->engine->getScreenSize(w, h); \
    __asm { \
        __asm push h \
        __asm push w \
        __asm push 0 \
        __asm xor edx, edx \
        __asm mov ecx, material \
        __asm call drawFunction \
        __asm add esp, 12 \
    } \
}

void Visuals::applyScreenEffects() noexcept
{
    if (!config->visuals.screenEffect)
        return;

    const auto material = interfaces->materialSystem->findMaterial([] {
        constexpr std::array effects{
            "effects/dronecam",
            "effects/underwater_overlay",
            "effects/healthboost",
            "effects/dangerzone_screen"
        };

        if (config->visuals.screenEffect <= 2 || static_cast<std::size_t>(config->visuals.screenEffect - 2) >= effects.size())
            return effects[0];
        return effects[config->visuals.screenEffect - 2];
    }());

    if (config->visuals.screenEffect == 1)
        material->findVar("$c0_x")->setValue(0.0f);
    else if (config->visuals.screenEffect == 2)
        material->findVar("$c0_x")->setValue(0.1f);
    else if (config->visuals.screenEffect >= 4)
        material->findVar("$c0_x")->setValue(1.0f);

    DRAW_SCREEN_EFFECT(material)
}

void Visuals::hitEffect(GameEvent* event) noexcept
{
    if (config->visuals.hitEffect && localPlayer) {
        static float lastHitTime = 0.0f;

        if (event && interfaces->engine->getPlayerForUserID(event->getInt("attacker")) == localPlayer->index()) {
            lastHitTime = memory->globalVars->realtime;
            return;
        }

        if (lastHitTime + config->visuals.hitEffectTime >= memory->globalVars->realtime) {
            constexpr auto getEffectMaterial = [] {
                static constexpr const char* effects[]{
                "effects/dronecam",
                "effects/underwater_overlay",
                "effects/healthboost",
                "effects/dangerzone_screen"
                };

                if (config->visuals.hitEffect <= 2)
                    return effects[0];
                return effects[config->visuals.hitEffect - 2];
            };

           
            auto material = interfaces->materialSystem->findMaterial(getEffectMaterial());
            if (config->visuals.hitEffect == 1)
                material->findVar("$c0_x")->setValue(0.0f);
            else if (config->visuals.hitEffect == 2)
                material->findVar("$c0_x")->setValue(0.1f);
            else if (config->visuals.hitEffect >= 4)
                material->findVar("$c0_x")->setValue(1.0f);

            DRAW_SCREEN_EFFECT(material)
        }
    }
}

void Visuals::transparentWorld(int resetType) noexcept
{
    static int asus[2] = { -1, -1 };

    if (resetType >= 0)
    {
        asus[0] = -1;
        asus[1] = -1;
    }

    if (asus[0] == config->visuals.asusWalls && asus[1] == config->visuals.asusProps)
        return;

    for (short h = interfaces->materialSystem->firstMaterial(); h != interfaces->materialSystem->invalidMaterial(); h = interfaces->materialSystem->nextMaterial(h)) {
        const auto mat = interfaces->materialSystem->getMaterial(h);

        const std::string_view textureGroup = mat->getTextureGroupName();

        if (resetType == 1)
        {
            if (textureGroup.starts_with("World"))
                mat->alphaModulate(1.0f);

            if (textureGroup.starts_with("StaticProp"))
                mat->alphaModulate(1.0f);
            continue;
        }

        if (asus[0] != config->visuals.asusWalls && textureGroup.starts_with("World"))
            mat->alphaModulate(static_cast<float>(config->visuals.asusWalls) / 100.0f);

        if (asus[1] != config->visuals.asusProps && textureGroup.starts_with("StaticProp"))
            mat->alphaModulate(static_cast<float>(config->visuals.asusProps) / 100.0f);
    }
    asus[0] = config->visuals.asusWalls;
    asus[1] = config->visuals.asusProps;
}

void Visuals::hitMarker(GameEvent* event, ImDrawList* drawList) noexcept
{
    if (config->visuals.hitMarker == 0)
        return;

    static float lastHitTime = 0.0f;

    if (event) {
        if (localPlayer && event->getInt("attacker") == localPlayer->getUserId())
            lastHitTime = memory->globalVars->realtime;
        return;
    }

    if (lastHitTime + config->visuals.hitMarkerTime < memory->globalVars->realtime)
        return;

    switch (config->visuals.hitMarker) {
    case 1:
        const auto& mid = ImGui::GetIO().DisplaySize / 2.0f;
        auto color = IM_COL32(255, 255, 255, static_cast<int>(Helpers::lerp(fabsf(lastHitTime + config->visuals.hitMarkerTime - memory->globalVars->realtime) / config->visuals.hitMarkerTime + FLT_EPSILON, 0.0f, 255.0f)));
        drawList->AddLine({ mid.x - 10, mid.y - 10 }, { mid.x - 4, mid.y - 4 }, color);
        drawList->AddLine({ mid.x + 10.5f, mid.y - 10.5f }, { mid.x + 4.5f, mid.y - 4.5f }, color);
        drawList->AddLine({ mid.x + 10.5f, mid.y + 10.5f }, { mid.x + 4.5f, mid.y + 4.5f }, color);
        drawList->AddLine({ mid.x - 10, mid.y + 10 }, { mid.x - 4, mid.y + 4 }, color);
        break;
    }
}

struct MotionBlurHistory
{
    MotionBlurHistory() noexcept
    {
        lastTimeUpdate = 0.0f;
        previousPitch = 0.0f;
        previousYaw = 0.0f;
        previousPositon = Vector{ 0.0f, 0.0f, 0.0f };
        noRotationalMotionBlurUntil = 0.0f;
    }

    float lastTimeUpdate;
    float previousPitch;
    float previousYaw;
    Vector previousPositon;
    float noRotationalMotionBlurUntil;
};

void Visuals::motionBlur(ViewSetup* setup) noexcept
{
    if (!localPlayer || !config->visuals.motionBlur.enabled)
        return;

    static MotionBlurHistory history;
    static float motionBlurValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (setup)
    {
        const float timeElapsed = memory->globalVars->realtime - history.lastTimeUpdate;

        const auto viewangles = setup->angles;

        const float currentPitch = Helpers::normalizeYaw(viewangles.x);
        const float currentYaw = Helpers::normalizeYaw(viewangles.y);

        Vector currentSideVector;
        Vector currentForwardVector;
        Vector currentUpVector;
        Vector::fromAngleAll(setup->angles, &currentForwardVector, &currentSideVector, &currentUpVector);

        Vector currentPosition = setup->origin;
        Vector positionChange = history.previousPositon - currentPosition;

        if ((positionChange.length() > 30.0f) && (timeElapsed >= 0.5f))
        {
            motionBlurValues[0] = 0.0f;
            motionBlurValues[1] = 0.0f;
            motionBlurValues[2] = 0.0f;
            motionBlurValues[3] = 0.0f;
        }
        else if (timeElapsed > (1.0f / 15.0f))
        {
            motionBlurValues[0] = 0.0f;
            motionBlurValues[1] = 0.0f;
            motionBlurValues[2] = 0.0f;
            motionBlurValues[3] = 0.0f;
        }
        else if (positionChange.length() > 50.0f)
        {
            history.noRotationalMotionBlurUntil = memory->globalVars->realtime + 1.0f;
        }
        else
        {
            const float horizontalFov = setup->fov;
            const float verticalFov = (setup->aspectRatio <= 0.0f) ? (setup->fov) : (setup->fov / setup->aspectRatio);
            const float viewdotMotion = currentForwardVector.dotProduct(positionChange);

            if (config->visuals.motionBlur.forwardEnabled)
                motionBlurValues[2] = viewdotMotion;

            const float sidedotMotion = currentSideVector.dotProduct(positionChange);
            float yawdiffOriginal = history.previousYaw - currentYaw;
            if (((history.previousYaw - currentYaw > 180.0f) || (history.previousYaw - currentYaw < -180.0f)) &&
                ((history.previousYaw + currentYaw > -180.0f) && (history.previousYaw + currentYaw < 180.0f)))
                yawdiffOriginal = history.previousYaw + currentYaw;

            float yawdiffAdjusted = yawdiffOriginal + (sidedotMotion / 3.0f);

            if (yawdiffOriginal < 0.0f)
                yawdiffAdjusted = std::clamp(yawdiffAdjusted, yawdiffOriginal, 0.0f);
            else
                yawdiffAdjusted = std::clamp(yawdiffAdjusted, 0.0f, yawdiffOriginal);

            const float undampenedYaw = yawdiffAdjusted / horizontalFov;
            motionBlurValues[0] = undampenedYaw * (1.0f - (fabsf(currentPitch) / 90.0f));

            const float pitchCompensateMask = 1.0f - ((1.0f - fabsf(currentForwardVector[2])) * (1.0f - fabsf(currentForwardVector[2])));
            const float pitchdiffOriginal = history.previousPitch - currentPitch;
            float pitchdiffAdjusted = pitchdiffOriginal;

            if (currentPitch > 0.0f)
                pitchdiffAdjusted = pitchdiffOriginal - ((viewdotMotion / 2.0f) * pitchCompensateMask);
            else
                pitchdiffAdjusted = pitchdiffOriginal + ((viewdotMotion / 2.0f) * pitchCompensateMask);


            if (pitchdiffOriginal < 0.0f)
                pitchdiffAdjusted = std::clamp(pitchdiffAdjusted, pitchdiffOriginal, 0.0f);
            else
                pitchdiffAdjusted = std::clamp(pitchdiffAdjusted, 0.0f, pitchdiffOriginal);

            motionBlurValues[1] = pitchdiffAdjusted / verticalFov;
            motionBlurValues[3] = undampenedYaw;
            motionBlurValues[3] *= (fabs(currentPitch) / 90.0f) * (fabs(currentPitch) / 90.0f) * (fabs(currentPitch) / 90.0f);

            if (timeElapsed > 0.0f)
                motionBlurValues[2] /= timeElapsed * 30.0f;
            else
                motionBlurValues[2] = 0.0f;

            motionBlurValues[2] = std::clamp((fabsf(motionBlurValues[2]) - config->visuals.motionBlur.fallingMin) / (config->visuals.motionBlur.fallingMax - config->visuals.motionBlur.fallingMin), 0.0f, 1.0f) * (motionBlurValues[2] >= 0.0f ? 1.0f : -1.0f);
            motionBlurValues[2] /= 30.0f;
            motionBlurValues[0] *= config->visuals.motionBlur.rotationIntensity * .15f * config->visuals.motionBlur.strength;
            motionBlurValues[1] *= config->visuals.motionBlur.rotationIntensity * .15f * config->visuals.motionBlur.strength;
            motionBlurValues[2] *= config->visuals.motionBlur.rotationIntensity * .15f * config->visuals.motionBlur.strength;
            motionBlurValues[3] *= config->visuals.motionBlur.fallingIntensity * .15f * config->visuals.motionBlur.strength;

        }

        if (memory->globalVars->realtime < history.noRotationalMotionBlurUntil)
        {
            motionBlurValues[0] = 0.0f;
            motionBlurValues[1] = 0.0f;
            motionBlurValues[3] = 0.0f;
        }
        else
        {
            history.noRotationalMotionBlurUntil = 0.0f;
        }
        history.previousPositon = currentPosition;

        history.previousPitch = currentPitch;
        history.previousYaw = currentYaw;
        history.lastTimeUpdate = memory->globalVars->realtime;
        return;
    }

    const auto material = interfaces->materialSystem->findMaterial("dev/motion_blur", "RenderTargets", false);
    if (!material)
        return;

    const auto MotionBlurInternal = material->findVar("$MotionBlurInternal", nullptr, false);

    MotionBlurInternal->setVecComponentValue(motionBlurValues[0], 0);
    MotionBlurInternal->setVecComponentValue(motionBlurValues[1], 1);
    MotionBlurInternal->setVecComponentValue(motionBlurValues[2], 2);
    MotionBlurInternal->setVecComponentValue(motionBlurValues[3], 3);

    const auto MotionBlurViewPortInternal = material->findVar("$MotionBlurViewportInternal", nullptr, false);

    MotionBlurViewPortInternal->setVecComponentValue(0.0f, 0);
    MotionBlurViewPortInternal->setVecComponentValue(0.0f, 1);
    MotionBlurViewPortInternal->setVecComponentValue(1.0f, 2);
    MotionBlurViewPortInternal->setVecComponentValue(1.0f, 3);

    DRAW_SCREEN_EFFECT(material)
}

void Visuals::disablePostProcessing(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    *memory->disablePostProcessing = stage == FrameStage::RENDER_START && config->visuals.disablePostProcessing;
}

bool Visuals::removeHands(const char* modelName) noexcept
{
    return config->visuals.noHands && std::strstr(modelName, "arms") && !std::strstr(modelName, "sleeve");
}

bool Visuals::removeSleeves(const char* modelName) noexcept
{
    return config->visuals.noSleeves && std::strstr(modelName, "sleeve");
}

bool Visuals::removeWeapons(const char* modelName) noexcept
{
    return config->visuals.noWeapons && std::strstr(modelName, "models/weapons/v_")
        && !std::strstr(modelName, "arms") && !std::strstr(modelName, "tablet")
        && !std::strstr(modelName, "parachute") && !std::strstr(modelName, "fists");
}

void Visuals::skybox(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    if (stage == FrameStage::RENDER_START && config->visuals.skybox > 0 && static_cast<std::size_t>(config->visuals.skybox) < skyboxList.size() - 1) {
        memory->loadSky(skyboxList[config->visuals.skybox]);
    } else if (config->visuals.skybox == 26 && stage == FrameStage::RENDER_START) {
        memory->loadSky(config->visuals.customSkybox.c_str());
    } else {
        static const auto sv_skyname = interfaces->cvar->findVar("sv_skyname");
        memory->loadSky(sv_skyname->string);
    }
}
struct shotRecords
{
    shotRecords(Vector eyePosition, float time) noexcept
    {
        this->eyePosition = eyePosition;
        this->time = time;
    }
    Vector eyePosition;
    bool gotImpact{ false };
    float time{ 0.0f };
};

std::deque<shotRecords> shotRecord;

void Visuals::updateShots(UserCmd* cmd) noexcept
{
    if (!config->visuals.bulletTracers.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive())
    {
        shotRecord.clear();
        return;
    }

    if (!(cmd->buttons & UserCmd::IN_ATTACK))
        return;

    if (localPlayer->nextAttack() > memory->globalVars->serverTime() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!*memory->gameRules || (*memory->gameRules)->freezePeriod())
        return;

    if (localPlayer->flags() & (1 << 6)) //Frozen
        return;

    shotRecord.push_back(shotRecords(localPlayer->getEyePosition(), memory->globalVars->serverTime()));

    while (!shotRecord.empty() && fabsf(shotRecord.front().time - memory->globalVars->serverTime()) > 1.0f)
        shotRecord.pop_front();
}

void Visuals::footstepESP(GameEvent* event) noexcept
{
    if (!config->visuals.footsteps.footstepBeams.enabled)
        return;

    const auto entity = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));

    if (!entity || !localPlayer.get() || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || !entity->isOtherEnemy(localPlayer.get()))
        return;

    if (entity->getAbsOrigin() == localPlayer.get()->getAbsOrigin()) // fix for weird bug
        return;

    /*
    @note: gr1ndy - other sprites that you can use:
    "sprites/physbeam",
    "sprites/purplelaser1",
    "sprites/white.vmt", <-- draws behind the wall
    */
    
    const auto modelIndex = interfaces->modelInfo->getModelIndex("sprites/purplelaser1.vmt");

    BeamInfo info;

    info.type = TE_BEAMRINGPOINT;
    info.modelName = "sprites/purplelaser1.vmt";
    info.modelIndex = modelIndex;
    info.haloIndex = -1;
    info.haloScale = 0.0f;
    info.life = 2.0f;
    info.width = static_cast<float>(config->visuals.footsteps.footstepBeamThickness);
    info.fadeLength = 0.0f;
    info.amplitude = 0.0f;
    info.red = config->visuals.footsteps.footstepBeams.color[0] * 255;
    info.green = config->visuals.footsteps.footstepBeams.color[1] * 255;
    info.blue = config->visuals.footsteps.footstepBeams.color[2] * 255;
    info.brightness = 255;
    info.speed = 0.0f;
    info.startFrame = 0;
    info.frameRate = 60.0f;
    info.segments = 1;
    info.flags = FBEAM_FADEOUT;
    info.ringCenter = entity->getAbsOrigin() + Vector(0.0f, 0.0f, 5.0f);
    info.ringStartRadius = 0.0f;
    info.ringEndRadius = static_cast<float>(config->visuals.footsteps.footstepBeamRadius);
    info.renderable = true;

    const auto beamDraw = memory->viewRenderBeams->createBeamRingPoints(info);

    if (beamDraw)
        memory->viewRenderBeams->drawBeam(beamDraw);
}

void Visuals::bulletTracer(GameEvent& event) noexcept
{
    if (!config->visuals.bulletTracers.enabled)
    {
        shotRecord.clear();
        return;
    }

    if (!localPlayer || shotRecord.empty())
        return;

    switch (fnv::hashRuntime(event.getName())) {
    case fnv::hash("round_start"):
        shotRecord.clear();
            break;
    case fnv::hash("weapon_fire"):
        if (event.getInt("userid") != localPlayer->getUserId())
            return;

        if (shotRecord.front().gotImpact)
            shotRecord.pop_front();
        break;
    case fnv::hash("bullet_impact"):
    {
        if (shotRecord.front().gotImpact)
            return;

        if (event.getInt("userid") != localPlayer->getUserId())
            return;

        if (shotRecord.front().eyePosition.null())
        {
            shotRecord.pop_front();
            return;
        }

        const auto bulletImpact = Vector{ event.getFloat("x"),  event.getFloat("y"),  event.getFloat("z") };
        const auto angle = AimbotFunction::calculateRelativeAngle(shotRecord.front().eyePosition, bulletImpact, Vector{ });
        const auto end = bulletImpact + Vector::fromAngle(angle) * 2000.f;

        BeamInfo beamInfo;

        beamInfo.start = shotRecord.front().eyePosition;
        beamInfo.end = end;

        /*
        @note: gr1ndy - other sprites that you can use:
        "sprites/blueglow1",
        "sprites/bubble",
        "sprites/glow01",
        "sprites/physbeam",
        "sprites/purpleglow1",
        "sprites/purplelaser1",
        "sprites/radio",
        "sprites/white",
        */
        
        beamInfo.modelName = "sprites/purplelaser1.vmt";
        beamInfo.modelIndex = -1;
        beamInfo.haloName = nullptr;
        beamInfo.haloIndex = -1;

        beamInfo.red = 255.0f * config->visuals.bulletTracers.color[0];
        beamInfo.green = 255.0f * config->visuals.bulletTracers.color[1];
        beamInfo.blue = 255.0f * config->visuals.bulletTracers.color[2];
        beamInfo.brightness = 255.0f * config->visuals.bulletTracers.color[3];

        beamInfo.type = TE_BEAMPOINTS;
        //beamInfo.life = 0.0f;
        beamInfo.amplitude = 0.0f;
        beamInfo.segments = -1;
        beamInfo.renderable = true;
        beamInfo.speed = 0.0f;
        beamInfo.startFrame = 0;
        beamInfo.frameRate = 0.0f;
        beamInfo.width = 2.0f;
        beamInfo.endWidth = 2.0f;
        //beamInfo.flags = 0x40;
        beamInfo.fadeLength = 20.0f;

        if (const auto beam = memory->viewRenderBeams->createBeamPoints(beamInfo)) {
            //constexpr auto FBEAM_FOREVER = 0x4000;
            beam->flags = FBEAM_FADEOUT | FBEAM_HALOBEAM;
            beam->die = memory->globalVars->currenttime + 2.0f;
        }
        shotRecord.front().gotImpact = true;
    }
    }
}

//Why 2 functions when you can do it in 1?, because it causes a crash doing it in 1 function

static std::deque<Vector> positions;

void Visuals::drawBulletImpacts() noexcept
{
    if (!config->visuals.bulletImpacts.enabled)
        return;

    if (!localPlayer)
        return;

    if (!interfaces->debugOverlay)
        return;

    const int r = static_cast<int>(config->visuals.bulletImpacts.color[0] * 255.f);
    const int g = static_cast<int>(config->visuals.bulletImpacts.color[1] * 255.f);
    const int b = static_cast<int>(config->visuals.bulletImpacts.color[2] * 255.f);
    const int a = static_cast<int>(config->visuals.bulletImpacts.color[3] * 255.f);

    for (int i = 0; i < static_cast<int>(positions.size()); i++)
    {
        if (!positions.at(i).notNull())
            continue;
        interfaces->debugOverlay->boxOverlay(positions.at(i), Vector{ -2.0f, -2.0f, -2.0f }, Vector{ 2.0f, 2.0f, 2.0f }, Vector{ 0.0f, 0.0f, 0.0f }, r, g, b, a, config->visuals.bulletImpactsTime);
    }
    positions.clear();
}

void Visuals::bulletImpact(GameEvent& event) noexcept
{
    if (!config->visuals.bulletImpacts.enabled)
        return;

    if (!localPlayer)
        return;

    if (event.getInt("userid") != localPlayer->getUserId())
        return;

    Vector endPos = Vector{ event.getFloat("x"), event.getFloat("y"), event.getFloat("z") };
    positions.push_front(endPos);
}

void Visuals::drawHitboxMatrix(GameEvent* event) noexcept {

    if (!config->visuals.onHitHitbox.color.enabled) return;

    if (config->visuals.onHitHitbox.duration <= 0.f) return;

    if (!event) return;

    if (!localPlayer) return;

    const auto userID = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));
    const auto attacker = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("attacker")));

    if (!userID) return;

    if (!attacker) return;

    if (localPlayer->getUserId() != attacker->getUserId() || localPlayer->getUserId() == userID->getUserId()) return;

    StudioHdr* hdr = interfaces->modelInfo->getStudioModel(userID->getModel());
    StudioHitboxSet* set = hdr->getHitboxSet(0);

    auto records = Animations::getBacktrackRecords(userID->index());
    const matrix3x4* matrix = userID->getBoneCache().memory;
    auto bestFov{ 255.f };

    if (records && !records->empty()) {
        for (int i = static_cast<int>(records->size() - 1); i >= 0; i--)
        {
            if (Backtrack::valid(records->at(i).simulationTime))
            {
                for (auto position : records->at(i).positions) {
                    auto angle = AimbotFunction::calculateRelativeAngle(localPlayer->getEyePosition(), position, interfaces->engine->getViewAngles());
                    auto fov = std::hypotf(angle.x, angle.y);
                    if (fov < bestFov) {
                        bestFov = fov;
                        matrix = records->at(i).matrix;
                    }
                }
            }
        }

    }

    const int r = static_cast<int>(config->visuals.onHitHitbox.color.color[0] * 255.f);
    const int g = static_cast<int>(config->visuals.onHitHitbox.color.color[1] * 255.f);
    const int b = static_cast<int>(config->visuals.onHitHitbox.color.color[2] * 255.f);
    const int a = static_cast<int>(config->visuals.onHitHitbox.color.color[3] * 255.f);
    const float d = config->visuals.onHitHitbox.duration;

    for (int i = 0; i < set->numHitboxes; i++) {
        StudioBbox* hitbox = set->getHitbox(i);

        if (!hitbox)
            continue;

        Vector vMin = hitbox->bbMin.transform(matrix[hitbox->bone]);
        Vector vMax = hitbox->bbMax.transform(matrix[hitbox->bone]);
        float size = hitbox->capsuleRadius;

        interfaces->debugOverlay->capsuleOverlay(vMin, vMax, size <= 0 ? 3.f : hitbox->capsuleRadius, r, g, b, a, d, 0, 1);
    }
}

void Visuals::drawMolotovHull(ImDrawList* drawList) noexcept
{
    if (!config->visuals.molotovHull.enabled)
        return;

    const auto color = Helpers::calculateColor(config->visuals.molotovHull);

    GameData::Lock lock;

    static const auto flameCircumference = [] {
        std::array<Vector, 72> points;
        for (std::size_t i = 0; i < points.size(); ++i) {
            constexpr auto flameRadius = 60.0f; // https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/cstrike15/Effects/inferno.cpp#L889
            points[i] = Vector{ flameRadius * std::cos(Helpers::deg2rad(i * (360.0f / points.size()))),
                                flameRadius * std::sin(Helpers::deg2rad(i * (360.0f / points.size()))),
                                0.0f };
        }
        return points;
    }();

    for (const auto& molotov : GameData::infernos()) {
        for (const auto& pos : molotov.points) {
            std::array<ImVec2, flameCircumference.size()> screenPoints;
            std::size_t count = 0;

            for (const auto& point : flameCircumference) {
                if (Helpers::worldToScreen(pos + point, screenPoints[count]))
                    ++count;
            }

            if (count < 1)
                continue;

            std::swap(screenPoints[0], *std::min_element(screenPoints.begin(), screenPoints.begin() + count, [](const auto& a, const auto& b) { return a.y < b.y || (a.y == b.y && a.x < b.x); }));

            constexpr auto orientation = [](const ImVec2& a, const ImVec2& b, const ImVec2& c) {
                return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
            };

            std::sort(screenPoints.begin() + 1, screenPoints.begin() + count, [&](const auto& a, const auto& b) { return orientation(screenPoints[0], a, b) > 0.0f; });
            drawList->AddConvexPolyFilled(screenPoints.data(), count, color);
        }
    }
}

void Visuals::drawSmokeHull(ImDrawList* drawList) noexcept
{
    if (!config->visuals.smokeHull.enabled)
        return;

    const auto color = Helpers::calculateColor(config->visuals.smokeHull);

    GameData::Lock lock;

    static const auto smokeCircumference = [] {
        std::array<Vector, 72> points;
        for (std::size_t i = 0; i < points.size(); ++i) {
            constexpr auto smokeRadius = 150.0f; // https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/cstrike15/Effects/inferno.cpp#L889
            points[i] = Vector{ smokeRadius * std::cos(Helpers::deg2rad(i * (360.0f / points.size()))),
                                smokeRadius* std::sin(Helpers::deg2rad(i * (360.0f / points.size()))),
                                0.0f };
        }
        return points;
    }();

    for (const auto& smoke : GameData::smokes())
    {
        std::array<ImVec2, smokeCircumference.size()> screenPoints;
        std::size_t count = 0;

        for (const auto& point : smokeCircumference)
        {
            if (Helpers::worldToScreen(smoke.origin + point, screenPoints[count]))
                ++count;
        }

        if (count < 1)
            continue;

        std::swap(screenPoints[0], *std::min_element(screenPoints.begin(), screenPoints.begin() + count, [](const auto& a, const auto& b) { return a.y < b.y || (a.y == b.y && a.x < b.x); }));

        constexpr auto orientation = [](const ImVec2& a, const ImVec2& b, const ImVec2& c)
        {
            return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
        };

        std::sort(screenPoints.begin() + 1, screenPoints.begin() + count, [&](const auto& a, const auto& b) { return orientation(screenPoints[0], a, b) > 0.0f; });
        drawList->AddConvexPolyFilled(screenPoints.data(), count, color);
    }
}


// Used to sort Vectors in ccw order about a pivot.
static float ccw(const Vector& a, const Vector& b, const Vector& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

struct ccwSorter {
    const Vector& pivot;

    ccwSorter(const Vector& inPivot) : pivot(inPivot) { }

    bool operator()(const Vector& a, const Vector& b) {
        return ccw(pivot, a, b) < 0;
    }
};

static bool isLeftOf(const Vector& a, const Vector& b) {
    return (a.x < b.x || (a.x == b.x && a.y < b.y));
}

static std::vector<Vector> gift_wrapping(std::vector<Vector> v) {
    std::vector<Vector> hull;

    // There must be at least 3 points
    if (v.size() < 3)
        return hull;

    // Move the leftmost Vector to the beginning of our vector.
    // It will be the first Vector in our convext hull.
    std::swap(v[0], *min_element(v.begin(), v.end(), isLeftOf));

    // Repeatedly find the first ccw Vector from our last hull Vector
    // and put it at the front of our array. 
    // Stop when we see our first Vector again.
    do {
        hull.push_back(v[0]);
        std::swap(v[0], *min_element(v.begin() + 1, v.end(), ccwSorter(v[0])));
    } while (v[0].x != hull[0].x && v[0].y != hull[0].y);

    return hull;
}


void Visuals::drawMolotovPolygon(ImDrawList* drawList) noexcept
{
    if (!config->visuals.molotovPolygon.enabled)
        return;

    ImColor enemy = Helpers::calculateColor(config->visuals.molotovPolygon.enemy);
    ImColor team = Helpers::calculateColor(config->visuals.molotovPolygon.team);
    ImColor self = Helpers::calculateColor(config->visuals.molotovPolygon.self);

    constexpr float pi = std::numbers::pi_v<float>;

    GameData::Lock lock;

    /* add the inferno position with largest possible inferno width so it's showing accurate radius. */
    auto flameCircumference = [](std::vector<Vector> points)
    {
        std::vector<Vector> new_points;

        for (size_t i = 0; i < points.size(); ++i)
        {
            const auto& pos = points[i];

            for (int j = 0; j <= 3; j++)
            {
                float p = j * (360.0f / 4.0f) * (pi / 200.0f);
                new_points.emplace_back(pos + Vector(std::cos(p) * 60.f, std::sin(p) * 60.f, 0.f));
            }
        }

        return new_points;
    };

    for (const auto& molotov : GameData::infernos()) 
    {
        const auto color = !molotov.owner || molotov.owner->isOtherEnemy(localPlayer.get()) ? enemy :
            molotov.owner->index() != localPlayer->index() ? team : self;

        /* we only wanted to draw the points on the edge, use giftwrap algorithm. */
        std::vector<Vector> giftWrapped = gift_wrapping(flameCircumference(molotov.points));

        /* transforms world position to screen position. */
        std::vector<ImVec2> points;

        for (size_t i = 0; i < giftWrapped.size(); ++i)
        {
            const auto& pos = giftWrapped[i];

            auto screen_pos = ImVec2();
            if (!Helpers::worldToScreen(pos, screen_pos))
                continue;

            points.emplace_back(ImVec2(screen_pos.x, screen_pos.y));
        }

        drawList->AddConvexPolyFilled(points.data(), points.size(), color);
    }
}

void Visuals::updateEventListeners(bool forceRemove) noexcept
{
    class ImpactEventListener : public GameEventListener {
    public:
        void fireGameEvent(GameEvent* event) { 
            bulletTracer(*event); 
            bulletImpact(*event);
        }
    };

    class FootstepEventListener : public GameEventListener {
    public:
        void fireGameEvent(GameEvent* event) {
            footstepESP(event);
        }
    };

    static ImpactEventListener impactListener;
    static bool impactListenerRegistered = false;

    if ((config->visuals.bulletImpacts.enabled || config->visuals.bulletTracers.enabled) && !impactListenerRegistered) {
        interfaces->gameEventManager->addListener(&impactListener, "bullet_impact");
        impactListenerRegistered = true;
    } else if (((!config->visuals.bulletImpacts.enabled && !config->visuals.bulletTracers.enabled) || forceRemove) && impactListenerRegistered) {
        interfaces->gameEventManager->removeListener(&impactListener);
        impactListenerRegistered = false;
    }

    static FootstepEventListener footstepListener;
    static bool footstepListenerRegistered = false;

    if (config->visuals.footsteps.footstepBeams.enabled && !footstepListenerRegistered) {
        interfaces->gameEventManager->addListener(&footstepListener, "player_footstep");
        footstepListenerRegistered = true;
    }
    else if (((!config->visuals.footsteps.footstepBeams.enabled) || forceRemove) && footstepListenerRegistered) {
        interfaces->gameEventManager->removeListener(&footstepListener);
        footstepListenerRegistered = false;
    }
}

void Visuals::updateInput() noexcept
{
    config->visuals.freeCamKey.handleToggle();
    config->visuals.thirdpersonKey.handleToggle();
    config->visuals.zoomKey.handleToggle();
}

void Visuals::reset(int resetType) noexcept
{
    shotRecord.clear();
    Visuals::transparentWorld(resetType);
    if (resetType == 1)
    {
        //Reset convars
        static auto bright = interfaces->cvar->findVar("mat_fullbright");
        static auto sky = interfaces->cvar->findVar("r_3dsky");
        static auto shadows = interfaces->cvar->findVar("cl_csm_enabled");
        static auto cl_csm_rot_override = interfaces->cvar->findVar("cl_csm_rot_override");
        bright->setValue(0);
        sky->setValue(1);
        shadows->setValue(1);
        cl_csm_rot_override->setValue(0);

        //Disable thirdperson/freecam
        const bool freeCamming = config->visuals.freeCam && config->visuals.freeCamKey.isActive() && localPlayer && localPlayer->isAlive();
        const bool thirdPerson = config->visuals.thirdperson && config->visuals.thirdpersonKey.isActive() && localPlayer && localPlayer->isAlive();
        if (freeCamming || thirdPerson)
            memory->input->isCameraInThirdPerson = false;
    }
}
