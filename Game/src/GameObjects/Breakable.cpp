#include "precompiled.h"
#include "Breakable.h"
#include "AudioManager.h"
#include "Material.h"
#include "particleEmitterManager.h"
#include "CurrencyOrb.h"
#include "Debris.h"
#include "SaveManager.h"
#include "Game.h"
#include "NPCManager.h"
#include "Projectile.h"

Breakable::Breakable(float x, float y, DepthLayer depthLayer, float width, float height) :
	SolidMovingSprite(x,y,depthLayer,width, height),
	mHealth(10.0f),
	mMaxHealth(10.0f),
	mState(kNoDamage),
	mDeleteWhenBroken(false)
{
	mIsBreakable = true;
}

Breakable::~Breakable(void)
{
}

void Breakable::Update(float delta)
{
	// update the base classes
	SolidMovingSprite::Update(delta);
}

void Breakable::Initialise()
{
	// update the base classes
	SolidMovingSprite::Initialise();

	if (!Game::GetInstance()->GetIsLevelEditMode() )
	{
		std::vector<unsigned int> breakablesBroken;
		SaveManager::GetInstance()->GetBreakablesBroken(GameObjectManager::Instance()->GetCurrentLevelFile(), breakablesBroken);

		if (std::find(breakablesBroken.begin(), breakablesBroken.end(), ID()) != breakablesBroken.end())
		{
			mHealth = 0.0f;
			mState = kBroken;
			UpdateState();
		}
	}
}

void Breakable::XmlRead(TiXmlElement * element)
{
	// update base classes
	SolidMovingSprite::XmlRead(element);

	mMaxHealth = XmlUtilities::ReadAttributeAsFloat(element, "health", "value");
	mHealth = mMaxHealth;

	mDeleteWhenBroken = XmlUtilities::ReadAttributeAsBool(element, "health", "delete_on_break");
}

void Breakable::XmlWrite(TiXmlElement * element)
{
	SolidMovingSprite::XmlWrite(element);

	TiXmlElement * healthElem = new TiXmlElement("health");
	healthElem->SetDoubleAttribute("value", mMaxHealth);
	healthElem->SetAttribute("delete_on_break", mDeleteWhenBroken ? "true" : "false");
	element->LinkEndChild(healthElem);
}

bool Breakable::OnCollision(SolidMovingSprite * object)
{
	if (mState == kBroken)
	{
		return false;
	}

	if (!object->IsProjectile())
	{
		return false;
	}

	if (NPCManager::Instance()->IsAnyEnemyNPCInWorld())
	{
		return false;
	}

	/*
	Projectile * asProjectile = static_cast<Projectile *>(object);

	if (asProjectile->GetOwnerType() == Projectile::kNPCProjectile)
	{
		// enemy projectile don't affect breakables
		return false;
	}
	*/

	return SolidMovingSprite::OnCollision(object);
}

void Breakable::OnDamage(GameObject * damageDealer, float damageAmount, Vector2 pointOfContact, bool shouldExplode)
{
	if (mState == kBroken)
	{
		return;
	}

	SolidMovingSprite::OnDamage(damageDealer, damageAmount, pointOfContact, shouldExplode);

	if (mCanBeDamaged)
	{
		mHealth -= damageAmount;
	}

	UpdateState();

	auto cam = Camera2D::GetInstance();
	if (cam->IsObjectInView(this))
	{
		if (mState == kBroken)
		{
			cam->DoMediumShake();
		}
		else
		{
			cam->DoSmallShake();
		}
	}
}

void Breakable::UpdateState()
{
	BreakableState previousState = mState;

	if (mMaxHealth == 0.0f)
	{
		return;
	}

	float percentHealthy = mHealth / mMaxHealth;
	if (percentHealthy == 1.0f)
	{
		mState = kNoDamage;
	}
	else if (percentHealthy > 0.8f)
	{
		mState = kSlightDamage;
	}
	else if (percentHealthy > 0.4f)
	{
		mState = kDamaged;
	}
	else if (percentHealthy > .0f)
	{
		mState = kHeavyDamaged;
	}
	else if (mState != kBroken)
	{
		GameObjectManager::Instance()->SetBreakableBroken(ID());

		mState = kBroken;
		CurrencyOrb::SpawnOrbs(m_position, (rand() % 8) + 1);
		AudioManager::Instance()->PlaySoundEffect(m_material->GetRandomDestroyedSound());

		if (mDeleteWhenBroken)
		{
			GameObjectManager::Instance()->RemoveGameObject(this, true);
		}
	}

	if (previousState != mState &&
		mState == kBroken)
	{
		SpawnDamageTransitionParticles();
	}
}

void Breakable::UpdateAnimations()
{
	std::string currentSequence = "Still";

	switch (mState)
	{
		case kSlightDamage:
		{
			currentSequence = "Damage1";
			break;
		}
		case kDamaged:
		{
			currentSequence = "Damage2";
			break;
		}
		case kHeavyDamaged:
		{
			currentSequence = "Damage3";
			break;
		}
		case kBroken:
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

void Breakable::SpawnDamageTransitionParticles()
{
	if (!m_material)
	{
		return;
	}

	// TODO: this is SLOW
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

	// A massive hack...
	if (m_material->GetMaterialName() == "crate")
	{
		size = Vector2(160.0f * scaleX, 443.0f * scaleY);
	}
	else if (m_material->GetMaterialName() == "pot")
	{
		// pot
		size = Vector2(310.0f * scaleX, 284.0f * scaleY);
	}
	else
	{
		// lantern
		size = Vector2(170.0f * scaleX * 1.1f, 150.0f * scaleY * 1.1f);
	}

	for (const auto & d : debrisTextures)
	{
		Debris * debris = new Debris(nullptr, 
									Vector2(m_position.X, m_position.Y + 50.0f),
									GetDepthLayer(),
									size,
									Vector2(30.0f, 30.0f),
									d.c_str(),
									false,
									1.0f);

		debris->SetMaterial(m_material);

		GameObjectManager::Instance()->AddGameObject(debris);
	}
} 