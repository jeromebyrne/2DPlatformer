#include "precompiled.h"
#include "AmbientBird.h"
#include "ParticleEmitterManager.h"
#include "AudioManager.h"
#include "material.h"
#include "Debris.h"
#include "AIStateFollow.h"
#include "AIStateRepel.h"
#include "Game.h"

AmbientBird::AmbientBird(void) :
	NPC(),
	mIsGliding(false),
	mTimeUntilFlap(0.0f),
	mTimeUntilEndFlap(0.0f),
	mTravelOffset(1000.0f)
{
	mAlwaysUpdate = true;
	mIsButterfly = true;
	mHealth = 1.0f; // 1 hit kills for rabbits
	mMatchAnimFrameRateWithMovement = false;
	mAddHealthBar = false;
}


AmbientBird::~AmbientBird(void)
{
}

void AmbientBird::OnDamage(GameObject * damageDealer, float damageAmount, Vector2 pointOfContact, bool shouldExplode)
{
}

void AmbientBird::Initialise()
{
	NPC::Initialise();

	mCheckNPCOverlapCollisions = false;

	m_applyGravity = false;

	m_passive = true;

	SetState(AIState::kNone); // let's just handle our own state

	m_direction.X = mTravelOffset;
	m_direction.Normalise();

	mStartPosition = m_position;
}

void AmbientBird::UpdateAnimations()
{
	AnimationPart * bodyPart = m_animation->GetPart("body");
	GAME_ASSERT(bodyPart);

	if (bodyPart != nullptr)
	{
		if (mIsGliding)
		{
			if (bodyPart->CurrentSequence()->Name() != "Still")
			{
				bodyPart->SetSequence("Still");

				bodyPart->CurrentSequence()->SetFrameRate(1);
			}
		}
		else
		{
			if (bodyPart->CurrentSequence()->Name() != "Running")
			{
				bodyPart->SetSequence("Running");

				bodyPart->CurrentSequence()->SetFrameRate(9.0f);
			}
		}
		
		bodyPart->AnimateLooped();

		m_texture = bodyPart->CurrentFrame(); // set the current texture
	}

	m_mainBodyTexture = m_texture;
}

bool AmbientBird::OnCollision(SolidMovingSprite * object)
{
	/*
	if (m_position.Z > 99)
	{
		// hack to stop butterfly's in the background being affected
		return false;
	}

	return NPC::OnCollision(object);
	*/

	return false;
}

void AmbientBird::Update(float delta)
{
	NPC::Update(delta);

	if (Game::GetIsLevelEditMode())
	{
		return;
	}

	AccelerateX(m_direction.X);

	if (m_direction.X > 0)
	{
		if (m_position.X > (mStartPosition.X + mTravelOffset))
		{
			float diffX = Camera2D::GetInstance()->X() - mStartPosition.X;
			float startParallaxOffsetX = (diffX * mParallaxMultiplier.X) - diffX;

			float diffY = Camera2D::GetInstance()->Y() - mStartPosition.Y;
			float startParallaxOffsetY = (diffY * mParallaxMultiplier.Y) - diffY;

			if (!Camera2D::GetInstance()->IsObjectInView(this) &&
				!Camera2D::GetInstance()->IsWorldPosInView(mStartPosition, startParallaxOffsetX, startParallaxOffsetY))
			{
				Player * player = GameObjectManager::Instance()->GetPlayer();
				if (player)
				{
					float distX = player->X() - m_position.X;

					if (std::abs(distX) > 1000.0f)
					{
						m_position = mStartPosition;
					}
				}
			}
		}
	}
	else
	{
		if (m_position.X < (mStartPosition.X + mTravelOffset))
		{
			float diffX = Camera2D::GetInstance()->X() - mStartPosition.X;
			float startParallaxOffsetX = (diffX * mParallaxMultiplier.X) - diffX;

			float diffY = Camera2D::GetInstance()->Y() - mStartPosition.Y;
			float startParallaxOffsetY = (diffY * mParallaxMultiplier.Y) - diffY;

			if (!Camera2D::GetInstance()->IsObjectInView(this) &&
				!Camera2D::GetInstance()->IsWorldPosInView(mStartPosition, startParallaxOffsetX, startParallaxOffsetY))
			{
				Player * player = GameObjectManager::Instance()->GetPlayer();
				if (player)
				{
					float distX = player->X() - m_position.X;

					if (std::abs(distX) > 1000.0f)
					{
						m_position = mStartPosition;
					}
				}
			}
		}
	}

	if (mIsGliding)
	{
		mTimeUntilFlap -= delta;

		if (mTimeUntilFlap <= 0.0f)
		{
			mTimeUntilFlap = (rand() % (unsigned)((mFlapWingsMaxDelay - mFlapWingsMinDelay) * 100)) + (mFlapWingsMinDelay * 100);
			mTimeUntilFlap *= 0.01f;

			mIsGliding = false;
		}
	}
	else
	{
		mTimeUntilEndFlap -= delta;
		if (mTimeUntilEndFlap <= 0.0f)
		{
			mTimeUntilEndFlap = (rand() % (unsigned)((mFlapTimeMaxDelay - mFlapTimeMinDelay) * 100)) + (mFlapTimeMinDelay * 100);
			mTimeUntilEndFlap *= 0.01f;

			mIsGliding = true;
		}
	}
}

void AmbientBird::XmlRead(TiXmlElement * element)
{
	NPC::XmlRead(element);

	mTravelOffset = XmlUtilities::ReadAttributeAsFloat(element, "travel_offset", "value");
}

void AmbientBird::XmlWrite(TiXmlElement * element)
{
	NPC::XmlWrite(element);

	TiXmlElement * travelOffset = new TiXmlElement("travel_offset");
	travelOffset->SetAttribute("value", Utilities::ConvertDoubleToString(mTravelOffset).c_str());
	element->LinkEndChild(travelOffset);
}