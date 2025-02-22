#include "precompiled.h"
#include "Smashable.h"
#include "AudioManager.h"
#include "Material.h"
#include "particleEmitterManager.h"
#include "CurrencyOrb.h"
#include "Debris.h"
#include "SaveManager.h"
#include "Game.h"

extern CSteamAchievements* g_SteamAchievements;

Smashable::Smashable(float x, float y, DepthLayer depthLayer, float width, float height) :
	SolidMovingSprite(x,y, depthLayer, width, height),
	mState(kNotSmashed)
{
	mCanBeDamaged = false;
	mIsSmashable = true;
}

Smashable::~Smashable(void)
{
}

void Smashable::Update(float delta)
{
	// update the base classes
	SolidMovingSprite::Update(delta);
}

void Smashable::Initialise()
{
	// update the base classes
	SolidMovingSprite::Initialise();

	if (!Game::GetInstance()->GetIsLevelEditMode() )
	{
		bool broken = SaveManager::GetInstance()->IsSmashableBroken(GameObjectManager::Instance()->GetCurrentLevelFile());

		if (broken)
		{
			mState = kSmashed;
		}
	}
}

void Smashable::XmlRead(TiXmlElement * element)
{
	// update base classes
	SolidMovingSprite::XmlRead(element);
}

void Smashable::XmlWrite(TiXmlElement * element)
{
	SolidMovingSprite::XmlWrite(element);
}

bool Smashable::OnCollision(SolidMovingSprite * object)
{
	if (mState == kSmashed)
	{
		return false;
	}

	auto player = GameObjectManager::Instance()->GetPlayer();

	if (player && player == object && player->GetIsDownwardDashing())
	{
		// we are downward dashing, this is the only thing that can break a smashable
		// just make sure we are on top of the object
		if (player->CollisionBottom() <= CollisionTop() &&
			player->CollisionBottom() > CollisionCentreY())
		{
			mState = kSmashed;
			SaveManager::GetInstance()->SetSmashableBroken(GameObjectManager::Instance()->GetCurrentLevelFile());
			CurrencyOrb::SpawnOrbs(m_position, (rand() % 8) + 1);
			if (m_material)
			{
				AudioManager::Instance()->PlaySoundEffect(m_material->GetRandomDestroyedSound());
			}
			SpawnSmashedParticles();

			Camera2D::GetInstance()->DoBigShake();

			Game::GetInstance()->Vibrate(0.75f, 1.0f, 0.4f);

			if (g_SteamAchievements)
				g_SteamAchievements->SetAchievement("ACH_STONE_SMASH");
		}
	}

	return SolidMovingSprite::OnCollision(object);
}

void Smashable::OnDamage(GameObject * damageDealer, float damageAmount, Vector2 pointOfContact, bool shouldExplode)
{
	if (mState == kSmashed)
	{
		return;
	}

	SolidMovingSprite::OnDamage(damageDealer, damageAmount, pointOfContact, shouldExplode);
}

void Smashable::UpdateAnimations()
{
	std::string currentSequence = "Still";

	switch (mState)
	{
		case kSmashed:
		{
			currentSequence = "Damage4";
			break;
		}
		default:
		{
			break;
		}
	}

	AnimationPart * bodyPart = m_animation->GetPart("body");
	GAME_ASSERT(bodyPart);

	if (bodyPart)
	{
		std::string sequence_name = bodyPart->CurrentSequence()->Name();
		if (sequence_name != currentSequence)
		{
			bodyPart->SetSequence(currentSequence);
		}

		bodyPart->AnimateLooped();

		m_texture = bodyPart->CurrentFrame(); // set the current texture
	}
}

void Smashable::SpawnSmashedParticles()
{
	if (!m_material)
	{
		return;
	}

	Vector2 nativeDimensions = GetTextureDimensions();

	float scaleX = m_dimensions.X / nativeDimensions.X;
	float scaleY = m_dimensions.Y / nativeDimensions.Y;

	ParticleEmitterManager::Instance()->CreateDirectedSpray(5,
																Vector2(m_position.X, m_position.Y),
																GetDepthLayer(),
																Vector2(0.0f, 1.0f),
																0.7f,
																Vector2(3200.0f, 1200.0f),
																m_material->GetRandomParticleTexture(),
																1.0f,
																7.0f,
																0.7f,
																1.50f,
																180.0f,
																180.0f,
																0.0f,
																false,
																0.7f,
																1.0f,
																10000.0f,
																true,
																2.0f,
																m_dimensions.X * 0.03f,
																m_dimensions.Y * 0.1f,
																0.1f,
																0.1f);

	const auto & debrisTextures = m_material->GetDebrisTextures();

	Vector2 size;
	bool isNativeDimensions = false;

	// A massive hack...
	if (m_material->GetMaterialName() == "crate")
	{
		size = Vector2(160.0f * scaleX, 443.0f * scaleY);
	}
	else if (m_material->GetMaterialName() == "pot")
	{
		size = Vector2(310.0f * scaleX, 284.0f * scaleY);
	}
	else
	{
		isNativeDimensions = true;
	}

	for (const auto & d : debrisTextures)
	{
		Debris * debris = new Debris(nullptr, 
									Vector2(m_position.X, m_position.Y + 50.0f),
									GetDepthLayer(),
									size,
									Vector2(40.0f, 40.0f),
									d.c_str(),
									isNativeDimensions,
									1.0f);

		debris->SetMaterial(m_material);

		GameObjectManager::Instance()->AddGameObject(debris);
	}
} 