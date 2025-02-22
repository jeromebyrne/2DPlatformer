#ifndef AISTATERANGEATTACK_H
#define AISTATERANGEATTACK_H

#include "AIState.h"

class AIStateRangeAttack : public AIState
{
public:

	AIStateRangeAttack(NPC * npc);
	virtual ~AIStateRangeAttack(void);

	virtual void OnTransition() override;
	virtual void Update(float delta) override;

	void SetDesiredRange(float value) { mDesiredRange = value; }

private:

	bool CanAccelerateX(float direction);

	void TeleportOnFrontOfPlayer();

	float mDesiredRange;
	float mFollowRange;
	float mRandOffset;
	float mLastTimeRanAway;
	float mTimeUntilRandomlyJumpOrRoll;
	float mTimeUntilCanTeleport;
	float mTimeStoodStill;

	bool mWillFireAtTarget = false;
	float mWillFireAtTargetCountdown = 0.0f;
};

#endif