#include "AnimState.h"
#include "Entity.h"

void AnimState::setupVelocity() noexcept
{
    interfaces->mdlCache->beginLock();

    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    Vector vecAbsVelocity = entity->getAbsVelocity();

    // prevent the client input velocity vector from exceeding a reasonable magnitude
    if (vecAbsVelocity.squareLength() > std::sqrt(CS_PLAYER_SPEED_RUN * CSGO_ANIM_MAX_VEL_LIMIT))
        vecAbsVelocity = vecAbsVelocity.normalized() * (CS_PLAYER_SPEED_RUN * CSGO_ANIM_MAX_VEL_LIMIT);

    // save vertical velocity component
    m_flVelocityLengthZ = vecAbsVelocity.z;

    // discard z component
    vecAbsVelocity.z = 0.f;

    // remember if the player is accelerating.
    m_bPlayerIsAccelerating = (m_vecVelocityLast.squareLength() < vecAbsVelocity.squareLength());

    // rapidly approach ideal velocity instead of instantly adopt it. This helps smooth out instant velocity changes, like
    // when the player runs headlong into a wall and their velocity instantly becomes zero.
    m_vecVelocity = Helpers::approach(vecAbsVelocity, m_vecVelocity, m_flLastUpdateIncrement * 2000.f);
    m_vecVelocityNormalized = m_vecVelocity.normalized();

    // save horizontal velocity length
    m_flVelocityLengthXY = std::fminf(m_vecVelocity.length(), CS_PLAYER_SPEED_RUN);

    if (m_flVelocityLengthXY > 0.f)
        m_vecVelocityNormalizedNonZero = m_vecVelocityNormalized;

    auto weapon = reinterpret_cast<Entity*>(m_pWeapon);
    float flMaxSpeedRun = weapon ? std::fmaxf(weapon->getMaxSpeed(), 0.001f) : CS_PLAYER_SPEED_RUN;

    //compute speed in various normalized forms
    m_flSpeedAsPortionOfRunTopSpeed = std::clamp(m_flVelocityLengthXY / flMaxSpeedRun, 0.f, 1.f);
    m_flSpeedAsPortionOfWalkTopSpeed = m_flVelocityLengthXY / (flMaxSpeedRun * CS_PLAYER_SPEED_WALK_MODIFIER);
    m_flSpeedAsPortionOfCrouchTopSpeed = m_flVelocityLengthXY / (flMaxSpeedRun * CS_PLAYER_SPEED_DUCK_MODIFIER);

    if (m_flSpeedAsPortionOfWalkTopSpeed >= 1.f)
    {
        m_flStaticApproachSpeed = m_flVelocityLengthXY;
    }
    else if (m_flSpeedAsPortionOfWalkTopSpeed < 0.5f)
    {
        m_flStaticApproachSpeed = Helpers::approach(80.f, m_flStaticApproachSpeed, m_flLastUpdateIncrement * 60.f);
    }

    bool bStartedMovingThisFrame = false;
    bool bStoppedMovingThisFrame = false;

    if (m_flVelocityLengthXY > 0.f)
    {
        bStartedMovingThisFrame = (m_flDurationMoving <= 0.f);
        m_flDurationStill = 0.f;
        m_flDurationMoving += m_flLastUpdateIncrement;
    }
    else
    {
        bStoppedMovingThisFrame = (m_flDurationStill <= 0.f);
        m_flDurationMoving = 0.f;
        m_flDurationStill += m_flLastUpdateIncrement;
    }

    if (!m_bAdjustStarted && bStoppedMovingThisFrame && m_bOnGround && !m_bOnLadder && !m_bLanding && m_flStutterStep < 50)
    {
        setLayerSequence(ANIMATION_LAYER_ADJUST, ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING);
        m_bAdjustStarted = true;
    }

    if (getLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ||
        getLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
    {
        if (m_bAdjustStarted && m_flSpeedAsPortionOfCrouchTopSpeed <= 0.25f)
        {
            incrementLayerCycleWeightRateGeneric(ANIMATION_LAYER_ADJUST);
            m_bAdjustStarted = !(isLayerSequenceCompleted(ANIMATION_LAYER_ADJUST));
        }
        else
        {
            m_bAdjustStarted = false;
            float flWeight = getLayerWeight(ANIMATION_LAYER_ADJUST);
            setLayerWeight(ANIMATION_LAYER_ADJUST, Helpers::approach(0.f, flWeight, m_flLastUpdateIncrement * 5.f));
            setLayerWeightRate(ANIMATION_LAYER_ADJUST, flWeight);
        }
    }

    m_flFootYawLast = m_flFootYaw;
    m_flFootYaw = std::clamp(m_flFootYaw, -360.0f, 360.0f);
    float flEyeFootDelta = Helpers::angleDiff(m_flEyeYaw, m_flFootYaw);

    float flAimMatrixWidthRange = Helpers::lerp(std::clamp(m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f), 1.0f, Helpers::lerp(m_flWalkToRunTransition, CSGO_ANIM_AIM_NARROW_WALK, CSGO_ANIM_AIM_NARROW_RUN));

    if (m_flAnimDuckAmount > 0.f)
    {
        flAimMatrixWidthRange = Helpers::lerp(m_flAnimDuckAmount * std::clamp(m_flSpeedAsPortionOfCrouchTopSpeed, 0.f, 1.f), flAimMatrixWidthRange, CSGO_ANIM_AIM_NARROW_CROUCHMOVING);
    }

    float flTempYawMax = m_flAimYawMax * flAimMatrixWidthRange;
    float flTempYawMin = m_flAimYawMin * flAimMatrixWidthRange;

    if (flEyeFootDelta > flTempYawMax)
    {
        m_flFootYaw = m_flEyeYaw - fabs(flTempYawMax);
    }
    else if (flEyeFootDelta < flTempYawMin)
    {
        m_flFootYaw = m_flEyeYaw + fabs(flTempYawMin);
    }
    m_flFootYaw = Helpers::angleNormalize(m_flFootYaw);

    if (m_flVelocityLengthXY > 0.1f || fabs(m_flVelocityLengthZ) > 100.0f)
    {
        m_flFootYaw = Helpers::approachAngle(m_flEyeYaw, m_flFootYaw, m_flLastUpdateIncrement * (30.0f + 20.0f * m_flWalkToRunTransition));
        m_flLowerBodyRealignTimer = m_flLastUpdateTime + (CSGO_ANIM_LOWER_REALIGN_DELAY * 0.2f);
        if (entity->lby() != m_flEyeYaw)
            entity->lby() = m_flEyeYaw;
    }
    else
    {
        m_flFootYaw = Helpers::approachAngle(entity->lby(), m_flFootYaw, m_flLastUpdateIncrement * CSGO_ANIM_LOWER_CATCHUP_IDLE);

        if (m_flLastUpdateTime > m_flLowerBodyRealignTimer && fabsf(Helpers::angleDiff(m_flFootYaw, m_flEyeYaw)) > 35.0f)
        {
            m_flLowerBodyRealignTimer = m_flLastUpdateTime + CSGO_ANIM_LOWER_REALIGN_DELAY;
            if (entity->lby() != m_flEyeYaw)
                entity->lby() = m_flEyeYaw;
        }
    }

    if (m_flVelocityLengthXY <= CS_PLAYER_SPEED_STOPPED && m_bOnGround && !m_bOnLadder && !m_bLanding && m_flLastUpdateIncrement > 0.f && std::fabsf(Helpers::angleDiff(m_flFootYawLast, m_flFootYaw) / m_flLastUpdateIncrement > 120.f))
    {
        setLayerSequence(ANIMATION_LAYER_ADJUST, ACT_CSGO_IDLE_TURN_BALANCEADJUST);
        m_bAdjustStarted = true;
    }

    if (getLayerWeight(ANIMATION_LAYER_ADJUST) > 0.f)
    {
        incrementLayerCycle(ANIMATION_LAYER_ADJUST, false);
        incrementLayerWeight(ANIMATION_LAYER_ADJUST);
    }

    // the final model render yaw is aligned to the foot yaw
    if (m_flVelocityLengthXY > 0.f)
    {
        // convert horizontal velocity vec to angular yaw
        float flRawYawIdeal = (std::atan2(-m_vecVelocity.y, -m_vecVelocity.x) * 180 / 3.141592654f);
        if (flRawYawIdeal < 0.f)
            flRawYawIdeal += 360.f;

        m_flMoveYawIdeal = Helpers::angleNormalize(Helpers::angleDiff(flRawYawIdeal, m_flFootYaw));
    }

    // delta between current yaw and ideal velocity derived target (possibly negative!)
    m_flMoveYawCurrentToIdeal = Helpers::angleNormalize(Helpers::angleDiff(m_flMoveYawIdeal, m_flMoveYaw));

    if (bStartedMovingThisFrame && m_flMoveWeight <= 0.f)
    {
        m_flMoveYaw = m_flMoveYawIdeal;

        // select a special starting cycle that's set by the animator in content
        int nMoveSeq = getLayerSequence(ANIMATION_LAYER_MOVEMENT_MOVE);
        if (nMoveSeq != -1)
        {
            StudioSeqdesc seqdesc = entity->getModelPtr()->seqdesc(nMoveSeq);
            if (seqdesc.numAnimTags > 0)
            {
                if (fabsf(Helpers::angleDiff(m_flMoveYaw, 180)) <= EIGHT_WAY_WIDTH) //N
                {
                    m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_N);
                }
                else if (fabsf(Helpers::angleDiff(m_flMoveYaw, 135)) <= EIGHT_WAY_WIDTH) //NE
                {
                    m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_NE);
                }
                else if (fabsf(Helpers::angleDiff(m_flMoveYaw, 90)) <= EIGHT_WAY_WIDTH) //E
                {
                    m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_E);
                }
                else if (fabsf(Helpers::angleDiff(m_flMoveYaw, 45)) <= EIGHT_WAY_WIDTH) //SE
                {
                    m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_SE);
                }
                else if (fabsf(Helpers::angleDiff(m_flMoveYaw, 0)) <= EIGHT_WAY_WIDTH) //S
                {
                    m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_S);
                }
                else if (fabsf(Helpers::angleDiff(m_flMoveYaw, -45)) <= EIGHT_WAY_WIDTH) //SW
                {
                    m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_SW);
                }
                else if (fabsf(Helpers::angleDiff(m_flMoveYaw, -90)) <= EIGHT_WAY_WIDTH) //W
                {
                    m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_W);
                }
                else if (fabsf(Helpers::angleDiff(m_flMoveYaw, -135)) <= EIGHT_WAY_WIDTH) //NW
                {
                    m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_NW);
                }
            }
        }

        /*
        if (m_flInAirSmoothValue >= 1 && !m_bFirstRunSinceInit && abs(m_flMoveYawCurrentToIdeal) > 45 && m_bOnGround && entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].GetCurrentWeight() <= 0)
        {
            entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].SetShouldCapture(bonesnapshot_get(cl_bonesnapshot_speed_movebegin));
        }
        */
    }
    else
    {
        if (getLayerWeight(ANIMATION_LAYER_MOVEMENT_STRAFECHANGE) >= 1.f)
        {
            m_flMoveYaw = m_flMoveYawIdeal;
        }
        else
        {
            /*
            if (m_flInAirSmoothValue >= 1 && !m_bFirstRunSinceInit && abs(m_flMoveYawCurrentToIdeal) > 100 && m_bOnGround && entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].GetCurrentWeight() <= 0)
            {
                entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].SetShouldCapture(bonesnapshot_get(cl_bonesnapshot_speed_movebegin));
            }
            */
            float flMoveWeight = Helpers::lerp(m_flAnimDuckAmount, std::clamp(m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f), std::clamp(m_flSpeedAsPortionOfCrouchTopSpeed, 0.f, 1.f));
            float flRatio = Helpers::bias(flMoveWeight, 0.18f) + 0.1f;

            m_flMoveYaw = Helpers::angleNormalize(m_flMoveYaw + (m_flMoveYawCurrentToIdeal * flRatio));
        }
    }
    m_tPoseParamMappings[PLAYER_POSE_PARAM_MOVE_YAW].setValue(entity, m_flMoveYaw);

    float flAimYaw = Helpers::angleDiff(m_flEyeYaw, m_flFootYaw);
    if (flAimYaw >= 0.f && m_flAimYawMax != 0.f)
    {
        flAimYaw = (flAimYaw / m_flAimYawMax) * 60.0f;
    }
    else if (m_flAimYawMin != 0.f)
    {
        flAimYaw = (flAimYaw / m_flAimYawMin) * -60.0f;
    }

    m_tPoseParamMappings[PLAYER_POSE_PARAM_BODY_YAW].setValue(entity, flAimYaw);

    // we need non-symmetrical arbitrary min/max bounds for vertical aim (pitch) too
    float flPitch = Helpers::angleDiff(m_flEyePitch, 0.f);
    if (flPitch > 0.f)
    {
        flPitch = (flPitch / m_flAimPitchMax) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MAX;
    }
    else
    {
        flPitch = (flPitch / m_flAimPitchMin) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MIN;
    }

    m_tPoseParamMappings[PLAYER_POSE_PARAM_BODY_PITCH].setValue(entity, flPitch);
    m_tPoseParamMappings[PLAYER_POSE_PARAM_SPEED].setValue(entity, m_flSpeedAsPortionOfWalkTopSpeed);
    m_tPoseParamMappings[PLAYER_POSE_PARAM_STAND].setValue(entity, 1.0f - (m_flAnimDuckAmount * m_flInAirSmoothValue));

    interfaces->mdlCache->endLock();
}

void AnimState::setupAliveLoop() noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    if (getLayerActivity(ANIMATION_LAYER_ALIVELOOP) != ACT_CSGO_ALIVE_LOOP)
    {
        interfaces->mdlCache->beginLock();
        setLayerSequence(ANIMATION_LAYER_ALIVELOOP, ACT_CSGO_ALIVE_LOOP);
        setLayerCycle(ANIMATION_LAYER_ALIVELOOP, memory->randomFloat(0, 1));
        auto layer = entity->getAnimationLayer(ANIMATION_LAYER_ALIVELOOP);
        if (layer)
        {
            float flNewRate = entity->getSequenceCycleRate(entity->getModelPtr(), layer->sequence);
            flNewRate *= memory->randomFloat(0.8f, 1.1f);
            setLayerRate(ANIMATION_LAYER_ALIVELOOP, flNewRate);
        }
        interfaces->mdlCache->endLock();
        incrementLayerCycle(ANIMATION_LAYER_ALIVELOOP, true);
    }
    else
    {
        if (m_pWeapon && m_pWeapon != m_pWeaponLast)
        {
            //re-roll act on weapon change
            float flRetainCycle = getLayerCycle(ANIMATION_LAYER_ALIVELOOP);
            setLayerSequence(ANIMATION_LAYER_ALIVELOOP, ACT_CSGO_ALIVE_LOOP);
            setLayerCycle(ANIMATION_LAYER_ALIVELOOP, flRetainCycle);
            incrementLayerCycle(ANIMATION_LAYER_ALIVELOOP, true);
        }
        else if (isLayerSequenceCompleted(ANIMATION_LAYER_ALIVELOOP))
        {
            //re-roll rate
            interfaces->mdlCache->beginLock();
            auto layer = entity->getAnimationLayer(ANIMATION_LAYER_ALIVELOOP);
            if (layer)
            {
                float flNewRate = entity->getSequenceCycleRate(entity->getModelPtr(), layer->sequence);
                flNewRate *= memory->randomFloat(0.8f, 1.1f);
                setLayerRate(ANIMATION_LAYER_ALIVELOOP, flNewRate);
            }
            interfaces->mdlCache->endLock();
            incrementLayerCycle(ANIMATION_LAYER_ALIVELOOP, true);
        }
        else
        {
            float flWeightOutPoseBreaker = Helpers::remapValClamped(m_flSpeedAsPortionOfRunTopSpeed, 0.55f, 0.9f, 1.0f, 0.0f);
            setLayerWeight(ANIMATION_LAYER_ALIVELOOP, flWeightOutPoseBreaker);
            incrementLayerCycle(ANIMATION_LAYER_ALIVELOOP, true);
        }
    }
}

void AnimState::addActivityModifier(const char* name) noexcept
{
    activityModifiersWrapper().addModifier(name);
}

void AnimState::updateActivityModifiers() noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    activityModifiersWrapper modifierWrapper{};
    modifierWrapper.addModifier(memory->getWeaponPrefix(this));

    if (m_flSpeedAsPortionOfWalkTopSpeed > .25f)
        modifierWrapper.addModifier("moving");

    if (m_flAnimDuckAmount > .55f)
        modifierWrapper.addModifier("crouch");

    m_ActivityModifiers = modifierWrapper.get();
}

void AnimState::doAnimationEvent(int animationEvent) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    updateActivityModifiers();

    switch (animationEvent)
    {
    case PLAYERANIMEVENT_COUNT:
        return;
    case PLAYERANIMEVENT_THROW_GRENADE_UNDERHAND:
    {
        addActivityModifier("underhand");
    }
    case PLAYERANIMEVENT_FIRE_GUN_PRIMARY:
    case PLAYERANIMEVENT_THROW_GRENADE:
    {
        auto weapon = reinterpret_cast<Entity*>(m_pWeapon);
        if (!weapon)
            break;

        if (weapon && animationEvent == PLAYERANIMEVENT_FIRE_GUN_PRIMARY && weapon->isBomb())
        {
            setLayerSequence(ANIMATION_LAYER_WHOLE_BODY, ACT_CSGO_PLANT_BOMB);
            m_bPlantAnimStarted = true;
            //*reinterpret_cast<int*>(animState + 316) = 1;//+316 m_bPlantAnimStarted
        }
        else
        {
            setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_FIRE_PRIMARY);
        }
        break;
    }
    case PLAYERANIMEVENT_FIRE_GUN_PRIMARY_OPT:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_FIRE_PRIMARY_OPT_1);
        break;
    }
    case PLAYERANIMEVENT_FIRE_GUN_PRIMARY_SPECIAL1:
    case PLAYERANIMEVENT_FIRE_GUN_PRIMARY_OPT_SPECIAL1:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_FIRE_PRIMARY_OPT_2);
        break;
    }
    case PLAYERANIMEVENT_FIRE_GUN_SECONDARY:
    {
        auto weapon = reinterpret_cast<Entity*>(m_pWeapon);
        if (!weapon)
            break;

        if (weapon->isSniperRifle())
        {
            // hack: sniper rifles use primary fire anim when 'alt' firing, meaning scoped.
            setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_FIRE_PRIMARY);
        }
        else
        {
            setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_FIRE_SECONDARY);
        }
        break;
    }
    case PLAYERANIMEVENT_FIRE_GUN_SECONDARY_SPECIAL1:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_FIRE_SECONDARY_OPT_1);
        break;
    }
    case PLAYERANIMEVENT_GRENADE_PULL_PIN:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_OPERATE);
        break;
    }
    case PLAYERANIMEVENT_SILENCER_ATTACH:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_SILENCER_ATTACH);
        break;
    }
    case PLAYERANIMEVENT_SILENCER_DETACH:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_SILENCER_DETACH);
        break;
    }
    case PLAYERANIMEVENT_RELOAD:
    {
        auto weapon = reinterpret_cast<Entity*>(m_pWeapon);
        if (!weapon)
            break;

        if (weapon && weapon->isShotgun() && weapon->itemDefinitionIndex2() != WeaponId::Mag7)
            break;

        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_RELOAD);
        break;
    }
    case PLAYERANIMEVENT_RELOAD_START:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_RELOAD_START);
        break;
    }
    case PLAYERANIMEVENT_RELOAD_LOOP:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_RELOAD_LOOP);
        break;
    }
    case PLAYERANIMEVENT_RELOAD_END:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_RELOAD_END);
        break;
    }
    case PLAYERANIMEVENT_CATCH_WEAPON:
    {
        setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_CATCH);
        break;
    }
    case PLAYERANIMEVENT_CLEAR_FIRING:
    {
        m_bPlantAnimStarted = false;
        break;
    }
    case PLAYERANIMEVENT_DEPLOY:
    {
        auto weapon = reinterpret_cast<Entity*>(m_pWeapon);
        if (!weapon)
            break;

        if (weapon && getLayerActivity(ANIMATION_LAYER_WEAPON_ACTION) == ACT_CSGO_DEPLOY &&
            getLayerCycle(ANIMATION_LAYER_WEAPON_ACTION) < 0.15f/*CSGO_ANIM_DEPLOY_RATELIMIT*/)
        {
            // we're already deploying
            m_bDeployRateLimiting = true;
        }
        else
        {
            setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_DEPLOY);
        }
        break;
    }
    case PLAYERANIMEVENT_JUMP:
    {
        // note: this event means a jump is definitely happening, not just that a jump is desired
        m_bJumping = true;
        setLayerSequence(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, ACT_CSGO_JUMP);
        break;
    }
    default:
        break;
    }
}

void animstatePoseParamCache::setValue(Entity* player, float flValue) noexcept
{
	if (!m_bInitialized)
	{
		init(player, m_szName);
	}
	if (m_bInitialized && player)
	{
		interfaces->mdlCache->beginLock();
		player->setPoseParameter(flValue, m_nIndex);
		interfaces->mdlCache->endLock();
	}
}

void AnimState::setLayerSequence(size_t layer, int32_t activity) noexcept
{
    if (activity == ACT_INVALID)
        return;

    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto hdr = entity->getModelPtr();
    if (!hdr)
        return;

    const auto mapping = memory->findMapping(hdr);
    const auto sequence = memory->selectWeightedSequenceFromModifiers(mapping, hdr, activity, &m_ActivityModifiers[0], m_ActivityModifiers.size);

    if (sequence < 2)
        return;

    auto& l = *entity->getAnimationLayer(layer);
    l.playbackRate = entity->getLayerSequenceCycleRate(&l, sequence);
    l.sequence = sequence;
    l.cycle = l.weight = 0.f;
}

void AnimState::setLayerRate(size_t layer, float newRate) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;
    l.playbackRate = newRate;
}

void AnimState::setLayerCycle(size_t layer, float newCycle) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;
    l.cycle = Helpers::clampCycle(newCycle);
}

void AnimState::setLayerWeight(size_t layer, float weight) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;
    l.weight = std::clamp(weight, 0.f, 1.f);
}

void AnimState::setLayerWeightRate(size_t layer, float previous) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;

    float flNewRate = (l.weight - previous) / m_flLastUpdateIncrement;
    l.weightDeltaRate = flNewRate;
}

void AnimState::incrementLayerCycle(size_t layer, bool allowLoop) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;

    if (abs(l.playbackRate) <= 0)
        return;

    float flCurrentCycle = l.cycle;
    flCurrentCycle += m_flLastUpdateIncrement * l.playbackRate;

    if (!allowLoop && flCurrentCycle >= 1)
    {
        flCurrentCycle = 0.999f;
    }

    l.cycle = (Helpers::clampCycle(flCurrentCycle));
}

void AnimState::incrementLayerCycleWeightRateGeneric(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;

    float flWeightPrevious = getLayerWeight(layer);
    incrementLayerCycle(layer, false);
    setLayerWeight(layer, getLayerIdealWeightFromSeqCycle(layer));
    setLayerWeightRate(layer, flWeightPrevious);
}

void AnimState::incrementLayerWeight(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;

    if (abs(l.weightDeltaRate) <= 0)
        return;

    float flCurrentWeight = l.weight;
    flCurrentWeight += m_flLastUpdateIncrement * l.weightDeltaRate;
    flCurrentWeight = std::clamp(flCurrentWeight, 0.f, 1.f);

    l.weight = flCurrentWeight;
}

float AnimState::getLayerIdealWeightFromSeqCycle(size_t layer) noexcept
{
    return memory->getLayerIdealWeightFromSeqCycle(this, layer);
}

bool AnimState::isLayerSequenceCompleted(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (&l)
        return (m_flLastUpdateIncrement * l.playbackRate) + l.cycle >= 1.0f;
    return false;
}

float AnimState::getLayerCycle(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return 0.f;
    return l.cycle;
}

float AnimState::getLayerWeight(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return 0.f;
    return l.weight;
}

int AnimState::getLayerActivity(size_t layer)noexcept
{
    return memory->getLayerActivity(this, layer);
}

int AnimState::getLayerSequence(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(m_pPlayer);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return 0;
    return l.sequence;
}