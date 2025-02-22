#include "precompiled.h"
#include "BombProjectile.h"
#include "Explosion.h"
#include "ParticleEmitterManager.h"
#include "NPC.h"
#include "Debris.h"
#include "AudioManager.h"
#include "waterblock.h"
#include "SolidLineStrip.h"
#include "Material.h"

static const float kParticleDelay = 0.1f;

BombProjectile::BombProjectile(ProjectileOwnerType ownerType, 
								const char* textureFileName, 
								const char * impactTextureFilename,
								Vector2 position, 
								Vector2 dimensions,
								Vector2 collisionDimensions,
								Vector2 direction,
								float damage,
								float speed,
								int maxTimeInActive):
Projectile(ownerType,
			textureFileName, 
			impactTextureFilename,
			position,
			GameObject::kBombProjectile,
			dimensions,
			collisionDimensions,
			 direction, 
			 damage,
			 speed,
			 (float)maxTimeInActive),
			 mTimeUntilNextParticleSpray(0.0f)
{
	mIsBombProjectile = true;

	mSpinningMovement = true;
	mBouncable = true;
	mBounceDampening = 0.7f;

	mType = kBombProjectile;

	mPositionalAudioEnabled = true;
	mPositionalAudio.SetRepeat(true);
	mPositionalAudio.SetAudioFilename("fuse_burning.wav");
	mPositionalAudio.SetDimensions(Vector2(600.0f, 600.0f));
	mPositionalAudio.SetFadeDimensions(Vector2(3000.0f, 3000.0f));

	m_maxTimeInActive = 5.0f;

	m_alpha = 1.0f;
}

BombProjectile::~BombProjectile(void)
{
	Explosion * explosion = new Explosion(m_damage, 1000.0f, m_position.X, m_position.Y, GetDepthLayer());

	GameObjectManager::Instance()->AddGameObject(explosion);

	if (Camera2D::GetInstance()->IsObjectInView(this))
	{
		Camera2D::GetInstance()->DoBigShake();
	}
}

bool BombProjectile::OnCollision(SolidMovingSprite* object)
{
	if (WasInWaterLastFrame() || mIsInWater)
	{
		return false;
	}

	if (object->IsDebris() || object->IsCurrencyOrb())
	{
		return false;
	}

	if (object->IsWaterBlock())
	{
		return false;
	}

	if (object->IsCharacter())
	{
		return false;
	}

	if (object->IsBreakable())
	{
		return false;
	}

	if (object->IsPickup())
	{
		return false;
	}

	if (object->IsBombProjectile())
	{
		return false;
	}

	if (object->IsProjectile())
	{
		Projectile* projectile = static_cast<Projectile*>(object);

		auto t = projectile->GetProjectileType();
		if (t == Projectile::kBladeProjectile)
		{
			// explode if hit by a blade
			GameObjectManager::Instance()->RemoveGameObject(this);
		}
	}

	if (object->IsFoliage())
	{
		return false;
	}

	if (object->IsSolidLineStrip())
	{
		SolidLineStrip * lineStrip = static_cast<SolidLineStrip*>(object);
		HandleSolidLineStripCollision(lineStrip);
		return false;
	}

	if (m_isActive)
	{
		m_isActive = false;
		m_applyGravity = false;
		SetVelocityXY(0.0f, 0.0f);
		m_timeBecameInactive = Timing::Instance()->GetTotalTimeSeconds();
	}

	return SolidMovingSprite::OnCollision(object);
}

void BombProjectile::Update(float delta)
{
	float targetDelta =  (float)Timing::Instance()->GetTargetDelta();
	float percentDelta = delta / targetDelta;

	// apply gravity to the 
	if (m_applyGravity && !mIsOnSolidLine)
	{
		if (!mIsInWater)
		{
			m_velocity.Y -= 0.3f * percentDelta;
		}
		else
		{
			m_velocity.Y -= 0.06f * percentDelta;
		}
	}

	// nice simple update
	if (mIsInWater)
	{
		m_velocity.X *= 0.92f; // slow down significantly
		//m_velocity.Y *= 0.9f; // slow down significantly
	}

	m_direction = m_velocity;
	m_direction.Normalise();

	m_position += m_velocity * percentDelta;

	// we dont need complicated movement so we'll ignore the MovingSprite class
	Sprite::Update(delta);

	if (IsOnSolidSurface())
	{
		// StopYAccelerating();

		if (!mIsInWater)
		{
			SetRotationAngle(GetRotationAngle() + ((m_velocity.X * -0.04f) *percentDelta));
		}
		else
		{
			SetRotationAngle(GetRotationAngle() + ((m_velocity.X * -0.009f) *percentDelta));
		}
	}
	else
	{
		if (!mIsInWater)
		{
			SetRotationAngle(GetRotationAngle() + ((m_velocity.X * -0.01f) * percentDelta));
		}
		else
		{
			SetRotationAngle(GetRotationAngle() + ((m_velocity.X * -0.005f) * percentDelta));
		}
	}

	if (!m_isActive)
	{
		float currentTime = Timing::Instance()->GetTotalTimeSeconds();
		float timeToDie = m_timeBecameInactive + m_maxTimeInActive;

		if(currentTime > timeToDie)
		{
			// time to kill ourselves
			GameObjectManager::Instance()->RemoveGameObject(this);
		}
	}

	if (mTimeUntilNextParticleSpray > 0.0f)
	{
		mTimeUntilNextParticleSpray -= delta;

		if (mTimeUntilNextParticleSpray < 0.0f)
		{
			mTimeUntilNextParticleSpray = 0.0f;
		}
	}
}

void BombProjectile::HandleSolidLineStripCollision(SolidLineStrip * solidLineStrip)
{
	Vector2 collisionPosition;

	if (solidLineStrip->GetBombProjectileCollisionData(this, collisionPosition))
	{
		m_applyGravity = false;
		SetVelocityXY(0.0f, 0.0f);

		if (m_isActive)
		{
			m_timeBecameInactive = Timing::Instance()->GetTotalTimeSeconds();
		}

		Material * objectMaterial = solidLineStrip->GetMaterial();
		if (objectMaterial != nullptr)
		{
			// where should the particles spray from
			Vector2 particlePos = solidLineStrip->Position() - collisionPosition;

			// show particles
			bool loop = false;
			float minLive = 0.35f;
			float maxLive = 0.75f;

			if (m_isActive)
			{
				// play sound for non-characters, characters handle their sounds in OnDamage
				string soundFile = objectMaterial->GetRandomDamageSoundFilename();
				AudioManager::Instance()->PlaySoundEffect(soundFile);
			}
			
			if (m_velocity.X > 5.0f && mTimeUntilNextParticleSpray == 0.0f)
			{
				string particleTexFile = objectMaterial->GetRandomParticleTexture();
				ParticleEmitterManager::Instance()->CreateDirectedSpray(5,
																		particlePos,
																		GetDepthLayer(),
																		Vector2(-m_direction.X, -m_direction.Y),
																		0.4f,
																		Vector2(3200.0f, 1200.0f),
																		particleTexFile,
																		1.0f,
																		4.0f,
																		minLive,
																		maxLive,
																		10.0f,
																		30.0f,
																		0.7f,
																		loop,
																		0.7f,
																		1.0f,
																		10.0f,
																		true,
																		3.5f,
																		1.5f,
																		3.0f,
																		0.15f,
																		0.6f);

				mTimeUntilNextParticleSpray = kParticleDelay;
			}
			
		}

		m_isActive = false;
	}
}
