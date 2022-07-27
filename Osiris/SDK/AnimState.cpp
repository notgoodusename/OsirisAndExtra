#include "AnimState.h"
#include "Entity.h"
#include "UserCmd.h"

void AnimState::setupVelocity() noexcept
{
    interfaces->mdlCache->beginLock();

    auto entity = reinterpret_cast<Entity*>(player);

    Vector vecAbsVelocity = entity->getAbsVelocity();

    // We should call this only if the entity isnt the localPlayer, since we cant figure out other players velocity
    /*
    if (localPlayer && localPlayer.get() != player)
    {
        // prevent the client input velocity vector from exceeding a reasonable magnitude
        if (vecAbsVelocity.squareLength() > std::sqrt(CS_PLAYER_SPEED_RUN * CSGO_ANIM_MAX_VEL_LIMIT))
            vecAbsVelocity = vecAbsVelocity.normalized() * (CS_PLAYER_SPEED_RUN * CSGO_ANIM_MAX_VEL_LIMIT);
    }
    */

    //this is a hack to fix shuffling animations due to rounding errors, not part of actual game code
    if (fabsf(vecAbsVelocity.x) < 0.001)
        vecAbsVelocity.x = 0.0f;
    if (fabsf(vecAbsVelocity.y) < 0.001)
        vecAbsVelocity.y = 0.0f;
    if (fabsf(vecAbsVelocity.z) < 0.001)
        vecAbsVelocity.z = 0.0f;

    // save vertical velocity component
    velocityLengthZ = vecAbsVelocity.z;

    // discard z component
    vecAbsVelocity.z = 0.f;

    // remember if the player is accelerating.
    playerIsAccelerating = (vecVelocityLast.squareLength() < vecAbsVelocity.squareLength());

    // rapidly approach ideal velocity instead of instantly adopt it. This helps smooth out instant velocity changes, like
    // when the player runs headlong into a wall and their velocity instantly becomes zero.
    vecVelocity = Helpers::approach(vecAbsVelocity, vecVelocity, lastUpdateIncrement * 2000.f);
    vecVelocityNormalized = vecVelocity.normalized();

    // save horizontal velocity length
    velocityLengthXY = std::fminf(vecVelocity.length(), CS_PLAYER_SPEED_RUN);

    if (velocityLengthXY > 0.f)
        vecVelocityNormalizedNonZero = vecVelocityNormalized;

    auto currentWeapon = reinterpret_cast<Entity*>(weapon);
    float maxSpeedRun = currentWeapon ? std::fmaxf(currentWeapon->getMaxSpeed(), 0.001f) : CS_PLAYER_SPEED_RUN;

    //compute speed in various normalized forms
    speedAsPortionOfRunTopSpeed = std::clamp(velocityLengthXY / maxSpeedRun, 0.f, 1.f);
    speedAsPortionOfWalkTopSpeed = velocityLengthXY / (maxSpeedRun * CS_PLAYER_SPEED_WALK_MODIFIER);
    speedAsPortionOfCrouchTopSpeed = velocityLengthXY / (maxSpeedRun * CS_PLAYER_SPEED_DUCK_MODIFIER);

    if (speedAsPortionOfWalkTopSpeed >= 1.f)
    {
        staticApproachSpeed = velocityLengthXY;
    }
    else if (speedAsPortionOfWalkTopSpeed < 0.5f)
    {
        staticApproachSpeed = Helpers::approach(80.f, staticApproachSpeed, lastUpdateIncrement * 60.f);
    }

    bool startedMovingThisFrame = false;
    bool stoppedMovingThisFrame = false;

    if (velocityLengthXY > 0.f)
    {
        startedMovingThisFrame = (durationMoving <= 0.f);
        durationStill = 0.f;
        durationMoving += lastUpdateIncrement;
    }
    else
    {
        stoppedMovingThisFrame = (durationStill <= 0.f);
        durationMoving = 0.f;
        durationStill += lastUpdateIncrement;
    }

    if (!adjustStarted && stoppedMovingThisFrame && onGround && !onLadder && !landing && stutterStep < 50)
    {
        setLayerSequence(ANIMATION_LAYER_ADJUST, ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING);
        adjustStarted = true;
    }

    if (getLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ||
        getLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
    {
        if (adjustStarted && speedAsPortionOfCrouchTopSpeed <= 0.25f)
        {
            incrementLayerCycleWeightRateGeneric(ANIMATION_LAYER_ADJUST);
            adjustStarted = !(isLayerSequenceCompleted(ANIMATION_LAYER_ADJUST));
        }
        else
        {
            adjustStarted = false;
            float weight = getLayerWeight(ANIMATION_LAYER_ADJUST);
            setLayerWeight(ANIMATION_LAYER_ADJUST, Helpers::approach(0.f, weight, lastUpdateIncrement * 5.f));
            setLayerWeightRate(ANIMATION_LAYER_ADJUST, weight);
        }
    }

    footYawLast = footYaw;
    footYaw = std::clamp(footYaw, -360.0f, 360.0f);
    float eyeFootDelta = Helpers::angleDiff(eyeYaw, footYaw);

    float aimMatrixWidthRange = Helpers::lerp(std::clamp(speedAsPortionOfWalkTopSpeed, 0.f, 1.f), 1.0f, Helpers::lerp(walkToRunTransition, CSGO_ANIM_AIM_NARROW_WALK, CSGO_ANIM_AIM_NARROW_RUN));

    if (animDuckAmount > 0.f)
    {
        aimMatrixWidthRange = Helpers::lerp(animDuckAmount * std::clamp(speedAsPortionOfCrouchTopSpeed, 0.f, 1.f), aimMatrixWidthRange, CSGO_ANIM_AIM_NARROW_CROUCHMOVING);
    }

    float tempYawMax = aimYawMax * aimMatrixWidthRange;
    float tempYawMin = aimYawMin * aimMatrixWidthRange;

    if (eyeFootDelta > tempYawMax)
    {
        footYaw = eyeYaw - fabs(tempYawMax);
    }
    else if (eyeFootDelta < tempYawMin)
    {
        footYaw = eyeYaw + fabs(tempYawMin);
    }
    footYaw = Helpers::angleNormalize(footYaw);

    if (velocityLengthXY > 0.1f || fabs(velocityLengthZ) > 100.0f)
    {
        footYaw = Helpers::approachAngle(eyeYaw, footYaw, lastUpdateIncrement * (30.0f + 20.0f * walkToRunTransition));
        lowerBodyRealignTimer = lastUpdateTime + (CSGO_ANIM_LOWER_REALIGN_DELAY * 0.2f);
    }
    else
    {
        footYaw = Helpers::approachAngle(entity->lby(), footYaw, lastUpdateIncrement * CSGO_ANIM_LOWER_CATCHUP_IDLE);

        if (lastUpdateTime > lowerBodyRealignTimer && fabsf(Helpers::angleDiff(footYaw, eyeYaw)) > 35.0f)
        {
            lowerBodyRealignTimer = lastUpdateTime + CSGO_ANIM_LOWER_REALIGN_DELAY;
        }
    }

    if (velocityLengthXY <= CS_PLAYER_SPEED_STOPPED && onGround && !onLadder && !landing && lastUpdateIncrement > 0.f && std::fabsf(Helpers::angleDiff(footYawLast, footYaw) / lastUpdateIncrement > 120.f))
    {
        setLayerSequence(ANIMATION_LAYER_ADJUST, ACT_CSGO_IDLE_TURN_BALANCEADJUST);
        adjustStarted = true;
    }

    // the final model render yaw is aligned to the foot yaw
    if (velocityLengthXY > 0.f)
    {
        // convert horizontal velocity vec to angular yaw
        float rawYawIdeal = (std::atan2(-vecVelocity.y, -vecVelocity.x) * 180 / 3.141592654f);
        if (rawYawIdeal < 0.f)
            rawYawIdeal += 360.f;

        moveYawIdeal = Helpers::angleNormalize(Helpers::angleDiff(rawYawIdeal, footYaw));
    }

    // delta between current yaw and ideal velocity derived target (possibly negative!)
    moveYawCurrentToIdeal = Helpers::angleNormalize(Helpers::angleDiff(moveYawIdeal, moveYaw));

    if (startedMovingThisFrame && moveWeight <= 0.f)
    {
        moveYaw = moveYawIdeal;

        // select a special starting cycle that's set by the animator in content
        int moveSeq = getLayerSequence(ANIMATION_LAYER_MOVEMENT_MOVE);
        if (moveSeq != -1)
        {
            StudioSeqdesc seqdesc = entity->getModelPtr()->seqdesc(moveSeq);
            if (seqdesc.numAnimTags > 0)
            {
                if (fabsf(Helpers::angleDiff(moveYaw, 180)) <= EIGHT_WAY_WIDTH) //N
                {
                    primaryCycle = entity->getFirstSequenceAnimTag(moveSeq, ANIMTAG_STARTCYCLE_N);
                }
                else if (fabsf(Helpers::angleDiff(moveYaw, 135)) <= EIGHT_WAY_WIDTH) //NE
                {
                    primaryCycle = entity->getFirstSequenceAnimTag(moveSeq, ANIMTAG_STARTCYCLE_NE);
                }
                else if (fabsf(Helpers::angleDiff(moveYaw, 90)) <= EIGHT_WAY_WIDTH) //E
                {
                    primaryCycle = entity->getFirstSequenceAnimTag(moveSeq, ANIMTAG_STARTCYCLE_E);
                }
                else if (fabsf(Helpers::angleDiff(moveYaw, 45)) <= EIGHT_WAY_WIDTH) //SE
                {
                    primaryCycle = entity->getFirstSequenceAnimTag(moveSeq, ANIMTAG_STARTCYCLE_SE);
                }
                else if (fabsf(Helpers::angleDiff(moveYaw, 0)) <= EIGHT_WAY_WIDTH) //S
                {
                    primaryCycle = entity->getFirstSequenceAnimTag(moveSeq, ANIMTAG_STARTCYCLE_S);
                }
                else if (fabsf(Helpers::angleDiff(moveYaw, -45)) <= EIGHT_WAY_WIDTH) //SW
                {
                    primaryCycle = entity->getFirstSequenceAnimTag(moveSeq, ANIMTAG_STARTCYCLE_SW);
                }
                else if (fabsf(Helpers::angleDiff(moveYaw, -90)) <= EIGHT_WAY_WIDTH) //W
                {
                    primaryCycle = entity->getFirstSequenceAnimTag(moveSeq, ANIMTAG_STARTCYCLE_W);
                }
                else if (fabsf(Helpers::angleDiff(moveYaw, -135)) <= EIGHT_WAY_WIDTH) //NW
                {
                    primaryCycle = entity->getFirstSequenceAnimTag(moveSeq, ANIMTAG_STARTCYCLE_NW);
                }
            }
        }

        /*
        if (inAirSmoothValue >= 1 && !firstRunSinceInit && abs(moveYawCurrentToIdeal) > 45 && onGround && entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].GetCurrentWeight() <= 0)
        {
            entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].SetShouldCapture(bonesnapshot_get(cl_bonesnapshot_speed_movebegin));
        }
        */
    }
    else
    {
        if (getLayerWeight(ANIMATION_LAYER_MOVEMENT_STRAFECHANGE) >= 1.f)
        {
            moveYaw = moveYawIdeal;
        }
        else
        {
            /*
            if (inAirSmoothValue >= 1 && !firstRunSinceInit && abs(moveYawCurrentToIdeal) > 100 && onGround && entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].GetCurrentWeight() <= 0)
            {
                entity->m_boneSnapshots[BONESNAPSHOT_ENTIRE_BODY].SetShouldCapture(bonesnapshot_get(cl_bonesnapshot_speed_movebegin));
            }
            */
            float moveWeight = Helpers::lerp(animDuckAmount, std::clamp(speedAsPortionOfWalkTopSpeed, 0.f, 1.f), std::clamp(speedAsPortionOfCrouchTopSpeed, 0.f, 1.f));
            float ratio = Helpers::bias(moveWeight, 0.18f) + 0.1f;

            moveYaw = Helpers::angleNormalize(moveYaw + (moveYawCurrentToIdeal * ratio));
        }
    }
    poseParamMappings[PLAYER_POSE_PARAM_MOVE_YAW].setValue(entity, moveYaw);

    float aimYaw = Helpers::angleDiff(eyeYaw, footYaw);
    if (aimYaw >= 0.f && aimYawMax != 0.f)
    {
        aimYaw = (aimYaw / aimYawMax) * 60.0f;
    }
    else if (aimYawMin != 0.f)
    {
        aimYaw = (aimYaw / aimYawMin) * -60.0f;
    }

    poseParamMappings[PLAYER_POSE_PARAM_BODY_YAW].setValue(entity, aimYaw);

    // we need non-symmetrical arbitrary min/max bounds for vertical aim (pitch) too
    float pitch = Helpers::angleDiff(eyePitch, 0.f);
    if (pitch > 0.f)
    {
        pitch = (pitch / aimPitchMax) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MAX;
    }
    else
    {
        pitch = (pitch / aimPitchMin) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MIN;
    }

    poseParamMappings[PLAYER_POSE_PARAM_BODY_PITCH].setValue(entity, pitch);
    poseParamMappings[PLAYER_POSE_PARAM_SPEED].setValue(entity, speedAsPortionOfWalkTopSpeed);
    poseParamMappings[PLAYER_POSE_PARAM_STAND].setValue(entity, 1.0f - (animDuckAmount * inAirSmoothValue));

    interfaces->mdlCache->endLock();
}

void AnimState::setupMovement() noexcept
{
    interfaces->mdlCache->beginLock();

    auto entity = reinterpret_cast<Entity*>(player);

    if (walkToRunTransition > 0.f && walkToRunTransition < 1.f)
    {
        //currently transitioning between walk and run
        if (walkToRunTransitionState == ANIM_TRANSITION_WALK_TO_RUN)
        {
            walkToRunTransition += lastUpdateIncrement * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
        }
        else // walkToRunTransitionState == ANIM_TRANSITION_RUN_TO_WALK
        {
            walkToRunTransition -= lastUpdateIncrement * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
        }
        walkToRunTransition = std::clamp(walkToRunTransition, 0.f, 1.f);
    }

    if (velocityLengthXY > (CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER) && walkToRunTransitionState == ANIM_TRANSITION_RUN_TO_WALK)
    {
        //crossed the walk to run threshold
        walkToRunTransitionState = ANIM_TRANSITION_WALK_TO_RUN;
        walkToRunTransition = std::fmaxf(0.01f, walkToRunTransition);
    }
    else if (velocityLengthXY < (CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER) && walkToRunTransitionState == ANIM_TRANSITION_WALK_TO_RUN)
    {
        //crossed the run to walk threshold
        walkToRunTransitionState = ANIM_TRANSITION_RUN_TO_WALK;
        walkToRunTransition = std::fmaxf(0.99f, walkToRunTransition);
    }

    if (animstateModelVersion < 2)
    {
        poseParamMappings[PLAYER_POSE_PARAM_RUN].setValue(entity, walkToRunTransition);
    }
    else
    {
        poseParamMappings[PLAYER_POSE_PARAM_MOVE_BLEND_WALK].setValue(entity, (1.0f - walkToRunTransition) * (1.0f - animDuckAmount));
        poseParamMappings[PLAYER_POSE_PARAM_MOVE_BLEND_RUN].setValue(entity, (walkToRunTransition) * (1.0f - animDuckAmount));
        poseParamMappings[PLAYER_POSE_PARAM_MOVE_BLEND_CROUCH_WALK].setValue(entity, animDuckAmount);
    }

    char weaponMoveSequenceString[MAX_ANIMSTATE_ANIMNAME_CHARS];
    sprintf(weaponMoveSequenceString, "move_%s", memory->getWeaponPrefix(this));

    int weaponMoveSeq = entity->lookupSequence(weaponMoveSequenceString);
    if (weaponMoveSeq == -1)
        weaponMoveSeq = entity->lookupSequence("move");

    if (entity->moveState() != previousMoveState)
        stutterStep += 10.f;

    previousMoveState = entity->moveState();
    stutterStep = std::clamp(Helpers::approach(0.f, stutterStep, lastUpdateIncrement * 40.f), 0.f, 100.f);

    // recompute moveweight
    float targetMoveWeight = Helpers::lerp(animDuckAmount, std::clamp(speedAsPortionOfWalkTopSpeed, 0.f, 1.f), std::clamp(speedAsPortionOfCrouchTopSpeed, 0.f, 1.f));

    if (moveWeight <= targetMoveWeight)
    {
        moveWeight = targetMoveWeight;
    }
    else
    {
        moveWeight = Helpers::approach(targetMoveWeight, moveWeight, lastUpdateIncrement * Helpers::remapValClamped(stutterStep, 0.0f, 100.0f, 2.f, 20.f));
    }

    Vector vecMoveYawDir;
    Vector::fromAngle(Vector{ 0.f, Helpers::angleNormalize(footYaw + moveYaw + 180.f), 0.f }, &vecMoveYawDir);
    float yawDeltaAbsDot = std::fabsf(vecVelocityNormalizedNonZero.dotProduct(vecMoveYawDir));
    moveWeight *= Helpers::bias(yawDeltaAbsDot, 0.2f);

    float moveWeightWithAirSmooth = moveWeight * inAirSmoothValue;

    // dampen move weight for landings
    moveWeightWithAirSmooth *= std::fmaxf((1.0f - getLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB)), 0.55f);

    float moveCycleRate = 0.f;
    if (velocityLengthXY > 0.f)
    {
        moveCycleRate = entity->getSequenceCycleRate(weaponMoveSeq);
        float sequenceGroundSpeed = std::fmaxf(entity->getSequenceMoveDist(entity->getModelPtr(), weaponMoveSeq) / (1.0f / moveCycleRate), 0.001f);
        moveCycleRate *= velocityLengthXY / sequenceGroundSpeed;

        moveCycleRate *= Helpers::lerp(walkToRunTransition, 1.0f, CSGO_ANIM_RUN_ANIM_PLAYBACK_MULTIPLIER);
    }

    float localCycleIncrement = (moveCycleRate * lastUpdateIncrement);

    primaryCycle = Helpers::clampCycle(primaryCycle + localCycleIncrement);

    moveWeightWithAirSmooth = std::clamp(moveWeightWithAirSmooth, 0.f, 1.f);
    updateAnimLayer(ANIMATION_LAYER_MOVEMENT_MOVE, weaponMoveSeq, localCycleIncrement, moveWeightWithAirSmooth, primaryCycle);

    // blend in a strafe direction-change pose when the player changes strafe dir

    // get the user's left and right button pressed states
    bool moveRight = (buttons & (UserCmd::IN_MOVERIGHT)) != 0;
    bool moveLeft = (buttons & (UserCmd::IN_MOVELEFT)) != 0;
    bool moveForward = (buttons & (UserCmd::IN_FORWARD)) != 0;
    bool moveBackward = (buttons & (UserCmd::IN_BACK)) != 0;

    Vector vecForward;
    Vector vecRight;
    Vector::fromAngleAll(Vector{ 0.f, footYaw, 0.f }, &vecForward, &vecRight, nullptr);
    vecRight = vecRight.normalized();
    float velToRightDot = vecVelocityNormalizedNonZero.dotProduct(vecRight);
    float velToForwardDot = vecVelocityNormalizedNonZero.dotProduct(vecForward);

    // We're interested in if the player's desired direction (indicated by their held buttons) is opposite their current velocity.
    // This indicates a strafing direction change in progress.

    bool bStrafeRight = (speedAsPortionOfWalkTopSpeed >= 0.73f && moveRight && !moveLeft && velToRightDot < -0.63f);
    bool bStrafeLeft = (speedAsPortionOfWalkTopSpeed >= 0.73f && moveLeft && !moveRight && velToRightDot > 0.63f);
    bool bStrafeForward = (speedAsPortionOfWalkTopSpeed >= 0.65f && moveForward && !moveBackward && velToForwardDot < -0.55f);
    bool bStrafeBackward = (speedAsPortionOfWalkTopSpeed >= 0.65f && moveBackward && !moveForward && velToForwardDot > 0.55f);

    entity->isStrafing() = (bStrafeRight || bStrafeLeft || bStrafeForward || bStrafeBackward);

    if (entity->isStrafing())
    {
        if (!strafeChanging)
        {
            durationStrafing = 0.f;

            /*
            if (!firstRunSinceInit && !strafeChanging && onGround && entity->m_boneSnapshots[BONESNAPSHOT_UPPER_BODY].GetCurrentWeight() <= 0)
            {
                entity->m_boneSnapshots[BONESNAPSHOT_UPPER_BODY].SetShouldCapture(bonesnapshot_get(cl_bonesnapshot_speed_strafestart));
            }
            */
        }

        strafeChanging = true;

        strafeChangeWeight = Helpers::approach(1.f, strafeChangeWeight, lastUpdateIncrement * 20.f);
        strafeChangeCycle = Helpers::approach(0.f, strafeChangeCycle, lastUpdateIncrement * 10.f);

        poseParamMappings[PLAYER_POSE_PARAM_STRAFE_DIR].setValue(entity, Helpers::angleNormalize(moveYaw));

    }
    else if (strafeChangeWeight > 0.f)
    {
        durationStrafing += lastUpdateIncrement;

        if (durationStrafing > 0.08f)
            strafeChangeWeight = Helpers::approach(0.f, strafeChangeWeight, lastUpdateIncrement * 5.f);

        strafeSequence = entity->lookupSequence("strafe");
        float rate = entity->getSequenceCycleRate(strafeSequence);
        strafeChangeCycle = std::clamp(strafeChangeCycle + lastUpdateIncrement * rate, 0.f, 1.f);
    }

    if (strafeChangeWeight <= 0.f)
    {
        strafeChanging = false;
    }

    // keep track of if the player is on the ground, and if the player has just touched or left the ground since the last check
    bool previousGroundState = onGround;
    onGround = (entity->flags() & 1);

    landedOnGroundThisFrame = (!firstRunSinceInit && previousGroundState != onGround && onGround);
    leftTheGroundThisFrame = (previousGroundState != onGround && !onGround);

    float distanceFell = 0.f;
    if (leftTheGroundThisFrame)
    {
        leftGroundHeight = vecPositionCurrent.z;
    }

    if (landedOnGroundThisFrame)
    {
        distanceFell = fabs(leftGroundHeight - vecPositionCurrent.z);
        float distanceFallNormalizedBiasRange = Helpers::bias(Helpers::remapValClamped(distanceFell, 12.0f, 72.0f, 0.0f, 1.0f), 0.4f);

        landAnimMultiplier = std::clamp(Helpers::bias(durationInAir, 0.3f), 0.1f, 1.0f);
        duckAdditional = std::fmax(landAnimMultiplier, distanceFallNormalizedBiasRange);

    }
    else
    {
        duckAdditional = Helpers::approach(0.f, duckAdditional, lastUpdateIncrement * 2.f);
    }

    // the in-air smooth value is a fuzzier representation of if the player is on the ground or not.
    // It will approach 1 when the player is on the ground and 0 when in the air. Useful for blending jump animations.
    inAirSmoothValue = Helpers::approach(onGround ? 1.f : 0.f, inAirSmoothValue, Helpers::lerp(animDuckAmount, CSGO_ANIM_ONGROUND_FUZZY_APPROACH, CSGO_ANIM_ONGROUND_FUZZY_APPROACH_CROUCH) * lastUpdateIncrement);
    inAirSmoothValue = std::clamp(inAirSmoothValue, 0.f, 1.f);



    strafeChangeWeight *= (1.0f - animDuckAmount);
    strafeChangeWeight *= inAirSmoothValue;
    strafeChangeWeight = std::clamp(strafeChangeWeight, 0.f, 1.f);

    if (strafeSequence != -1)
        updateAnimLayer(ANIMATION_LAYER_MOVEMENT_STRAFECHANGE, strafeSequence, 0.f, strafeChangeWeight, strafeChangeCycle);

    //ladders
    bool previouslyOnLadder = onLadder;
    onLadder = !onGround && entity->moveType() == MoveType::LADDER;
    bool startedLadderingThisFrame = (!previouslyOnLadder && onLadder);
    bool stoppedLadderingThisFrame = (previouslyOnLadder && !onLadder);

    if (ladderWeight > 0.f || onLadder)
    {
        if (startedLadderingThisFrame)
        {
            setLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, ACT_CSGO_CLIMB_LADDER);
        }

        if (std::abs(velocityLengthZ) > 100.f)
        {
            ladderSpeed = Helpers::approach(1, ladderSpeed, lastUpdateIncrement * 10.0f);
        }
        else
        {
            ladderSpeed = Helpers::approach(0.f, ladderSpeed, lastUpdateIncrement * 10.0f);
        }
        ladderSpeed = std::clamp(ladderSpeed, 0.f, 1.f);

        if (onLadder)
        {
            ladderWeight = Helpers::approach(1.f, ladderWeight, lastUpdateIncrement * 5.0f);
        }
        else
        {
            ladderWeight = Helpers::approach(0.f, ladderWeight, lastUpdateIncrement * 10.0f);
        }
        ladderWeight = std::clamp(ladderWeight, 0.f, 1.f);

        Vector vecLadderNormal = entity->getLadderNormal();
        Vector angLadder;
        angLadder = vecLadderNormal.toAngle();
        float ladderYaw = Helpers::angleDiff(angLadder.y, footYaw);
        poseParamMappings[PLAYER_POSE_PARAM_LADDER_YAW].setValue(entity, ladderYaw);

        float ladderClimbCycle = getLayerCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB);
        ladderClimbCycle += (vecPositionCurrent.z - vecPositionLast.z) * Helpers::lerp(ladderSpeed, 0.010f, 0.004f);

        poseParamMappings[PLAYER_POSE_PARAM_LADDER_SPEED].setValue(entity, ladderSpeed);

        if (getLayerActivity(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) == ACT_CSGO_CLIMB_LADDER)
        {
            setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, ladderWeight);
        }

        setLayerCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, ladderClimbCycle);

        // fade out jump if we're climbing
        if (onLadder)
        {
            float idealJumpWeight = 1.0f - ladderWeight;
            if (getLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL) > idealJumpWeight)
            {
                setLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, idealJumpWeight);
            }
        }
    }
    else
    {
        ladderSpeed = 0.f;
    }

    if (onGround)
    {
        if (!landing && (landedOnGroundThisFrame || stoppedLadderingThisFrame))
        {
            if(durationInAir > 1.f)
                setLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, ACT_CSGO_LAND_HEAVY);
            else
                setLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, ACT_CSGO_LAND_LIGHT);
            setLayerCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0.f);
            landing = true;
        }
        durationInAir = 0.f;

        if (landing && getLayerActivity(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) != ACT_CSGO_CLIMB_LADDER)
        {
            jumping = false;

            incrementLayerCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, false);
            incrementLayerCycle(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false);

            poseParamMappings[PLAYER_POSE_PARAM_JUMP_FALL].setValue(entity, 0.f);

            if (isLayerSequenceCompleted(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB))
            {
                landing = false;
                setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0.f);
                setLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, 0.f);
                landAnimMultiplier = 1.0f;
            }
            else
            {

                float landWeight = getLayerIdealWeightFromSeqCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) * landAnimMultiplier;

                // if we hit the ground crouched, reduce the land animation as a function of crouch, since the land animations move the head up a bit ( and this is undesirable )
                landWeight *= std::clamp((1.0f - animDuckAmount), 0.2f, 1.0f);

                setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, landWeight);

                // fade out jump because land is taking over
                float currentJumpFallWeight = getLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
                if (currentJumpFallWeight > 0.f)
                {
                    currentJumpFallWeight = Helpers::approach(0.f, currentJumpFallWeight, lastUpdateIncrement * 10.0f);
                    setLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, currentJumpFallWeight);
                }

            }
        }

        if (!landing && !jumping && ladderWeight <= 0.f)
        {
            setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0.f);
        }
    }
    else if (!onLadder)
    {
        landing = false;

        // we're in the air
        if (leftTheGroundThisFrame || stoppedLadderingThisFrame)
        {
            // If entered the air by jumping, then we already set the jump activity.
            // But if we're in the air because we strolled off a ledge or the floor collapsed or something,
            // we need to set the fall activity here.
            if (!jumping)
            {
                setLayerSequence(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, ACT_CSGO_FALL);
            }

            durationInAir = 0.f;
        }

        durationInAir += lastUpdateIncrement;

        incrementLayerCycle(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false);

        // increase jump weight
        float jumpWeight = getLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
        float nextJumpWeight = getLayerIdealWeightFromSeqCycle(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
        if (nextJumpWeight > jumpWeight)
        {
            setLayerWeight(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, nextJumpWeight);
        }

        // bash any lingering land weight to zero
        float lingeringLandWeight = getLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB);
        if (lingeringLandWeight > 0)
        {
            lingeringLandWeight *= Helpers::smoothStepBounds(0.2f, 0.0f, durationInAir);
            setLayerWeight(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, lingeringLandWeight);
        }

        // blend jump into fall. This is a no-op if we're playing a fall anim.
        poseParamMappings[PLAYER_POSE_PARAM_JUMP_FALL].setValue(entity, std::clamp(Helpers::smoothStepBounds(0.72f, 1.52f, durationInAir), 0.f, 1.f));
    }
    interfaces->mdlCache->endLock();
}

void AnimState::setupAliveLoop() noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    if (getLayerActivity(ANIMATION_LAYER_ALIVELOOP) != ACT_CSGO_ALIVE_LOOP)
    {
        interfaces->mdlCache->beginLock();
        setLayerSequence(ANIMATION_LAYER_ALIVELOOP, ACT_CSGO_ALIVE_LOOP);
        setLayerCycle(ANIMATION_LAYER_ALIVELOOP, memory->randomFloat(0.f, 1.f));
        auto layer = entity->getAnimationLayer(ANIMATION_LAYER_ALIVELOOP);
        if (layer)
        {
            float newRate = entity->getSequenceCycleRate(layer->sequence);
            newRate *= memory->randomFloat(0.8f, 1.1f);
            setLayerRate(ANIMATION_LAYER_ALIVELOOP, newRate);
        }
        interfaces->mdlCache->endLock();
        incrementLayerCycle(ANIMATION_LAYER_ALIVELOOP, true);
    }
    else
    {
        if (weapon && weapon != weaponLast)
        {
            //re-roll act on weapon change
            float retainCycle = getLayerCycle(ANIMATION_LAYER_ALIVELOOP);
            setLayerSequence(ANIMATION_LAYER_ALIVELOOP, ACT_CSGO_ALIVE_LOOP);
            setLayerCycle(ANIMATION_LAYER_ALIVELOOP, retainCycle);
            incrementLayerCycle(ANIMATION_LAYER_ALIVELOOP, true);
        }
        else if (isLayerSequenceCompleted(ANIMATION_LAYER_ALIVELOOP))
        {
            //re-roll rate
            interfaces->mdlCache->beginLock();
            auto layer = entity->getAnimationLayer(ANIMATION_LAYER_ALIVELOOP);
            if (layer)
            {
                float newRate = entity->getSequenceCycleRate(layer->sequence);
                newRate *= memory->randomFloat(0.8f, 1.1f);
                setLayerRate(ANIMATION_LAYER_ALIVELOOP, newRate);
            }
            interfaces->mdlCache->endLock();
            incrementLayerCycle(ANIMATION_LAYER_ALIVELOOP, true);
        }
        else
        {
            float weightOutPoseBreaker = Helpers::remapValClamped(speedAsPortionOfRunTopSpeed, 0.55f, 0.9f, 1.0f, 0.0f);
            setLayerWeight(ANIMATION_LAYER_ALIVELOOP, weightOutPoseBreaker);
            incrementLayerCycle(ANIMATION_LAYER_ALIVELOOP, true);
        }
    }
}

void AnimState::doAnimationEvent(int animationEvent) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    switch (animationEvent)
    {
    case PLAYERANIMEVENT_COUNT:
        return;
    case PLAYERANIMEVENT_THROW_GRENADE_UNDERHAND:
    {
        //addActivityModifier("underhand");
    }
    case PLAYERANIMEVENT_FIRE_GUN_PRIMARY:
    case PLAYERANIMEVENT_THROW_GRENADE:
    {
        auto currentWeapon = reinterpret_cast<Entity*>(weapon);
        if (!currentWeapon)
            break;

        if (currentWeapon && animationEvent == PLAYERANIMEVENT_FIRE_GUN_PRIMARY && currentWeapon->isBomb())
        {
            setLayerSequence(ANIMATION_LAYER_WHOLE_BODY, ACT_CSGO_PLANT_BOMB);
            plantAnimStarted = true;
            //*reinterpret_cast<int*>(animState + 316) = 1;//+316 plantAnimStarted
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
        auto currentWeapon = reinterpret_cast<Entity*>(weapon);
        if (!currentWeapon)
            break;

        if (currentWeapon->isSniperRifle())
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
        auto currentWeapon = reinterpret_cast<Entity*>(weapon);
        if (!currentWeapon)
            break;

        if (currentWeapon && currentWeapon->isShotgun() && currentWeapon->itemDefinitionIndex2() != WeaponId::Mag7)
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
        plantAnimStarted = false;
        break;
    }
    case PLAYERANIMEVENT_DEPLOY:
    {
        auto currentWeapon = reinterpret_cast<Entity*>(weapon);
        if (!currentWeapon)
            break;

        if (currentWeapon && getLayerActivity(ANIMATION_LAYER_WEAPON_ACTION) == ACT_CSGO_DEPLOY &&
            getLayerCycle(ANIMATION_LAYER_WEAPON_ACTION) < 0.15f/*CSGO_ANIM_DEPLOY_RATELIMIT*/)
        {
            // we're already deploying
            deployRateLimiting = true;
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
        jumping = true;
        setLayerSequence(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, ACT_CSGO_JUMP);
        break;
    }
    default:
        break;
    }
}

void AnimState::updateAnimLayer(size_t layerIndex, int sequence, float playbackRate, float weight, float cycle) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    if (sequence >= 2)
    {
        interfaces->mdlCache->beginLock();
        auto& layer = *entity->getAnimationLayer(layerIndex);
        if (&layer)
        {
            layer.sequence = sequence;
            layer.playbackRate = playbackRate;
            layer.cycle = std::clamp(cycle, 0.0f, 1.0f);
            layer.weight = std::clamp(weight, 0.0f, 1.0f);
        }
        interfaces->mdlCache->endLock();
    }
}

bool animstatePoseParamCache::init(Entity* player, char* poseParamName) noexcept
{
    interfaces->mdlCache->beginLock();
    name = poseParamName;
    int poseIndex = memory->lookUpPoseParameter(player->getModelPtr(), poseParamName);
    index = poseIndex;
    if (poseIndex != -1)
        initialized = true;
    interfaces->mdlCache->endLock();
    return initialized;
}

void animstatePoseParamCache::setValue(Entity* player, float value) noexcept
{
    if (!initialized)
    {
        init(player, name);
    }
    if (initialized && player)
    {
        interfaces->mdlCache->beginLock();
        player->setPoseParameter(value, index);
        interfaces->mdlCache->endLock();
    }
}

int32_t AnimState::selectSequenceFromActivityModifier(int activity) noexcept
{
    bool isPlayerDucked = animDuckAmount > 0.55f;
    bool isPlayerRunning = speedAsPortionOfWalkTopSpeed > 0.25f;

    int32_t layerSequence = -1;
    switch (activity)
    {
    case ACT_CSGO_JUMP:
        layerSequence = 15 + static_cast <int32_t>(isPlayerRunning);
        if (isPlayerDucked)
            layerSequence = 17 + static_cast <int32_t>(isPlayerRunning);
        break;
    case ACT_CSGO_ALIVE_LOOP:
        layerSequence = 8;
        if (weaponLast != weapon)
            layerSequence = 9;
        break;
    case ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING:
        layerSequence = 6;
        break;
    case ACT_CSGO_FALL:
        layerSequence = 14;
        break;
    case ACT_CSGO_IDLE_TURN_BALANCEADJUST:
        layerSequence = 4;
        break;
    case ACT_CSGO_LAND_LIGHT:
        layerSequence = 20;
        if (isPlayerRunning)
            layerSequence = 22;

        if (isPlayerDucked)
        {
            layerSequence = 21;
            if (isPlayerRunning)
                layerSequence = 19;
        }
        break;
    case ACT_CSGO_LAND_HEAVY:
        layerSequence = 23;
        if (isPlayerDucked)
            layerSequence = 24;
        break;

    case ACT_CSGO_CLIMB_LADDER:
        layerSequence = 13;
        break;
    default:
        break;
    }

    return layerSequence;
}

void AnimState::setLayerSequence(size_t layer, int32_t activity) noexcept
{
    if (activity == ACT_INVALID)
        return;

    auto entity = reinterpret_cast<Entity*>(player);

    auto hdr = entity->getModelPtr();
    if (!hdr)
        return;

    const auto sequence = selectSequenceFromActivityModifier(activity);

    if (sequence < 2)
        return;

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;

    l.playbackRate = entity->getLayerSequenceCycleRate(&l, sequence);
    l.sequence = sequence;
    l.cycle = l.weight = 0.f;
}

void AnimState::setLayerRate(size_t layer, float newRate) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;
    l.playbackRate = newRate;
}

void AnimState::setLayerCycle(size_t layer, float newCycle) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;
    l.cycle = Helpers::clampCycle(newCycle);
}

void AnimState::setLayerWeight(size_t layer, float weight) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;
    l.weight = std::clamp(weight, 0.f, 1.f);
}

void AnimState::setLayerWeightRate(size_t layer, float previous) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;

    float flNewRate = (l.weight - previous) / lastUpdateIncrement;
    l.weightDeltaRate = flNewRate;
}

void AnimState::incrementLayerCycle(size_t layer, bool allowLoop) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;

    if (fabsf(l.playbackRate) <= 0)
        return;

    float currentCycle = l.cycle;
    currentCycle += lastUpdateIncrement * l.playbackRate;

    if (!allowLoop && currentCycle >= 1)
    {
        currentCycle = 0.999f;
    }

    l.cycle = (Helpers::clampCycle(currentCycle));
}

void AnimState::incrementLayerCycleWeightRateGeneric(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

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
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return;

    if (abs(l.weightDeltaRate) <= 0)
        return;

    float currentWeight = l.weight;
    currentWeight += lastUpdateIncrement * l.weightDeltaRate;
    currentWeight = std::clamp(currentWeight, 0.f, 1.f);

    l.weight = currentWeight;
}

float AnimState::getLayerIdealWeightFromSeqCycle(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return 0.f;

    auto seqdesc = entity->getModelPtr()->seqdesc(l.sequence);

    float cycle = l.cycle;
    if (cycle >= 0.999f)
        cycle = 1.f;
    float easeIn = seqdesc.fadeInTime;
    float easeOut = seqdesc.fadeOutTime;
    float idealWeight = 1.f;

    if (easeIn > 0.f && cycle < easeIn)
    {
        idealWeight = Helpers::smoothStepBounds(0.0f, easeIn, cycle);
    }
    else if (easeOut < 1.f && cycle > easeOut)
    {
        idealWeight = Helpers::smoothStepBounds(1.0f, easeOut, cycle);
    }

    if (idealWeight < 0.0015f)
        return 0.f;

    return std::clamp(idealWeight, 0.f, 1.f);
}

bool AnimState::isLayerSequenceCompleted(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (&l)
        return (lastUpdateIncrement * l.playbackRate) + l.cycle >= 1.0f;
    return false;
}

float AnimState::getLayerCycle(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return 0.f;
    return l.cycle;
}

float AnimState::getLayerWeight(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return 0.f;
    return l.weight;
}

int AnimState::getLayerActivity(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    AnimationLayer* l = entity->getAnimationLayer(layer);
    if (l)
        return memory->getSequenceActivity(entity, l->sequence);
    return ACT_INVALID;
}

int AnimState::getLayerSequence(size_t layer) noexcept
{
    auto entity = reinterpret_cast<Entity*>(player);

    auto& l = *entity->getAnimationLayer(layer);
    if (!&l)
        return 0;
    return l.sequence;
}

int AnimState::getTicksFromCycle(float playbackRate, float cycle, float previousCycle) noexcept
{
    int ticks = 0;

    if (onLadder || playbackRate <= 0.0f)
        return ticks;

    float targetCycle = cycle;

    if (targetCycle != previousCycle)
    {
        float currentCycle = previousCycle;
        float timeDelta = lastUpdateIncrement != 0.0f ? lastUpdateIncrement : memory->globalVars->intervalPerTick;
        while (currentCycle < targetCycle)
        {
            currentCycle += playbackRate * timeDelta;
            ticks++;
        }
    }

    return ticks;
}

float AnimState::calculatePlaybackRate(Vector velocity) noexcept
{
    const auto entity = reinterpret_cast<Entity*>(player);

    const float lastUpdateIncrement = memory->globalVars->intervalPerTick;

    const Vector vecVelocity = Helpers::approach(velocity, this->vecVelocity, lastUpdateIncrement * 2000.f);
    const float velocityLengthXY = std::fminf(vecVelocity.length(), CS_PLAYER_SPEED_RUN);

    float walkToRunTransition = this->walkToRunTransition;
    float walkToRunTransitionState = this->walkToRunTransitionState;

    if (walkToRunTransition > 0.f && walkToRunTransition < 1.f)
    {
        if (walkToRunTransitionState == ANIM_TRANSITION_WALK_TO_RUN)
        {
            walkToRunTransition += lastUpdateIncrement * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
        }
        else
        {
            walkToRunTransition -= lastUpdateIncrement * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
        }
        walkToRunTransition = std::clamp(walkToRunTransition, 0.f, 1.f);
    }

    if (velocityLengthXY > (CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER) && walkToRunTransitionState == ANIM_TRANSITION_RUN_TO_WALK)
    {
        walkToRunTransitionState = ANIM_TRANSITION_WALK_TO_RUN;
        walkToRunTransition = std::fmaxf(0.01f, walkToRunTransition);
    }
    else if (velocityLengthXY < (CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER) && walkToRunTransitionState == ANIM_TRANSITION_WALK_TO_RUN)
    {
        walkToRunTransitionState = ANIM_TRANSITION_RUN_TO_WALK;
        walkToRunTransition = std::fmaxf(0.99f, walkToRunTransition);
    }

    char weaponMoveSequenceString[MAX_ANIMSTATE_ANIMNAME_CHARS];
    sprintf(weaponMoveSequenceString, "move_%s", memory->getWeaponPrefix(this));

    int weaponMoveSeq = entity->lookupSequence(weaponMoveSequenceString);
    if (weaponMoveSeq == -1)
        weaponMoveSeq = entity->lookupSequence("move");

    float moveCycleRate = 0.f;
    if (velocityLengthXY > 0.f)
    {
        moveCycleRate = entity->getSequenceCycleRate(weaponMoveSeq);
        float sequenceGroundSpeed = std::fmaxf(entity->getSequenceMoveDist(entity->getModelPtr(), weaponMoveSeq) / (1.0f / moveCycleRate), 0.001f);
        moveCycleRate *= velocityLengthXY / sequenceGroundSpeed;

        moveCycleRate *= Helpers::lerp(walkToRunTransition, 1.0f, CSGO_ANIM_RUN_ANIM_PLAYBACK_MULTIPLIER);
    }

    return moveCycleRate * lastUpdateIncrement;
}