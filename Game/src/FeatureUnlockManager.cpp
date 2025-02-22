#include "precompiled.h"
#include "FeatureUnlockManager.h"
#include "SaveManager.h"
#include <Achievements\Achievement.h>

extern CSteamAchievements* g_SteamAchievements;

FeatureUnlockManager * FeatureUnlockManager::m_instance = nullptr;

FeatureUnlockManager * FeatureUnlockManager::GetInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new FeatureUnlockManager();
	}

	return m_instance;
}
FeatureUnlockManager::FeatureUnlockManager(void)
{
}

FeatureUnlockManager::~FeatureUnlockManager(void)
{
}

void FeatureUnlockManager::Initialise()
{
}

bool FeatureUnlockManager::IsFeatureUnlocked(const FeatureType type)
{
#ifdef _DEBUG
	// testing
	// return true;
#endif

	return SaveManager::GetInstance()->IsGameFeatureUnlocked(type);
}

void FeatureUnlockManager::SetFeatureUnlocked(const FeatureType type)
{
	if (type == kProjectileDamageIncrease)
	{
		Player* player = GameObjectManager::Instance()->GetPlayer();
		if (player)
		{
			player->SetUpgradedKnifeTexture();
		}
	}
	else if (type == kSpeedIncrease)
	{
		Player* player = GameObjectManager::Instance()->GetPlayer();
		if (player)
		{
			player->OnPurchaseSpeedUpgrade();
		}
	}
	else if (type == kSlowMotion)
	{
		if (g_SteamAchievements)
			g_SteamAchievements->SetAchievement("ACH_FOCUS_UNLOCK");
	}
	else if (type == kRoll)
	{
		if (g_SteamAchievements)
			g_SteamAchievements->SetAchievement("ACH_ROLL_UNLOCK");
	}

	SaveManager::GetInstance()->SetGameFeatureUnlocked(type);
}

FeatureUnlockManager::FeatureType FeatureUnlockManager::GetFeatureTypeFromString(const string & asString)
{
	if (asString == "kDownwardDash")
	{
		return kDownwardDash;
	}
	else if (asString == "kRoll")
	{
		return kRoll;
	}
	else if (asString == "kSlowMotion")
	{
		return kSlowMotion;
	}
	else if (asString == "kDeflection")
	{
		return kDeflection;
	}
	else if (asString == "kSpeedIncrease")
	{
		return kSpeedIncrease;
	}
	else if (asString == "kProjectileDamageIncrease")
	{
		return kProjectileDamageIncrease;
	}
	else if (asString == "kProjectileSpeedIncrease")
	{
		return kProjectileSpeedIncrease;
	}
	else if (asString == "kBombUnlock")
	{
		return kBombUnlock;
	}
	
	GAME_ASSERT(false);
	return kNone;
}

string FeatureUnlockManager::GetFeatureAsString(FeatureType featureType)
{
	switch (featureType)
	{
		case kDownwardDash:
		{
			return "kDownwardDash";
		}
		case kRoll:
		{
			return "kRoll";
		}
		case kSlowMotion:
		{
			return "kSlowMotion";
		}
		case kDeflection:
		{
			return "kDeflection";
		}
		case kSpeedIncrease:
		{
			return "kSpeedIncrease";
		}
		case kProjectileDamageIncrease:
		{
			return "kProjectileDamageIncrease";
		}
		case kProjectileSpeedIncrease:
		{
			return "kProjectileSpeedIncrease";
		}
		case kBombUnlock:
		{
			return "kBombUnlock";
		}
	}

	return "kNone";
}

int FeatureUnlockManager::GetFeatureCost(const FeatureType featureType)
{
	switch (featureType)
	{
		case kDownwardDash:
		{
			return 450;
		}
		case kRoll:
		{
			return 25;
		}
		case kSlowMotion:
		{
			return 600;
		}
		case kDeflection:
		{
#if _DEBUG
			// testing
			return 2;
#endif
			return 375;
		}
		case kSpeedIncrease:
		{
			return 200;
		}
		case kProjectileDamageIncrease:
		{
			return 225;
		}
		case kProjectileSpeedIncrease:
		{
			return 200;
		}
		case kBombUnlock:
		{
#if _DEBUG
			// testing
			return 2;
#endif
			return 1000;
		}
		default: return 0;
	}
}