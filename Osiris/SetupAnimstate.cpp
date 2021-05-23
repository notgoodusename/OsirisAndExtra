#include "Hacks/Animations.h"

#include "SDK/Vector.h"
#include "SDK/Entity.h"

#include "SDK/UserCmd.h"

#include "Helpers.h"


/*TODO: 
I need to find

m_boneSnapshots
//lookUpSequence
//updateAnimLayer
//getSequenceMoveDist
//pSeqdesc
//GetFirstSequenceAnimTag
*/

void Reset(void* thisPointer, void* edx)
{
    //static auto original = hooks->setupVelocity.getOriginalDetour<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    if (!animState)
        return;// original(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->m_pPlayer);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return;// original(thisPointer);

    //original(thisPointer);

    animState->getServerVars().m_bJumping = false;
    animState->getServerVars().m_bDeployRateLimiting = false;
    animState->m_flTimeToAlignLowerBody = 0;
    entity->lby() = 0;
}

void SetUpVelocity(void* thisPointer, void* edx)
{
    //static auto original = hooks->setupVelocity.getOriginalDetour<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    if (!animState)
        return;// original(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->m_pPlayer);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return;// original(thisPointer);

    interfaces->mdlCache->beginLock();

    //Use custom velocity
    Vector vecAbsVelocity = entity->getAbsVelocity();

    // prevent the client input velocity vector from exceeding a reasonable magnitude
#define CSGO_ANIM_MAX_VEL_LIMIT 1.2f
    if (vecAbsVelocity.squareLength() > std::sqrt(CS_PLAYER_SPEED_RUN * CSGO_ANIM_MAX_VEL_LIMIT))
        vecAbsVelocity = vecAbsVelocity.normalize() * (CS_PLAYER_SPEED_RUN * CSGO_ANIM_MAX_VEL_LIMIT);

    // save vertical velocity component
    animState->m_flVelocityLengthZ = vecAbsVelocity.z;

    // discard z component
    vecAbsVelocity.z = 0;

    // remember if the player is accelerating.
    animState->m_bPlayerIsAccelerating = (animState->m_vecVelocityLast.squareLength() < vecAbsVelocity.squareLength());

    // rapidly approach ideal velocity instead of instantly adopt it. This helps smooth out instant velocity changes, like
    // when the player runs headlong into a wall and their velocity instantly becomes zero.
    animState->m_vecVelocity = Helpers::approach(vecAbsVelocity, animState->m_vecVelocity, animState->m_flLastUpdateIncrement * 2000);
    animState->m_vecVelocityNormalized = animState->m_vecVelocity;
    animState->m_vecVelocityNormalized.normalize();

    // save horizontal velocity length
    animState->m_flVelocityLengthXY = std::min(animState->m_vecVelocity.length(), CS_PLAYER_SPEED_RUN);

    //compute speed in various normalized forms
    auto weapon = entity->getActiveWeapon();
    float flMaxSpeedRun = CS_PLAYER_SPEED_RUN;
    if (weapon)
    {
        if (auto weaponData = weapon->getWeaponData(); weaponData)
        {
            const float maxSpeed = (entity->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed);
            flMaxSpeedRun = std::fmax(maxSpeed, 0.001f);
        }
    }

    animState->m_flSpeedAsPortionOfRunTopSpeed = std::clamp(animState->m_flVelocityLengthXY / flMaxSpeedRun, 0.f, 1.f);
    animState->m_flSpeedAsPortionOfWalkTopSpeed = animState->m_flVelocityLengthXY / (flMaxSpeedRun * CS_PLAYER_SPEED_WALK_MODIFIER);
    animState->m_flSpeedAsPortionOfCrouchTopSpeed = animState->m_flVelocityLengthXY / (flMaxSpeedRun * CS_PLAYER_SPEED_DUCK_MODIFIER);
    
    if (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 1)
    {
        animState->m_flStaticApproachSpeed = animState->m_flVelocityLengthXY;
    }
    else if (m_flSpeedAsPortionOfWalkTopSpeed < 0.5f)
    {
        animState->m_flStaticApproachSpeed = Helpers::approach(80, animState->m_flStaticApproachSpeed, animState->m_flLastUpdateIncrement * 60);
    }

    bool bStartedMovingThisFrame = false;
    bool bStoppedMovingThisFrame = false;

    if (animState->m_flVelocityLengthXY > 0)
    {
        bStartedMovingThisFrame = (animState->m_flDurationMoving <= 0);
        animState->m_flDurationStill = 0;
        animState->m_flDurationMoving += animState->m_flLastUpdateIncrement;
    }
    else
    {
        bStoppedMovingThisFrame = (animState->m_flDurationStill <= 0);
        animState->m_flDurationMoving = 0;
        animState->m_flDurationStill += animState->m_flLastUpdateIncrement;
    }

    if (!animState->m_bAdjustStarted && bStoppedMovingThisFrame && animState->m_bOnGround && !animState->m_bOnLadder && !animState->m_bLanding && animState->m_flStutterStep < 50)
    {
        animState->setLayerSequence(ANIMATION_LAYER_ADJUST, ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING, entity->getModifiers());
        animState->m_bAdjustStarted = true;
    }

    if (animState->getLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ||
        animState->getLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
    {
        if (animState->m_bAdjustStarted && animState->m_flSpeedAsPortionOfCrouchTopSpeed <= 0.25f)
        {
            animState->incrementLayerCycleWeightRateGeneric(ANIMATION_LAYER_ADJUST);
            animState->m_bAdjustStarted = !(animState->isLayerSequenceCompleted(ANIMATION_LAYER_ADJUST));
        }
        else
        {
            animState->m_bAdjustStarted = false;
            float flWeight = animState->getLayerWeight(ANIMATION_LAYER_ADJUST);
            animState->setLayerWeight(ANIMATION_LAYER_ADJUST, Helpers::approach(0, flWeight, animState->m_flLastUpdateIncrement * 5));
            animState->setLayerWeightRate(ANIMATION_LAYER_ADJUST, flWeight);
        }
    }

    animState->m_flFootYawLast = animState->m_flFootYaw;
    animState->m_flFootYaw = std::clamp(animState->m_flFootYaw, -360.0f, 360.0f);
    float flEyeFootDelta = Helpers::angleDiff(animState->m_flEyeYaw, animState->m_flFootYaw);

    float flAimMatrixWidthRange = std::lerp(std::clamp(animState->m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f), 1.0f, std::lerp(animState->m_flWalkToRunTransition, CSGO_ANIM_AIM_NARROW_WALK, CSGO_ANIM_AIM_NARROW_RUN));

    if (animState->m_flAnimDuckAmount > 0)
    {
        flAimMatrixWidthRange = std::lerp(animState->m_flAnimDuckAmount * std::clamp(animState->m_flSpeedAsPortionOfCrouchTopSpeed, 0.f, 1.f), flAimMatrixWidthRange, CSGO_ANIM_AIM_NARROW_CROUCHMOVING);
    }


    float flTempYawMax = animState->m_flAimYawMax * flAimMatrixWidthRange;
    float flTempYawMin = animState->m_flAimYawMin * flAimMatrixWidthRange;

    if (flEyeFootDelta > flTempYawMax)
    {
        animState->m_flFootYaw = animState->m_flEyeYaw - abs(flTempYawMax);
    }
    else if (flEyeFootDelta < flTempYawMin)
    {
        animState->m_flFootYaw = animState->m_flEyeYaw + abs(flTempYawMin);
    }
    animState->m_flFootYaw = Helpers::angleNormalize(animState->m_flFootYaw);

    if (animState->m_flVelocityLengthXY > 0.1f || fabs(animState->m_flVelocityLengthZ) > 100.0f)
    {
        animState->m_flFootYaw = Helpers::approachAngle(animState->m_flEyeYaw, animState->m_flFootYaw, animState->m_flLastUpdateIncrement * (30.0f + 20.0f * animState->m_flWalkToRunTransition));
        animState->m_flTimeToAlignLowerBody = memory->globalVars->currenttime + (CSGO_ANIM_LOWER_REALIGN_DELAY * 0.2f);
        if (entity->lby() != m_flEyeYaw)
            entity->lby() = m_flEyeYaw;
    }
    else
    {
        animState->m_flFootYaw = Helpers::approachAngle(entity->lby(), animState->m_flFootYaw, animState->m_flLastUpdateIncrement * CSGO_ANIM_LOWER_CATCHUP_IDLE);

        if (memory->globalVars->currenttime > animState->m_flTimeToAlignLowerBody && abs(Helpers::angleDiff(animState->m_flFootYaw, animState->m_flEyeYaw)) > 35.0f)
        {
            animState->m_flTimeToAlignLowerBody = memory->globalVars->currenttime + CSGO_ANIM_LOWER_REALIGN_DELAY;
            if (entity->lby() != animState->m_flEyeYaw)
                entity->lby() = animState->m_flEyeYaw;
        }
    }

    if (animState->m_flVelocityLengthXY <= CS_PLAYER_SPEED_STOPPED && animState->m_bOnGround && !animState->m_bOnLadder && !animState->m_bLanding && animState->m_flLastUpdateIncrement > 0 && std::fabsf(Helpers::angleDiff(animState->m_flFootYawLast, animState->m_flFootYaw) / animState->m_flLastUpdateIncrement > 120.f))
    {
        animState->setLayerSequence(ANIMATION_LAYER_ADJUST, ACT_CSGO_IDLE_TURN_BALANCEADJUST, entity->getModifiers());
        animState->m_bAdjustStarted = true;
    }

    if (animState->getLayerWeight(ANIMATION_LAYER_ADJUST) > 0)
    {
        animState->incrementLayerCycle(ANIMATION_LAYER_ADJUST, false);
        animState->incrementLayerWeight(ANIMATION_LAYER_ADJUST);
    }

    // the final model render yaw is aligned to the foot yaw
    if (animState->m_flVelocityLengthXY > 0 && animState->m_bOnGround)
    {
        // convert horizontal velocity vec to angular yaw
        float flRawYawIdeal = (std::atan2(-animState->m_vecVelocity.x, -animState->m_vecVelocity.y) * 180 / 3.14159265358979323846);
        if (flRawYawIdeal < 0)
            flRawYawIdeal += 360;

        animState->m_flMoveYawIdeal = Helpers::angleNormalize(Helpers::angleDiff(flRawYawIdeal, animState->m_flFootYaw));
    }

    // delta between current yaw and ideal velocity derived target (possibly negative!)
    animState->m_flMoveYawCurrentToIdeal = Helpers::angleNormalize(Helpers::angleDiff(animState->m_flMoveYawIdeal, animState->m_flMoveYaw));

    if (bStartedMovingThisFrame && animState->m_flMoveWeight <= 0)
    {
        animState->m_flMoveYaw = animState->m_flMoveYawIdeal;

        // select a special starting cycle that's set by the animator in content
        int nMoveSeq = animState->getLayerSequence(ANIMATION_LAYER_MOVEMENT_MOVE);
        if (nMoveSeq != -1)
        {
            StudioSeqdesc& seqdesc = entity->getModelPtr()->seqdesc(nMoveSeq);
            if (seqdesc.numAnimTags > 0)
            {
                if (abs(Helpers::angleDiff(animState->m_flMoveYaw, 180)) <= EIGHT_WAY_WIDTH) //N
                {
                    animState->m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_N);
                }
                else if (abs(Helpers::angleDiff(animState->m_flMoveYaw, 135)) <= EIGHT_WAY_WIDTH) //NE
                {
                    animState->m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_NE);
                }
                else if (abs(Helpers::angleDiff(animState->m_flMoveYaw, 90)) <= EIGHT_WAY_WIDTH) //E
                {
                    animState->m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_E);
                }
                else if (abs(Helpers::angleDiff(animState->m_flMoveYaw, 45)) <= EIGHT_WAY_WIDTH) //SE
                {
                    animState->m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_SE);
                }
                else if (abs(Helpers::angleDiff(animState->m_flMoveYaw, 0)) <= EIGHT_WAY_WIDTH) //S
                {
                    animState->m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_S);
                }
                else if (abs(Helpers::angleDiff(animState->m_flMoveYaw, -45)) <= EIGHT_WAY_WIDTH) //SW
                {
                    animState->m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_SW);
                }
                else if (abs(Helpers::angleDiff(animState->m_flMoveYaw, -90)) <= EIGHT_WAY_WIDTH) //W
                {
                    animState->m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_W);
                }
                else if (abs(Helpers::angleDiff(animState->m_flMoveYaw, -135)) <= EIGHT_WAY_WIDTH) //NW
                {
                    animState->m_flPrimaryCycle = entity->getFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_NW);
                }
            }
        }

        /*
        if (animState->m_flInAirSmoothValue >= 1 && !animState->m_bFirstRunSinceInit && abs(animState->m_flMoveYawCurrentToIdeal) > 45 && animState->m_bOnGround && entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].GetCurrentWeight() <= 0)
        {
            entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].SetShouldCapture(bonesnapshot_get(cl_bonesnapshot_speed_movebegin));
        }
        */
    }
    else
    {
        if (animState->getLayerWeight(ANIMATION_LAYER_MOVEMENT_STRAFECHANGE) >= 1)
        {
            animState->m_flMoveYaw = animState->m_flMoveYawIdeal;
        }
        else
        {
            /*
            if (animState->m_flInAirSmoothValue >= 1 && !animState->m_bFirstRunSinceInit && abs(animState->m_flMoveYawCurrentToIdeal) > 100 && animState->m_bOnGround && entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].GetCurrentWeight() <= 0)
            {
                entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].SetShouldCapture(bonesnapshot_get(cl_bonesnapshot_speed_movebegin));
            }
            */
            float flMoveWeight = std::lerp(animState->m_flAnimDuckAmount, std::clamp(animState->m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f), std::clamp(animState->m_flSpeedAsPortionOfCrouchTopSpeed, 0.f, 1.f));
            float flRatio = Helpers::bias(flMoveWeight, 0.18f) + 0.1f;

            animState->m_flMoveYaw = Helpers::angleNormalize(animState->m_flMoveYaw + (animState->m_flMoveYawCurrentToIdeal * flRatio));
        }
    }
    animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_MOVE_YAW].setValue(entity, animState->m_flMoveYaw);

    float flAimYaw = Helpers::angleDiff(animState->m_flEyeYaw, animState->m_flFootYaw);
    if (flAimYaw >= 0 && animState->m_flAimYawMax != 0)
    {
        flAimYaw = (flAimYaw / animState->m_flAimYawMax) * 60.0f;
    }
    else if (animState->m_flAimYawMin != 0)
    {
        flAimYaw = (flAimYaw / animState->m_flAimYawMin) * -60.0f;
    }

    animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_BODY_YAW].setValue(entity, flAimYaw);

    // we need non-symmetrical arbitrary min/max bounds for vertical aim (pitch) too
    float flPitch = Helpers::angleDiff(animState->m_flEyePitch, 0);
    if (flPitch > 0)
    {
        flPitch = (flPitch / animState->m_flAimPitchMax) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MAX;
    }
    else
    {
        flPitch = (flPitch / animState->m_flAimPitchMin) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MIN;
    }

    animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_BODY_PITCH].setValue(entity, flPitch);
    animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_SPEED].setValue(entity, animState->m_flSpeedAsPortionOfWalkTopSpeed);
    animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_STAND].setValue(entity, 1.0f - (animState->m_flAnimDuckAmount * animState->m_flInAirSmoothValue));

    interfaces->mdlCache->endLock();
}

void SetUpMovement(void* thisPointer, void* edx)
{
    //static auto original = hooks->setupVelocity.getOriginalDetour<void>();

    auto ecx = *reinterpret_cast<uintptr_t*>(thisPointer);

    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    if (!animState)
        return;// original(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->m_pPlayer);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return;// original(thisPointer);

    interfaces->mdlCache->beginLock();

    if (animState->m_flWalkToRunTransition > 0 && animState->m_flWalkToRunTransition < 1)
    {
        //currently transitioning between walk and run
        if (animState->m_bWalkToRunTransitionState == ANIM_TRANSITION_WALK_TO_RUN)
        {
            animState->m_flWalkToRunTransition += animState->m_flLastUpdateIncrement * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
        }
        else // m_bWalkToRunTransitionState == ANIM_TRANSITION_RUN_TO_WALK
        {
            animState->m_flWalkToRunTransition -= animState->m_flLastUpdateIncrement * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
        }
        animState->m_flWalkToRunTransition = std::clamp(animState->m_flWalkToRunTransition, 0.f, 1.f);
    }

    if (animState->m_flVelocityLengthXY > (CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER) && animState->m_bWalkToRunTransitionState == ANIM_TRANSITION_RUN_TO_WALK)
    {
        //crossed the walk to run threshold
        animState->m_bWalkToRunTransitionState = ANIM_TRANSITION_WALK_TO_RUN;
        animState->m_flWalkToRunTransition = std::max(0.01f, m_flWalkToRunTransition);
    }
    else if (animState->m_flVelocityLengthXY < (CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER) && animState->m_bWalkToRunTransitionState == ANIM_TRANSITION_WALK_TO_RUN)
    {
        //crossed the run to walk threshold
        animState->m_bWalkToRunTransitionState = ANIM_TRANSITION_RUN_TO_WALK;
        animState->m_flWalkToRunTransition = std::min(0.99f, m_flWalkToRunTransition);
    }

    if (animState->m_nAnimstateModelVersion < 2)
    {
        animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_RUN].setValue(entity, animState->m_flWalkToRunTransition);
    }
    else
    {
        animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_MOVE_BLEND_WALK].setValue(entity, (1.0f - animState->m_flWalkToRunTransition) * (1.0f - animState->m_flAnimDuckAmount));
        animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_MOVE_BLEND_RUN].setValue(entity, (animState->m_flWalkToRunTransition) * (1.0f - animState->m_flAnimDuckAmount));
        animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_MOVE_BLEND_CROUCH_WALK].setValue(entity, animState->m_flAnimDuckAmount);
    }

    char szWeaponMoveSeq[MAX_ANIMSTATE_ANIMNAME_CHARS];
    sprintf(szWeaponMoveSeq, "move_%s", memory->getWeaponPrefix(animState));

    int nWeaponMoveSeq = entity->lookupSequence(szWeaponMoveSeq);
    if (nWeaponMoveSeq == -1)
        nWeaponMoveSeq = entity->lookupSequence("move");

    if (entity->moveState() != animState->m_nPreviousMoveState)
        animState->m_flStutterStep += 10;

    animState->m_nPreviousMoveState = entity->moveState();
    animState->m_flStutterStep = std::clamp(Helpers::approach(0.f, animState->m_flStutterStep, animState->m_flLastUpdateIncrement * 40.f), 0.f, 100.f);

    // recompute moveweight
    float flTargetMoveWeight = Helpers::lerp(animState->m_flAnimDuckAmount, std::clamp(animState->m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f), std::clamp(animState->m_flSpeedAsPortionOfCrouchTopSpeed, 0.f, 1.f));

    if (animState->m_flMoveWeight <= flTargetMoveWeight)
    {
        animState->m_flMoveWeight = flTargetMoveWeight;
    }
    else
    {
        animState->m_flMoveWeight = Helpers::approach(flTargetMoveWeight, animState->m_flMoveWeight, animState->m_flLastUpdateIncrement * Helpers::remapValClamped(animState->m_flStutterStep, 0.0f, 100.0f, 2, 20));
    }

    Vector vecMoveYawDir;
    vecMoveYawDir = Vector::fromAngle(Vector{ 0.f, Helpers::angleNormalize(animState->m_flFootYaw + animState->m_flMoveYaw + 180 ), 0.f});
    float flYawDeltaAbsDot = std::abs(animState->m_vecVelocityNormalizedNonZero.dotProduct(vecMoveYawDir));
    animState->m_flMoveWeight *= Helpers::bias(flYawDeltaAbsDot, 0.2);

    float flMoveWeightWithAirSmooth = animState->m_flMoveWeight * animState->m_flInAirSmoothValue;

    // dampen move weight for landings
    flMoveWeightWithAirSmooth *= std::max((1.0f - animState->getLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB)), 0.55f);

    float flMoveCycleRate = 0;
    if (animState->m_flVelocityLengthXY > 0)
    {
        flMoveCycleRate = entity->getSequenceCycleRate(entity->getModelPtr(), nWeaponMoveSeq);
        float flSequenceGroundSpeed = std::max(entity->getSequenceMoveDist(entity->getModelPtr(), nWeaponMoveSeq) / (1.0f / flMoveCycleRate), 0.001f);
        flMoveCycleRate *= animState->m_flVelocityLengthXY / flSequenceGroundSpeed;

        flMoveCycleRate *= std::lerp(animState->m_flWalkToRunTransition, 1.0f, CSGO_ANIM_RUN_ANIM_PLAYBACK_MULTIPLIER);
    }

    float flLocalCycleIncrement = (flMoveCycleRate * animState->m_flLastUpdateIncrement);

    animState->m_flPrimaryCycle = Helpers::clampCycle(animState->m_flPrimaryCycle + flLocalCycleIncrement);

    flMoveWeightWithAirSmooth = std::clamp(flMoveWeightWithAirSmooth, 0.f, 1.f);
    entity->updateAnimLayer(ANIMATION_LAYER_MOVEMENT_MOVE, nWeaponMoveSeq, flLocalCycleIncrement, flMoveWeightWithAirSmooth, animState->m_flPrimaryCycle);

    // blend in a strafe direction-change pose when the player changes strafe dir

    // get the user's left and right button pressed states
    bool moveRight = (entity->m_nButtons & (IN_MOVERIGHT)) != 0;
    bool moveLeft = (entity->m_nButtons & (IN_MOVELEFT)) != 0;
    bool moveForward = (entity->m_nButtons & (IN_FORWARD)) != 0;
    bool moveBackward = (entity->m_nButtons & (IN_BACK)) != 0;

    Vector vecForward;
    Vector vecRight;
    Vector::fromAngle(Vector{ 0.f, animState->m_flFootYaw, 0.f }, vecForward, vecRight, Vector{});
    vecRight.normalize();
    float flVelToRightDot = animState->m_vecVelocityNormalizedNonZero.dotProduct(vecRight);
    float flVelToForwardDot = animState->m_vecVelocityNormalizedNonZero.dotProduct(vecForward);

    // We're interested in if the player's desired direction (indicated by their held buttons) is opposite their current velocity.
    // This indicates a strafing direction change in progress.

    bool bStrafeRight = (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 0.73f && moveRight && !moveLeft && flVelToRightDot < -0.63f);
    bool bStrafeLeft = (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 0.73f && moveLeft && !moveRight && flVelToRightDot > 0.63f);
    bool bStrafeForward = (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 0.65f && moveForward && !moveBackward && flVelToForwardDot < -0.55f);
    bool bStrafeBackward = (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 0.65f && moveBackward && !moveForward && flVelToForwardDot > 0.55f);

    entity->isStrafing() = (bStrafeRight || bStrafeLeft || bStrafeForward || bStrafeBackward);

    if (entity->isStrafing())
    {
        if (!animState->m_bStrafeChanging)
        {
            animState->m_flDurationStrafing = 0;

            /*
            if (!animState->m_bFirstRunSinceInit && !animState->m_bStrafeChanging && animState->m_bOnGround && entity->m_boneSnapshots[BONESNAPSHOT_UPPER_BODY].GetCurrentWeight() <= 0)
            {
                entity->m_boneSnapshots[BONESNAPSHOT_UPPER_BODY].SetShouldCapture(bonesnapshot_get(cl_bonesnapshot_speed_strafestart));
            }
            */
        }

        animState->m_bStrafeChanging = true;

        animState->m_flStrafeChangeWeight = Helpers::approach(1.f, animState->m_flStrafeChangeWeight, animState->m_flLastUpdateIncrement * 20.f);
        animState->m_flStrafeChangeCycle = Helpers::approach(0.f, animState->m_flStrafeChangeCycle, animState->m_flLastUpdateIncrement * 10.f);

        animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_STRAFE_DIR].setValue(entity, Helpers::angleNormalize(animState->m_flMoveYaw));

    }
    else if (animState->m_flStrafeChangeWeight > 0.f)
    {
        animState->m_flDurationStrafing += animState->m_flLastUpdateIncrement;

        if (animState->m_flDurationStrafing > 0.08f)
            animState->m_flStrafeChangeWeight = Helpers::approach(0.f, animState->m_flStrafeChangeWeight, animState->m_flLastUpdateIncrement * 5.f);

        animState->m_nStrafeSequence = entity->lookupSequence("strafe");
        float flRate = entity->getSequenceCycleRate(entity->getModelPtr(), animState->m_nStrafeSequence);
        animState->m_flStrafeChangeCycle = std::clamp(animState->m_flStrafeChangeCycle + animState->m_flLastUpdateIncrement * flRate, 0.f, 1.f);
    }

    if (animState->m_flStrafeChangeWeight <= 0.f)
    {
        animState->m_bStrafeChanging = false;
    }

    // keep track of if the player is on the ground, and if the player has just touched or left the ground since the last check
    bool bPreviousGroundState = animState->m_bOnGround;
    animState->m_bOnGround = (entity->flags() & 1);

    animState->m_bLandedOnGroundThisFrame = (!animState->m_bFirstRunSinceInit && bPreviousGroundState != animState->m_bOnGround && animState->m_bOnGround);
    animState->m_bLeftTheGroundThisFrame = (bPreviousGroundState != animState->m_bOnGround && !animState->m_bOnGround);

    float flDistanceFell = 0.f;
    if (animState->m_bLeftTheGroundThisFrame)
    {
        animState->m_flLeftGroundHeight = animState->m_vecPositionCurrent.z;
    }

    if (animState->m_bLandedOnGroundThisFrame)
    {
        flDistanceFell = fabs(animState->m_flLeftGroundHeight - animState->m_vecPositionCurrent.z);
        float flDistanceFallNormalizedBiasRange = Helpers::bias(Helpers::remapValClamped(flDistanceFell, 12.0f, 72.0f, 0.0f, 1.0f), 0.4f);

        animState->m_flLandAnimMultiplier = std::clamp(Helpers::bias(animState->m_flDurationInAir, 0.3f), 0.1f, 1.0f);
        animState->m_flDuckAdditional = std::max(animState->m_flLandAnimMultiplier, flDistanceFallNormalizedBiasRange);

    }
    else
    {
        animState->m_flDuckAdditional = Helpers::approach(0.f, animState->m_flDuckAdditional, animState->m_flLastUpdateIncrement * 2.f);
    }

    // the in-air smooth value is a fuzzier representation of if the player is on the ground or not.
    // It will approach 1 when the player is on the ground and 0 when in the air. Useful for blending jump animations.
    animState->m_flInAirSmoothValue = Helpers::approach(animState->m_bOnGround ? 1 : 0, animState->m_flInAirSmoothValue, std::lerp(animState->m_flAnimDuckAmount, CSGO_ANIM_ONGROUND_FUZZY_APPROACH, CSGO_ANIM_ONGROUND_FUZZY_APPROACH_CROUCH) * animState->m_flLastUpdateIncrement);
    animState->m_flInAirSmoothValue = std::clamp(animState->m_flInAirSmoothValue, 0.f, 1.f);



    animState->m_flStrafeChangeWeight *= (1.0f - animState->m_flAnimDuckAmount);
    animState->m_flStrafeChangeWeight *= animState->m_flInAirSmoothValue;
    animState->m_flStrafeChangeWeight = std::clamp(animState->m_flStrafeChangeWeight, 0.f, 1.f);

    if (animState->m_nStrafeSequence != -1)
        entity->updateAnimLayer(ANIMATION_LAYER_MOVEMENT_STRAFECHANGE, animState->m_nStrafeSequence, 0, animState->m_flStrafeChangeWeight, animState->m_flStrafeChangeCycle);

    //ladders
    bool bPreviouslyOnLadder = animState->m_bOnLadder;
    animState->m_bOnLadder = !animState->m_bOnGround && entity->moveType() == MoveType::LADDER;
    bool bStartedLadderingThisFrame = (!bPreviouslyOnLadder && animState->m_bOnLadder);
    bool bStoppedLadderingThisFrame = (bPreviouslyOnLadder && !animState->m_bOnLadder);

    if (animState->m_flLadderWeight > 0 || animState->m_bOnLadder)
    {
        if (bStartedLadderingThisFrame)
        {
            animState->setLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, ACT_CSGO_CLIMB_LADDER, entity->getModifiers());
        }

        if (std::abs(animState->m_flVelocityLengthZ) > 100)
        {
            animState->m_flLadderSpeed = Helpers::approach(1, animState->m_flLadderSpeed, animState->m_flLastUpdateIncrement * 10.0f);
        }
        else
        {
            animState->m_flLadderSpeed = Helpers::approach(0, animState->m_flLadderSpeed, animState->m_flLastUpdateIncrement * 10.0f);
        }
        animState->m_flLadderSpeed = std::clamp(animState->m_flLadderSpeed, 0.f, 1.f);

        if (animState->m_bOnLadder)
        {
            animState->m_flLadderWeight = Helpers::approach(1, animState->m_flLadderWeight, animState->m_flLastUpdateIncrement * 5.0f);
        }
        else
        {
            animState->m_flLadderWeight = Helpers::approach(0, animState->m_flLadderWeight, animState->m_flLastUpdateIncrement * 10.0f);
        }
        animState->m_flLadderWeight = std::clamp(animState->m_flLadderWeight, 0.f, 1.f);

        Vector vecLadderNormal = entity->getLadderNormal();
        Vector angLadder;
        angLadder = Vector::toAngle(vecLadderNormal);
        float flLadderYaw = Helpers::angleDiff(angLadder.y, animState->m_flFootYaw);
        animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_LADDER_YAW].setValue(entity, flLadderYaw);

        float flLadderClimbCycle = animState->getLayerCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB);
        flLadderClimbCycle += (animState->m_vecPositionCurrent.z - animState->m_vecPositionLast.z) * std::lerp(animState->m_flLadderSpeed, 0.010f, 0.004f);

        animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_LADDER_SPEED].setValue(entity, animState->m_flLadderSpeed);

        if (animState->getLayerActivity(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) == ACT_CSGO_CLIMB_LADDER)
        {
            animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, animState->m_flLadderWeight);
        }

        animState->setLayerCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLadderClimbCycle);

        // fade out jump if we're climbing
        if (animState->m_bOnLadder)
        {
            float flIdealJumpWeight = 1.0f - animState->m_flLadderWeight;
            if (animState->getLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL) > flIdealJumpWeight)
            {
                animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flIdealJumpWeight);
            }
        }
    }
    else
    {
        animState->m_flLadderSpeed = 0;
    }

    if (animState->m_bOnGround)
    {
        if (!animState->m_bLanding && (animState->m_bLandedOnGroundThisFrame || bStoppedLadderingThisFrame))
        {
            animState->setLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, (animState->m_flDurationInAir > 1) ? ACT_CSGO_LAND_HEAVY : ACT_CSGO_LAND_LIGHT, entity->getModifiers());
            animState->setLayerCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0);
            animState->m_bLanding = true;
        }
        animState->m_flDurationInAir = 0;

        if (animState->m_bLanding && animState->getLayerActivity(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) != ACT_CSGO_CLIMB_LADDER)
        {
            Animations::data.m_bJumping = false;

            animState->incrementLayerCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, false);
            animState->incrementLayerCycle(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false);

            animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_JUMP_FALL].setValue(entity, 0);

            if (animState->isLayerSequenceCompleted(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB))
            {
                animState->m_bLanding = false;
                animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0);
                animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, 0);
                animState->m_flLandAnimMultiplier = 1.0f;
            }
            else
            {

                float flLandWeight = animState->getLayerIdealWeightFromSeqCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) * animState->m_flLandAnimMultiplier;

                // if we hit the ground crouched, reduce the land animation as a function of crouch, since the land animations move the head up a bit ( and this is undesirable )
                flLandWeight *= std::clamp((1.0f - animState->m_flAnimDuckAmount), 0.2f, 1.0f);

                animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLandWeight);

                // fade out jump because land is taking over
                float flCurrentJumpFallWeight = animState->getLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
                if (flCurrentJumpFallWeight > 0)
                {
                    flCurrentJumpFallWeight = Helpers::approach(0, flCurrentJumpFallWeight, animState->m_flLastUpdateIncrement * 10.0f);
                    animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flCurrentJumpFallWeight);
                }

            }
        }

        if (!animState->m_bLanding && !Animations::data.m_bJumping && animState->m_flLadderWeight <= 0)
        {
            animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0);
        }
    }
    else if (!animState->m_bOnLadder)
    {
        animState->m_bLanding = false;

        // we're in the air
        if (animState->m_bLeftTheGroundThisFrame || bStoppedLadderingThisFrame)
        {
            // If entered the air by jumping, then we already set the jump activity.
            // But if we're in the air because we strolled off a ledge or the floor collapsed or something,
            // we need to set the fall activity here.
            if (!Animations::data.m_bJumping)
            {
                animState->setLayerSequence(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, ACT_CSGO_FALL, entity->getModifiers());
            }

            animState->m_flDurationInAir = 0;
        }

        animState->m_flDurationInAir += animState->m_flLastUpdateIncrement;

        animState->incrementLayerCycle(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false);

        // increase jump weight
        float flJumpWeight = animState->getLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
        float flNextJumpWeight = animState->getLayerIdealWeightFromSeqCycle(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
        if (flNextJumpWeight > flJumpWeight)
        {
            animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flNextJumpWeight);
        }

        // bash any lingering land weight to zero
        float flLingeringLandWeight = animState->getLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB);
        if (flLingeringLandWeight > 0)
        {
            flLingeringLandWeight *= Helpers::smoothStepBounds(0.2f, 0.0f, animState->m_flDurationInAir);
            animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLingeringLandWeight);
        }

        // blend jump into fall. This is a no-op if we're playing a fall anim.
        animState->m_tPoseParamMappings[PLAYER_POSE_PARAM_JUMP_FALL].setValue(entity, std::clamp(Helpers::smoothStepBounds(0.72f, 1.52f, animState->m_flDurationInAir), 0.f, 1.f));
    }
    interfaces->mdlCache->endLock();
}

void __fastcall setupVelocityHook(void* thisPointer, void* edx)
{
    //static auto original = hooks->setupVelocity.getOriginalDetour<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    if (!animState)
        return;// original(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->m_pPlayer);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return;// original(thisPointer);

    float m_flLastUpdateIncrement = *reinterpret_cast<float*>(ecx + 116);//+116

    Vector vecAbsVelocity = entity->getAbsVelocity();
    Vector vecVelocity = entity->velocity();
    if (vecAbsVelocity.squareLength() > std::sqrt(260.f * 1.2f /*CSGO_ANIM_MAX_VEL_LIMIT*/))
        vecAbsVelocity = vecAbsVelocity.normalize() * (260.f * 1.2 /*CSGO_ANIM_MAX_VEL_LIMIT*/);
    float m_flAbsVelocityZ = vecAbsVelocity.z;
    vecAbsVelocity.z = 0;

    vecVelocity = Helpers::approach(vecAbsVelocity, vecVelocity, m_flLastUpdateIncrement * 2000);

    float m_flVelocityLengthXY = min(vecVelocity.length(), 260.f /*CS_PLAYER_SPEED_RUN*/);
    auto weapon = entity->getActiveWeapon();
    float flMaxSpeedRun = 260.0f;
    if (weapon)
    {
        if (auto weaponData = weapon->getWeaponData(); weaponData)
        {
            const float maxSpeed = (entity->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed);
            flMaxSpeedRun = std::fmax(maxSpeed, 0.001f);
        }
    }

    bool bStartedMovingThisFrame = false;
    bool bStoppedMovingThisFrame = false;

    float m_flSpeedAsPortionOfRunTopSpeed = std::clamp(m_flVelocityLengthXY / flMaxSpeedRun, 0.f, 1.f);
    float m_flSpeedAsPortionOfWalkTopSpeed = m_flVelocityLengthXY / (flMaxSpeedRun * 0.52f/*CS_PLAYER_SPEED_WALK_MODIFIER*/);
    float m_flSpeedAsPortionOfCrouchTopSpeed = m_flVelocityLengthXY / (flMaxSpeedRun * 0.34f/*CS_PLAYER_SPEED_DUCK_MODIFIER*/);

    if (m_flVelocityLengthXY > 0)
    {
        bStartedMovingThisFrame = (animState->TimeSinceStartedMoving <= 0);
    }
    else
    {
        bStoppedMovingThisFrame = (animState->TimeSinceStoppedMoving <= 0);
    }

    if (!animState->m_bAdjustStarted && bStoppedMovingThisFrame && animState->OnGround && !animState->onLadder && !animState->InHitGroundAnimation && m_flStutterStep < 50)
    {
        animState->setLayerSequence(ANIMATION_LAYER_ADJUST, ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING, entity->getModifiers());
        m_bAdjustStarted = true;
    }

    if (memory->getLayerActivity(animState, ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ||
        memory->getLayerActivity(animState, ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
    {
        if (m_bAdjustStarted && m_flSpeedAsPortionOfCrouchTopSpeed <= 0.25f)
        {
            entity->incrementLayerCycleWeightRateGeneric(ANIMATION_LAYER_ADJUST);
            m_bAdjustStarted = !(entity->isLayerSequenceCompleted(ANIMATION_LAYER_ADJUST));
        }
        else
        {
            m_bAdjustStarted = false;
            float flWeight = entity->getLayerWeight(ANIMATION_LAYER_ADJUST);
            entity->setLayerWeight(ANIMATION_LAYER_ADJUST, Helpers::approach(0, flWeight, m_flLastUpdateIncrement * 5));
            entity->setLayerWeightRate(ANIMATION_LAYER_ADJUST, flWeight);
        }
    }

    float m_flFootYawLast = animState->GoalFeetYaw;
    float m_flFootYaw = std::clamp(animState->GoalFeetYaw, -360.0f, 360.0f);
    float flEyeFootDelta = Helpers::angleDiff(animState->EyeYaw, m_flFootYaw);

    float flAimMatrixWidthRange = std::lerp(std::clamp(m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f), 1.0f, std::lerp(m_flWalkToRunTransition, 0.8f/*CSGO_ANIM_AIM_NARROW_WALK*/, 0.5f/*CSGO_ANIM_AIM_NARROW_RUN*/));

    if (animState->m_flAnimDuckAmount > 0)
    {
        flAimMatrixWidthRange = std::lerp(animState->m_flAnimDuckAmount * std::clamp(m_flSpeedAsPortionOfCrouchTopSpeed, 0.f, 1.f), flAimMatrixWidthRange, 0.5f/*CSGO_ANIM_AIM_NARROW_CROUCHMOVING*/);
    }

    float flTempYawMax = m_flAimYawMax * flAimMatrixWidthRange;
    float flTempYawMin = m_flAimYawMin * flAimMatrixWidthRange;

    if (flEyeFootDelta > flTempYawMax)
    {
        m_flFootYaw = animState->EyeYaw - abs(flTempYawMax);
    }
    else if (flEyeFootDelta < flTempYawMin)
    {
        m_flFootYaw = animState->EyeYaw + abs(flTempYawMin);
    }
    m_flFootYaw = Helpers::angleNormalize(m_flFootYaw);

    static float m_flLowerBodyRealignTimer = 0;

    if (m_flVelocityLengthXY > 0.1f || fabs(m_flAbsVelocityZ) > 100.0f)
    {
        m_flFootYaw = Helpers::approachAngle(animState->EyeYaw, m_flFootYaw, m_flLastUpdateIncrement * (30.0f + 20.0f * m_flWalkToRunTransition));
        m_flLowerBodyRealignTimer = memory->globalVars->currenttime + (/*CSGO_ANIM_LOWER_REALIGN_DELAY*/ 1.1f * 0.2f);
        if (entity->lby() != animState->EyeYaw)
            entity->lby() = animState->EyeYaw;
    }
    else
    {
        m_flFootYaw = Helpers::approachAngle(entity->lby(), m_flFootYaw, m_flLastUpdateIncrement * 100.f/*CSGO_ANIM_LOWER_CATCHUP_IDLE*/);

        if (memory->globalVars->currenttime > m_flLowerBodyRealignTimer && abs(Helpers::angleDiff(m_flFootYaw, animState->EyeYaw)) > 35.0f)
        {
            m_flLowerBodyRealignTimer = memory->globalVars->currenttime + /*CSGO_ANIM_LOWER_REALIGN_DELAY*/ 1.1f;
            if (entity->lby() != animState->EyeYaw)
                entity->lby() = animState->EyeYaw;
        }
    }

    if (m_flVelocityLengthXY <= 1.0f /*CS_PLAYER_SPEED_STOPPED*/ && animState->OnGround && !onLadder && !animState->InHitGroundAnimation && m_flLastUpdateIncrement > 0 && abs(Helpers::angleDiff(m_flFootYawLast, m_flFootYaw) / m_flLastUpdateIncrement > 120.f))
    {
        entity->setLayerSequence(ANIMATION_LAYER_ADJUST, ACT_CSGO_IDLE_TURN_BALANCEADJUST, entity->getModifiers());
        m_bAdjustStarted = true;
    }

    return;// original(thisPointer);
}

void __fastcall setupMovementHook(void* thisPointer, void* edx)
{
    static auto original = hooks->setupMovement.getOriginalDetour<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    if (!animState)
        return original(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->m_pPlayer);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    bool bPreviousGroundState = animState->m_bOnGround;
    bool onGround = entity->flags() & 1;
    bool m_bLandedOnGroundThisFrame = (!animState->m_bFirstRunSinceInit && bPreviousGroundState != onGround && onGround);
    bool m_bLeftTheGroundThisFrame = (bPreviousGroundState != onGround && !onGround);

    bool bPreviouslyOnLadder = animState->m_bOnLadder;
    bool onLadder = !onGround && entity->moveType() == MoveType::LADDER;
    bool bStartedLadderingThisFrame = (!bPreviouslyOnLadder && onLadder);
    bool bStoppedLadderingThisFrame = (bPreviouslyOnLadder && !onLadder);

    // get the user's left and right button pressed states
    bool moveRight = (Animations::data.cmd.buttons & (UserCmd::IN_MOVERIGHT)) != 0;
    bool moveLeft = (Animations::data.cmd.buttons & (UserCmd::IN_MOVELEFT)) != 0;
    bool moveForward = (Animations::data.cmd.buttons & (UserCmd::IN_FORWARD)) != 0;
    bool moveBackward = (Animations::data.cmd.buttons & (UserCmd::IN_BACK)) != 0;

    Vector vecForward;
    Vector vecRight;
    Vector::fromAngle(Vector{ 0.f, animState->m_flFootYaw, 0.f }, vecForward, vecRight, Vector{});
    vecRight.normalize();
    float flVelToRightDot = animState->m_vecVelocityNormalizedNonZero.dotProduct(vecRight);
    float flVelToForwardDot = animState->m_vecVelocityNormalizedNonZero.dotProduct(vecForward);

    // We're interested in if the player's desired direction (indicated by their held buttons) is opposite their current velocity.
    // This indicates a strafing direction change in progress.

    bool bStrafeRight = (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 0.73f && moveRight && !moveLeft && flVelToRightDot < -0.63f);
    bool bStrafeLeft = (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 0.73f && moveLeft && !moveRight && flVelToRightDot > 0.63f);
    bool bStrafeForward = (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 0.65f && moveForward && !moveBackward && flVelToForwardDot < -0.55f);
    bool bStrafeBackward = (animState->m_flSpeedAsPortionOfWalkTopSpeed >= 0.65f && moveBackward && !moveForward && flVelToForwardDot > 0.55f);

    entity->isStrafing() = (bStrafeRight || bStrafeLeft || bStrafeForward || bStrafeBackward);

    float ladderWeight = animState->m_flLadderWeight;
    if (ladderWeight > 0 || onLadder)
    {
        if (bStartedLadderingThisFrame)
        {
            animState->setLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, ACT_CSGO_CLIMB_LADDER, entity->getModifiers());
        }
        if (onLadder)
        {
            ladderWeight = Helpers::approach(1, ladderWeight, animState->m_flLastUpdateIncrement * 5.0f);
        }
        else
        {
            ladderWeight = Helpers::approach(0, ladderWeight, animState->m_flLastUpdateIncrement * 10.0f);
        }
        ladderWeight = std::clamp(ladderWeight, 0.f, 1.f);
    }

    if (onGround)
    {
        bool landing = animState->m_bLanding;
        if (!landing && (m_bLandedOnGroundThisFrame || bStoppedLadderingThisFrame))
        {
            int act = 0;
            if (durationInAir > 1)
                act = ACT_CSGO_LAND_HEAVY;
            else
                act = ACT_CSGO_LAND_LIGHT;
            animState->setLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, act, entity->getModifiers());
            landing = true;
        }
        if (landing && animState->getLayerActivity(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) != ACT_CSGO_CLIMB_LADDER)
        {
            Animations::data.m_bJumping = false;
        }

        if (entity->isLayerSequenceCompleted(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB))
        {
            landing = false;
        }

        if (!landing && !Animations::data.m_bJumping && ladderWeight <= 0)
        {
            animState->setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0);
        }

    }
    else if (!onLadder)
    {
        // we're in the air
        if (m_bLeftTheGroundThisFrame || bStoppedLadderingThisFrame)
        {
            if (!Animations::data.m_bJumping)
                animState->setLayerSequence(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, ACT_CSGO_FALL, entity->getModifiers());
        }
    }

    return original(thisPointer);
}

void __fastcall setupWeaponActionHook(void* thisPointer, void* edx)
{
    //static auto original = hooks->setupWeaponAction.getOriginalDetour<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    if (!animState)
        return;// original(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->m_pPlayer);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return;// original(thisPointer);
    
    animStateLayer nLayer = ANIMATION_LAYER_WEAPON_ACTION;
    bool bDoIncrement = true;
    auto m_pWeapon = animState->m_pWeapon;
    if (m_pWeapon && animState->m_bDeployRateLimiting && animState->getLayerActivity(ANIMATION_LAYER_WEAPON_ACTION) == ACT_CSGO_DEPLOY)
    {
        if (animState->getLayerCycle(ANIMATION_LAYER_WEAPON_ACTION) >= CSGO_ANIM_DEPLOY_RATELIMIT)
        {
            animState->m_bDeployRateLimiting = false;
            animState->setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION, ACT_CSGO_DEPLOY, entity->getModifiers());
            animState->setLayerWeight(ANIMATION_LAYER_WEAPON_ACTION, 0);
            bDoIncrement = false;
        }
    }

    // fixme: this is a hack to fix all-body weapon actions that need to transition into crouch or stand poses they weren't built for.
    // This only matters for idle animation - the move layer is itself a kind of 're-crouch' and 're-stand' layer itself.

    if (animState->m_nAnimstateModelVersion < 2)
    {
        // old re-crouch behavior

        // fixme: this is a hack to fix the all-body weapon action that wants to crouch case. There's no fix for the crouching all-body action that wants to stand
        if (animState->m_flAnimDuckAmount > 0 && animState->getLayerWeight(ANIMATION_LAYER_WEAPON_ACTION) > 0 && !animState->layerSequenceHasActMod(ANIMATION_LAYER_WEAPON_ACTION, "crouch"))
        {
            if (animState->getLayerSequence(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH) <= 0)
                animState->setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH, entity->lookupSequence("recrouch_generic"));
            animState->setLayerWeight(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH, animState->getLayerWeight(ANIMATION_LAYER_WEAPON_ACTION) * animState->m_flAnimDuckAmount);
        }
        else
        {
            animState->setLayerWeight(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH, 0);
        }
    }
    else
    {
        // newer version with "re-stand" blended into the re-crouch.

        float flTargetRecrouchWeight = 0;
        if (animState->getLayerWeight(ANIMATION_LAYER_WEAPON_ACTION) > 0)
        {
            if (animState->getLayerSequence(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH) <= 0)
                animState->setLayerSequence(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH, entity->lookupSequence("recrouch_generic"));

            if (animState->layerSequenceHasActMod(ANIMATION_LAYER_WEAPON_ACTION, "crouch"))
            {
                // this is a crouching anim. It might be the only anim available, or it's just lasted long enough that the
                // player stood up after it started. If we're standing up at all, we need to force the stand pose artificially.
                if (animState->m_flAnimDuckAmount < 1)
                    flTargetRecrouchWeight = animState->getLayerWeight(ANIMATION_LAYER_WEAPON_ACTION) * (1.0f - animState->m_flAnimDuckAmount);
            }
            else
            {
                // this is NOT a crouching anim. Still if it's not a whole-body anim it might work fine when crouched though,
                // and not actually need the re-crouch. How to detect this?

                // We can't trust this anim to crouch the player since it's not tagged as a crouch anim. So we need to force the
                // crouch pose artificially.
                if (animState->duckAmount > 0)
                    flTargetRecrouchWeight = animState->getLayerWeight(ANIMATION_LAYER_WEAPON_ACTION) * animState->m_flAnimDuckAmount;
            }
        }
        else
        {
            if (animState->getLayerWeight(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH) > 0)
                flTargetRecrouchWeight = Helpers::approach(0, animState->getLayerWeight(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH), animState->m_flLastUpdateIncrement * 4.f);
        }
        animState->setLayerWeight(ANIMATION_LAYER_WEAPON_ACTION_RECROUCH, flTargetRecrouchWeight);
    }

    if (bDoIncrement)
    {
        // increment the action
        animState->incrementLayerCycle(ANIMATION_LAYER_WEAPON_ACTION, false);
        float flWeightPrev = animState->getLayerWeight(ANIMATION_LAYER_WEAPON_ACTION);
        float flDesiredWeight = animState->getLayerIdealWeightFromSeqCycle(ANIMATION_LAYER_WEAPON_ACTION);

        animState->setLayerWeight(ANIMATION_LAYER_WEAPON_ACTION, flDesiredWeight);
        animState->setLayerWeightRate(ANIMATION_LAYER_WEAPON_ACTION, flWeightPrev);
    }

    // set weapon sequence and cycle so dispatched events hit
    AnimationLayer* pWeaponLayer = entity->getAnimationLayer(ANIMATION_LAYER_WEAPON_ACTION);
    if (pWeaponLayer && m_pWeapon)
    {
        CBaseWeaponWorldModel* pWeaponWorldModel = m_pWeapon->m_hWeaponWorldModel.Get();
        if (pWeaponWorldModel)
        {
            //MDLCACHE_CRITICAL_SECTION();
            if (pWeaponLayer->m_nDispatchedDst > 0 && pWeaponLayer->GetWeight() > 0) // fixme: does the weight check make 0-frame events fail? Added a check below just in case.
            {
                pWeaponWorldModel->SetSequence(pWeaponLayer->m_nDispatchedDst);
                pWeaponWorldModel->SetCycle(pWeaponLayer->GetCycle());
                pWeaponWorldModel->SetPlaybackRate(pWeaponLayer->GetPlaybackRate());
            }
            else
            {
                pWeaponWorldModel->SetSequence(0);
                pWeaponWorldModel->SetCycle(0);
                pWeaponWorldModel->SetPlaybackRate(0);
            }
        }
    }
    
    return;// original(thisPointer);
}

//DONE