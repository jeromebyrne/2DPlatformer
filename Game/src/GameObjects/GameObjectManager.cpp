#include "precompiled.h"
#include "GameObjectManager.h"
#include "sprite.h"
#include "movingsprite.h"
#include "player.h"
#include "parallaxlayer.h"
#include "ParticleSpray.h"
#include "collisionmanager.h"
#include "Projectile.h"
#include "AudioManager.h"
#include "dxwindow.h"
#include "ParticleEmitterManager.h"
#include "UIManager.h"
#include "npc.h"
#include "platform.h"
#include "pathingplatform.h"
#include "fallingplatform.h"
#include "gamepad.h"
#include "NinjaSpawner.h"
#include "NPCManager.h"
#include "WeatherManager.h"
#include "AudioObject.h"
#include "Rabbit.h"
#include "Game.h"
#include "Debris.h"
#include "LevelTrigger.h"
#include "WaterBlock.h"
#include "butterfly.h"
#include "scrollingsprite.h"
#include "SolidLineStrip.h"
#include "NPCTrigger.h"
#include "ForceBox.h"
#include "TextObject.h"
#include "AmbientBird.h"
#include "CurrencyOrb.h"
#include "Breakable.h"
#include "SaveManager.h"
#include "ActiveBird.h"
#include "Smashable.h"
#include "SaveShrine.h"
#include "Door.h"
#include "GhostEnemy.h"
#include "HealthDevil.h"
#include "HealthUpgradePickup.h"
#include "FocusUpgradePickup.h"
#include "HealthIncreasePickup.h"
#include "DojoScrollPickup.h"
#include "LevelEntry.h"
#include "UIGameLoading.h"
#include "Foliage.h"
#include "SkeletonEnemy.h"
#include "SkeletonEnemySpawner.h"
#include "PaperPickup.h"
#include "KeyPickup.h"
#include "SfxPickup.h"
#include "Boat.h"

struct DepthSortPredicate
{
	bool operator() (const shared_ptr<GameObject> & lhs, const shared_ptr<GameObject> & rhs) 
	{
		return lhs->GetDepthLayer() > rhs->GetDepthLayer();
	}
};

GameObjectManager* GameObjectManager::m_instance = 0;

GameObjectManager::GameObjectManager(): 
	m_camera(),
	m_player(nullptr),
	m_levelLoaded(false),
	mSwitchToLevel(false),
	mSlowMotionLayer(nullptr),
	mFreshLevelLaunch(true)
{
}

GameObjectManager::~GameObjectManager(void)
{
}

void GameObjectManager::RemoveGameObject(GameObject * object, bool defer)
{
	if (!object)
	{
		GAME_ASSERT(object);
		return;
	}

#if _DEBUG
	Game::GetInstance()->ResetLevelEditorSelectedObject();

	if (object->IsDebris())
	{
		bool debug = true;
	}
#endif
	
	if (defer)
	{
		for (auto obj : m_killList)
		{
			if (obj->ID() == object->ID())
			{
				// already on the kill list 
				LOG_ERROR("Attempting to remove GameObject with ID: %u when already scheduled for removal", object->ID());
				GAME_ASSERT(false);
				return;
			}
		}

		m_killList.push_back(object);
	}
	else
	{
		object->Detach(); // always detach from anything first

		auto iter = m_gameObjects.begin();
		for (auto & obj : m_gameObjects)
		{
			if (obj.get() == object)
			{
				break;
			}
			++iter;
		}

		if (iter != m_gameObjects.end())
		{
#if _DEBUG

			// the ref count should only be 1 when removing an object from the master list
			long refCount = (*iter).use_count();
			if (refCount > 1)
			{
				LOG_ERROR("POSSIBLE MEMORY LEAK!!! GameObject with ID: %u is being removed but still has a ref count greater than 1.", (*iter)->ID());
				GAME_ASSERT(false);
			}
#endif
			m_gameObjects.remove(*iter);
		}
	}
}

shared_ptr<GameObject> & GameObjectManager::GetObjectByID(int id)
{
	for (auto & obj : m_gameObjects)
	{
		if(obj->ID() == id)
		{
			return obj;
		}
	}

	LOG_INFO("Did not find object with id: %i", id);
	GAME_ASSERT(false);
	return shared_ptr<GameObject>(nullptr); // JB: this is bad
}

// this is to be called before initialise
void GameObjectManager::LoadContent(ID3D10Device * device)
{
	for(auto & obj : m_gameObjects)
	{
		if (!obj)
		{
			GAME_ASSERT(obj);
			continue;
		}

		if (!obj->IsDrawable())
		{
			continue;
		}

		GAME_ASSERT(dynamic_cast<DrawableObject*>(obj.get()));
		DrawableObject * drawObj = static_cast<DrawableObject*>(obj.get());

		drawObj->LoadContent(device);
	}
}

void GameObjectManager::Initialise()
{
	for(auto &obj : m_gameObjects) 
	{
		GAME_ASSERT(obj);
		if (obj)
		{
			obj->Initialise();
		}
	}

	GAME_ASSERT(m_player);
 	if (m_player)
	{
		// set the camera to the players position initially
		m_camera->FollowObjectsOrigin(m_player);

		// now tell the camera that the player is the object we should follow
		m_camera->SetTargetObject(m_player);
	}
}

void GameObjectManager::Update(bool paused, float delta)
{
	mMusicManager.Update(delta); // TODO: delta should not have time modifier

	if (!paused)
	{
		float camX = m_camera->X();
		float camY = m_camera->Y();

		NPCManager::Instance()->Update();

		// update the weather
		WeatherManager::GetInstance()->Update(delta);

		mPostUpdateObjects.clear();

		for(auto & obj : m_gameObjects) 
		{
			if (!obj)
			{
				GAME_ASSERT(obj);
				continue;
			}

			if (!obj->IsUpdateable())
			{
				continue;
			}

			if (obj->AlwaysUpdate())
			{
				obj->Update(delta); 
				mPostUpdateObjects.push_back(obj.get());
			}
			else 
			{
				bool inView = 
					Utilities::IsObjectInRectangle(obj.get(), camX, camY, m_updateZoneDimensions.X, m_updateZoneDimensions.Y);

				if (inView)
				{
					obj->Update(delta);
					mPostUpdateObjects.push_back(obj.get());
				}
				else if (obj->IsProjectile() || obj->IsDebris())
				{
					// if a projectile has gone outside the bounds then just remove it
					RemoveGameObject(obj.get());
				}

			}
		}
	} // end of if paused
}

void GameObjectManager::PostUpdate(bool paused, float delta)
{
	if (paused)
	{
		return;
	}

	for (GameObject * obj : mPostUpdateObjects)
	{
		obj->PostUpdate(delta);
	}

	mPostUpdateObjects.clear();

	// kill any objects in the kill list
	for (auto obj : m_killList)
	{
		RemoveGameObject(obj, false);
	}

	m_killList.clear(); // remove our objects

	if (mSwitchToLevel)
	{
		SwitchToLevel(mLevelToSwitch, mDoorIdCameFrom, false);
		mLevelToSwitch = "";
		mSwitchToLevel = false;
	}
}

void GameObjectManager::ScaleObjects(float xScale, float yScale)
{
	for (auto &obj : m_gameObjects)
	{
		GAME_ASSERT(obj);
		if (obj)
		{
			obj->Scale(xScale, yScale);
		}
	}
}

void GameObjectManager::Draw(ID3D10Device *  device)
{
	for (auto & obj : m_gameObjects)
	{
		GAME_ASSERT(obj);
		if (!obj)
		{
			continue;
		}

		if (!obj->IsDrawable())
		{
			continue;
		}

		GAME_ASSERT(dynamic_cast<DrawableObject*>(obj.get()));
		DrawableObject * drawObj = static_cast<DrawableObject*>(obj.get());

		// only draw if object is in view
		if (drawObj->Alpha() > 0.0f && (m_camera->IsObjectInView(drawObj) || drawObj->IsPlayer()))
		{
			// apply any changes needed
			if (drawObj->IsChangeRequired())
			{
				// TODO: potential profiling 
				drawObj->ApplyChange(device);
			}
			drawObj->Draw(device, m_camera);
		}
	}

	if (mSlowMotionLayer != nullptr)
	{
		if (Timing::Instance()->GetTimeModifier() < 1.0f)
		{
			float alpha = mSlowMotionLayer->Alpha();
			if (alpha < 1.0f)
			{
				alpha += 0.1f;
			}
			mSlowMotionLayer->SetAlpha(alpha);
		}
		else if (mSlowMotionLayer)
		{
			float alpha = mSlowMotionLayer->Alpha();
			if (alpha > 0.0f)
			{
				alpha -= 0.1f;
			}
			mSlowMotionLayer->SetAlpha(alpha);
		}
	}

	NPCManager::Instance()->Draw();
}

void GameObjectManager::DebugDraw()
{
	for (auto & obj : m_gameObjects)
	{
		GAME_ASSERT(obj);
		if (!obj)
		{
			continue;
		}

		// only draw if we are in view
		if (m_camera->IsObjectInView(obj.get()))
		{
			obj.get()->DebugDraw(Graphics::GetInstance()->Device());
		}
	}
}

// load game objects via xml file
void GameObjectManager::LoadObjectsFromFile(const string & filename)
{
	mCurrentLevelFile = filename;

	GameObject::ResetGameIds();

#if _DEBUG
	if (Game::GetIsLevelEditMode())
	{
		Game::GetInstance()->SetLevelEditFilename(filename);
	}
#endif

	// play opening stinger
	AudioManager::Instance()->StopAllSounds();

	Graphics * graphics = Graphics::GetInstance();
	Camera2D * camera = Camera2D::GetInstance();

	// space in which include objects for Updates() and collision detection
	m_updateZoneDimensions = Vector2(4000.0f, 4000.0f);

	CollisionManager::Instance()->SetCollisionArea((int)m_updateZoneDimensions.X, (int)m_updateZoneDimensions.Y);

	m_camera = camera;

	XmlDocument doc;
	doc.Load(filename);

	TiXmlHandle * hdoc = doc.Handle();
	TiXmlElement * root = hdoc->FirstChildElement().Element();

	TiXmlElement * child = root->FirstChildElement();

	// get the orbs that have already been created so that we don't create them
	std::vector<unsigned int> currencyOrbsCollected;
	SaveManager::GetInstance()->GetCurrencyOrbsCollected(mCurrentLevelFile, currencyOrbsCollected);

	// loop through our game objects
	unsigned int objLoadCount = 0;
	while(child)
	{
		GameObject * object = CreateObject(child, currencyOrbsCollected);
		if (object)
		{
			m_gameObjects.push_back(shared_ptr<GameObject>(object));
		}
		
		child = child->NextSiblingElement();

		++objLoadCount;
	}

	ParseLevelProperties(root);

	mMusicManager = MusicManager();
	mMusicManager.Initialise(mLevelProperties.GetLevelMusic(), 
							 mLevelProperties.GetMusicLength(),
							 mLevelProperties.GetMusicInitialDelay(), 
							 mLevelProperties.GetMusicTimeBetween());

	// initialise all objects
	LoadContent(graphics->Device());
	Initialise();

	if (m_player)
	{
		m_player->AddAimLineSprite();
		m_player->AddShadowSprite();
	}

	// now order the objects by z value
	// TODO: figure out if this is even needed, there is a z buffer
	OrderDrawablesByDepth();

	// get the backbuffer width and height and determine how much we need to scale by
	// base width and height are 1920x1080 (need to not hard code this)
	float bbWidth = (float)Graphics::GetInstance()->BackBufferWidth();
	float bbHeight = (float)Graphics::GetInstance()->BackBufferHeight();

	float scaleX = bbWidth / 1920.0f;
	float scaleY = bbHeight / 1080.0f;

	m_updateZoneDimensions.X *= scaleX;
	m_updateZoneDimensions.Y *= scaleY;

	ScaleObjects(scaleX, scaleY);

	WeatherManager::GetInstance()->RefreshAssets();

	AddSlowMotionLayer();

	if (mFreshLevelLaunch)
	{
		mFreshLevelLaunch = false;
	}

	// update all the objects at least once at the start
	for (auto & obj : m_gameObjects)
	{
		// if (obj->isPart)
		{
			// TODO: crashing on particles for some reason
		}
		obj->Update(0.0f);
	}

	m_levelLoaded = true;
}

void GameObjectManager::ParseLevelProperties(TiXmlElement * element)
{
	TiXmlElement * child = element->FirstChildElement();

	bool foundLevelProperties = false;
	// loop through our game objects
	while (child)
	{
		// what type of object is this
		const char* objName = child->Value();
		Utilities::ToLower((char *)objName); // TODO: this is nasty, pass a const char * and return a new std::string

		// start looking at what object we need to make
		if (strcmp(objName, "levelproperties") == 0)
		{
			mLevelProperties.XmlRead(child);

			foundLevelProperties = true;

			return;
		}

		child = child->NextSiblingElement();
	}

	GAME_ASSERT(foundLevelProperties);
}

void GameObjectManager::CacheSaveData()
{
	// JB: need to do this still even when we save between levels?
	SaveManager::GetInstance()->SetCurrencyOrbsCollected(mCurrentLevelFile, mCurrentCurrencyOrbIdsCollected);
	SaveManager::GetInstance()->SetBreakablesBroken(mCurrentLevelFile, mCurrentBreakablesBroken);

	mCurrentCurrencyOrbIdsCollected.clear();
	mCurrentBreakablesBroken.clear();
}

void GameObjectManager::SaveGame()
{
	CacheSaveData();

	SaveManager::GetInstance()->WriteSaveFile();
}

void GameObjectManager::SwitchToLevel(const string & level, const string & doorId, bool defer)
{
	if (defer)
	{
		mSwitchToLevel = true;
		mLevelToSwitch = level;
		mDoorIdCameFrom = doorId;
		return;
	}

	SaveGame();

	mLastLevel = mCurrentLevelFile;

	m_camera->SetTargetObject(nullptr);

	mCachedPlayerHealth = m_player != nullptr ? m_player->GetHealth() : -1.0f;

	mCachedPlayerFocus = m_player != nullptr ? m_player->GetCurrentFocusAmount() : -1.0f;

	CacheSaveData();

	UIGameLoading * loadingScreen = static_cast<UIGameLoading*>(UIManager::Instance()->PushUI("gameloading"));

	// Pause so that we don't trigger SwitchToLevel again
	Game::GetInstance()->PauseGame(false);

	loadingScreen->StartLoading(level);

	// NOTE: UIGameLoading takes over from here
}

void GameObjectManager::SaveObjectsToFile(const string & filename)
{
	// loop through the game objects and make sure there are no duplicates
	list<GameObject *> checkedList;

	for (auto & obj : m_gameObjects)
	{
		for (auto checked : checkedList)
		{
			if (checked == obj.get())
			{
				// duplicate
				GAME_ASSERT(false);
				return;
			}
		}

		checkedList.push_back(obj.get());
	}

	std::map<int, GameObject *> sortedObjects;

	list<int> addedIDs;

	for (auto & obj : m_gameObjects)
	{
		for (auto i : addedIDs)
		{
			if (i == obj->ID())
			{
				// make sure IDs are unique
				GAME_ASSERT(i != obj->ID());
				return;
			}
		}
		sortedObjects[obj->ID()] = obj.get();
		addedIDs.push_back(obj->ID());
	}

	XmlDocument doc;
	TiXmlElement * root = new TiXmlElement( "level" );
	root->SetAttribute("audio", ""); 

	TiXmlElement * levelPropsElement = new TiXmlElement("LevelProperties");

	mLevelProperties.XmlWrite(levelPropsElement);

	root->LinkEndChild(levelPropsElement);

	for (auto obj : sortedObjects)
	{
		TiXmlElement * objElem = ConvertObjectToXmlElement(obj.second);

		if (objElem)
		{
			root->LinkEndChild(objElem);
		}
	}

	doc.Save(filename, root);
}

TiXmlElement * GameObjectManager::ConvertObjectToXmlElement(GameObject * object)
{
	string objectType = object->GetTypeName();
	TiXmlElement * element = new TiXmlElement(objectType.c_str());
	object->XmlWrite(element);
	return element;
}

GameObject * GameObjectManager::CreateObject(TiXmlElement * objectElement, const std::vector<unsigned int> & orbsCollected)
{
	// what type of object is this
	const char* gameObjectTypeName = objectElement->Value();
	Utilities::ToLower((char *)gameObjectTypeName); // TODO: this is nasty, pass a const char * and return a new std::string
	
	GameObject * newGameObject = nullptr;

	// start looking at what object we need to make
	if(strcmp(gameObjectTypeName, "player") == 0)
	{
		if (!m_player || Game::GetInstance()->GetIsLevelEditMode()) // only create 1 player unless in editor
		{
			newGameObject = new Player();
			m_player = static_cast<Player*>(newGameObject);

			if (mCachedPlayerHealth != -1.0f)
			{
				m_player->SetHealth(mCachedPlayerHealth);
			}

			if (mCachedPlayerFocus != -1.0f)
			{
				m_player->SetCurrentFocusAmount(mCachedPlayerFocus);
			}
		}
	}
	else if (strcmp(gameObjectTypeName, "parallaxlayer") == 0)
	{
		newGameObject = new ParallaxLayer(m_camera);
	}
	else if (strcmp(gameObjectTypeName, "levelentry") == 0)
	{
		newGameObject = new LevelEntry();
	}
	else if (strcmp(gameObjectTypeName, "solidmovingsprite") == 0)
	{
		newGameObject = new SolidMovingSprite();
	}
	else if (strcmp(gameObjectTypeName, "sprite") == 0)
	{
		newGameObject = new Sprite();
	}
	else if (strcmp(gameObjectTypeName, "movingsprite") == 0)
	{
		newGameObject = new MovingSprite();
	}
	else if (strcmp(gameObjectTypeName, "platform") == 0)
	{
		newGameObject = new Platform();
	}
	else if (strcmp(gameObjectTypeName, "pathingplatform") == 0)
	{
		newGameObject = new PathingPlatform();
	}
	else if (strcmp(gameObjectTypeName, "fallingplatform") == 0)
	{
		newGameObject = new FallingPlatform();
	}
	else if(strcmp(gameObjectTypeName, "particlespray") == 0)
	{
		newGameObject = ReadParticleSpray(objectElement);
	}
	else if (strcmp(gameObjectTypeName, "audioobject") == 0)
	{
		newGameObject = new AudioObject();
	}
	else if (strcmp(gameObjectTypeName, "npc") == 0)
	{
		newGameObject = new NPC();
	}
	else if (strcmp(gameObjectTypeName, "rabbit") == 0)
	{
		newGameObject = new Rabbit();
	}
	else if (strcmp(gameObjectTypeName, "leveltrigger") == 0)
	{
		newGameObject = new LevelTrigger();
	}
	else if (strcmp(gameObjectTypeName, "waterblock") == 0)
	{
		newGameObject = new WaterBlock();
	}
	else if (strcmp(gameObjectTypeName, "butterfly") == 0)
	{
		newGameObject = new Butterfly();
	}
	else if (strcmp(gameObjectTypeName, "scrollingsprite") == 0)
	{
		newGameObject = new ScrollingSprite();
	}
	else if (strcmp(gameObjectTypeName, "solidlinestrip") == 0)
	{
		newGameObject = new SolidLineStrip();
	}
	else if (strcmp(gameObjectTypeName, "npctrigger") == 0)
	{
		newGameObject = new NPCTrigger();
	}
	else if (strcmp(gameObjectTypeName, "forcebox") == 0)
	{
		newGameObject = new ForceBox();
	}
	else if (strcmp(gameObjectTypeName, "textobject") == 0)
	{
		newGameObject = new TextObject();
	}
	else if (strcmp(gameObjectTypeName, "ambientbird") == 0)
	{
		newGameObject = new AmbientBird();
	}
	else if (strcmp(gameObjectTypeName, "activebird") == 0)
	{
		newGameObject = new ActiveBird();
	}
	else if (strcmp(gameObjectTypeName, "saveshrine") == 0)
	{
		newGameObject = new SaveShrine();
	}
    else if (strcmp(gameObjectTypeName, "door") == 0)
    {
        newGameObject = new Door();
    }
	else if (strcmp(gameObjectTypeName, "dojoscrollpickup") == 0)
	{
		newGameObject = new DojoScrollPickup();
	}
	else if (strcmp(gameObjectTypeName, "currencyorb") == 0)
	{
		bool alreadyCollected = std::find(orbsCollected.begin(), orbsCollected.end(), GameObject::GetCurrentGameObjectCount()) != orbsCollected.end();
		if (alreadyCollected && !Game::GetIsLevelEditMode())
		{
			// mimic the creation of an object to keep game ids deterministic
			GameObject::ForceIncrementGameObjectCount();
			newGameObject = nullptr;
		}
		else
		{
			newGameObject = new CurrencyOrb();
			static_cast<CurrencyOrb*>(newGameObject)->SetIsLoadTimeObject(true);
		}
	}
	else if (strcmp(gameObjectTypeName, "breakable") == 0)
	{
		newGameObject = new Breakable();
	}
	else if (strcmp(gameObjectTypeName, "smashable") == 0)
	{
		newGameObject = new Smashable();
	}
	else if (strcmp(gameObjectTypeName, "ghostenemy") == 0)
	{
		newGameObject = new GhostEnemy();
	}
	else if (strcmp(gameObjectTypeName, "healthdevil") == 0)
	{
		newGameObject = new HealthDevil();
	}
	else if (strcmp(gameObjectTypeName, "healthupgradepickup") == 0)
	{
		newGameObject = new HealthUpgradePickup();
	}
	else if (strcmp(gameObjectTypeName, "focusupgradepickup") == 0)
	{
		newGameObject = new FocusUpgradePickup();
	}
	else if (strcmp(gameObjectTypeName, "foliage") == 0)
	{
		newGameObject = new Foliage();
	}
	else if (strcmp(gameObjectTypeName, "skeletonenemy") == 0)
	{
		newGameObject = new SkeletonEnemy();
	}
	else if (strcmp(gameObjectTypeName, "paperpickup") == 0)
	{
		newGameObject = new PaperPickup();
	}
	else if (strcmp(gameObjectTypeName, "keypickup") == 0)
	{
		newGameObject = new KeyPickup();
	}
	else if (strcmp(gameObjectTypeName, "sfxpickup") == 0)
	{
		newGameObject = new SfxPickup();
	}
	else if (strcmp(gameObjectTypeName, "boat") == 0)
	{
		newGameObject = new Boat();
	}

	if (newGameObject && !dynamic_cast<ParticleSpray*>(newGameObject))
	{
		newGameObject->XmlRead(objectElement);
	}
	
	return newGameObject;
}

ParticleSpray * GameObjectManager::ReadParticleSpray(TiXmlElement * element)
{
	string texFileName = XmlUtilities::ReadAttributeAsString(element, "texture", "filename");

	float numParticles = XmlUtilities::ReadAttributeAsFloat(element, "numparticles", "value");

	float posX = XmlUtilities::ReadAttributeAsFloat(element, "position", "x");
	float posY = XmlUtilities::ReadAttributeAsFloat(element, "position", "y");

	GameObject::DepthLayer depthLayer = GameObject::ConvertStringToDepthLayer(XmlUtilities::ReadAttributeAsString(element, "position", "depth_layer"));

	//dimensions 
	float dimX = XmlUtilities::ReadAttributeAsFloat(element, "dimensions", "width");
	float dimY = XmlUtilities::ReadAttributeAsFloat(element, "dimensions", "height");

	float dirX = XmlUtilities::ReadAttributeAsFloat(element, "direction", "x");
	float dirY = XmlUtilities::ReadAttributeAsFloat(element, "direction", "y");

	float spread = XmlUtilities::ReadAttributeAsFloat(element, "spread", "value");

	float minSpeed = XmlUtilities::ReadAttributeAsFloat(element, "speed", "min");
	float maxSpeed = XmlUtilities::ReadAttributeAsFloat(element, "speed", "max");

	float minLive = XmlUtilities::ReadAttributeAsFloat(element, "livetime", "min");
	float maxLive = XmlUtilities::ReadAttributeAsFloat(element, "livetime", "max");

	float minSize = XmlUtilities::ReadAttributeAsFloat(element, "size", "min");
	float maxSize = XmlUtilities::ReadAttributeAsFloat(element, "size", "max");
	bool scaleto = XmlUtilities::ReadAttributeAsBool(element, "size", "scaleto");
	float scaletoValue = XmlUtilities::ReadAttributeAsFloat(element, "size", "scaletovalue");

	float gravity = XmlUtilities::ReadAttributeAsFloat(element, "gravity", "value");

	bool loop = XmlUtilities::ReadAttributeAsBool(element, "loop", "value");
	float loopTime = XmlUtilities::ReadAttributeAsFloat(element, "loop", "looptime");

	float minBrightness = XmlUtilities::ReadAttributeAsFloat(element, "brightness", "min");
	float maxBrightness = XmlUtilities::ReadAttributeAsFloat(element, "brightness", "max"); 

	float spawnSpreadX = XmlUtilities::ReadAttributeAsFloat(element, "position", "spawn_spread_x");
	float spawnSpreadY = XmlUtilities::ReadAttributeAsFloat(element, "position", "spawn_spread_y");

	float fadeInPercentTime = XmlUtilities::ReadAttributeAsFloat(element, "fade_time_percent", "in");
	float fadeOutPercentTime = XmlUtilities::ReadAttributeAsFloat(element, "fade_time_percent", "out");

	Vector3 pos = Vector3(0, -23, 16);
	ParticleSpray * p = ParticleEmitterManager::Instance()->CreateDirectedSprayLoadTime((int)numParticles,
																						Vector2(posX, posY),
																						depthLayer,
																						Vector2(dirX, dirY),
																						spread,
																						Vector2(dimX, dimY),
																						texFileName.c_str(),
																						minSpeed,
																						maxSpeed,
																						minLive,
																						maxLive,
																						minSize,
																						maxSize,
																						gravity,
																						loop,
																						minBrightness,
																						maxBrightness,
																						loopTime,
																						scaleto,
																						scaletoValue,
																						spawnSpreadX,
																						spawnSpreadY,
																						fadeInPercentTime,
																						fadeOutPercentTime);

	p->mPositionalAudioEnabled = XmlUtilities::ReadAttributeAsBool(element, "pos_audio_props", "enabled");

	if (p->mPositionalAudioEnabled)
	{
		p->mPositionalAudio.SetAudioFilename(XmlUtilities::ReadAttributeAsString(element, "pos_audio_props", "file"));
		Vector2 dimensions;
		dimensions.X = XmlUtilities::ReadAttributeAsFloat(element, "pos_audio_props", "dim_x");
		dimensions.Y = XmlUtilities::ReadAttributeAsFloat(element, "pos_audio_props", "dim_y");
		p->mPositionalAudio.SetDimensions(dimensions);
		Vector2 fadeDimensions;
		fadeDimensions.X = XmlUtilities::ReadAttributeAsFloat(element, "pos_audio_props", "fade_dim_x");
		fadeDimensions.Y = XmlUtilities::ReadAttributeAsFloat(element, "pos_audio_props", "fade_dim_y");
		p->mPositionalAudio.SetFadeDimensions(fadeDimensions);
		p->mPositionalAudioStartDelay = XmlUtilities::ReadAttributeAsFloat(element, "pos_audio_props", "play_delay");
		p->mPositionalAudio.SetRepeat(XmlUtilities::ReadAttributeAsBool(element, "pos_audio_props", "repeat"));
	}

	p->Warm(true);

	return p;
}

void GameObjectManager::DeleteGameObjects()
{
	for (auto g : m_gameObjects)
	{
		g.reset();
	}

	m_gameObjects.clear();
	ParticleEmitterManager::Instance()->ClearParticles();
	
	mSlowMotionLayer = nullptr;

	m_levelLoaded = false;

	m_player = nullptr;
}

void GameObjectManager::AddGameObject(GameObject * object, bool editModeAdd)
{
	GAME_ASSERT(object);

	if (!object)
	{
		return;
	}

#if _DEBUG
	// don't let any new objects be created unless we purposely added them
	if (Game::GetIsLevelEditMode() && !editModeAdd)
	{
		return;
	}
#endif

	auto obj = shared_ptr<GameObject>(object);
	m_gameObjects.push_back(obj);

	// no need to manually add it to gameObject list as this is done automatically in the constructor
	if (object->IsDrawable())
	{
		object->LoadContent(Graphics::GetInstance()->Device());
	}

	object->Initialise();
	
	// SCALE
	// get the backbuffer width and height and determine how much we need to scale by
	// base width and height are 1920x1080 (need to not hard code this)
	float bbWidth = (float)Graphics::GetInstance()->BackBufferWidth();
	float bbHeight = (float)Graphics::GetInstance()->BackBufferHeight();

	float scaleX = bbWidth / 1920.0f;
	float scaleY = bbHeight / 1080.0f;

	// never scale position during the game, only before
	// it is always assumed you are giving a valid position during the game
	object->Scale(scaleX, scaleY, false);

	// TODO: Optimise, huge bottleneck
	LOG_INFO("optimise AddGameObject");

	// TODO: we shouldn't actually need to do this 
	OrderDrawablesByDepth();
}

void GameObjectManager::AddGameObjectViaLevelEditor(GameObject * object)
{
	// don't let any new objects be created unless we purposely added them
	if (!Game::GetIsLevelEditMode())
	{
		return;
	}

	// TODO: figure out WHY I'm doing these casts...
	DrawableObject * drawable = dynamic_cast<DrawableObject*>(object);
	AudioObject * audiobj = dynamic_cast<AudioObject*>(object);
	NPCTrigger * npcTrigger = dynamic_cast<NPCTrigger*>(object);
	if (drawable)
	{
		GameObjectManager::Instance()->AddGameObject(drawable, true);
	}
	else if (audiobj)
	{
		GameObjectManager::Instance()->AddGameObject(audiobj, true);
	}
	else if (npcTrigger)
	{
		GameObjectManager::Instance()->AddGameObject(npcTrigger, true);
	}
	else
	{
		GameObjectManager::Instance()->AddGameObject(object, true);
	}
}

// order our drawable list by depth - this a once off that should be done at initialise stage
void GameObjectManager::OrderDrawablesByDepth()
{
	m_gameObjects.sort(DepthSortPredicate());
}

GameObject * GameObjectManager::CopyObject(GameObject * toCopy)
{
	GAME_ASSERT(toCopy);
	if (!toCopy)
	{
		return nullptr;
	}

	auto xmlElement = GameObjectManager::Instance()->ConvertObjectToXmlElement(toCopy);

	if (!xmlElement)
	{
		GAME_ASSERT(false);
		return nullptr;
	}

	return CreateObject(xmlElement, std::vector<unsigned int>());
}

void GameObjectManager::AddSlowMotionLayer()
{
	if (mSlowMotionLayer != nullptr)
	{
		return;
	}

	float gameScale = Game::GetGameScale().X;
	mSlowMotionLayer = new ParallaxLayer(Camera2D::GetInstance());

	mSlowMotionLayer->m_textureFilename = "Media\\slow_motion_overlay.png";
	mSlowMotionLayer->m_drawAtNativeDimensions = true;
	mSlowMotionLayer->m_updateable = true;
	mSlowMotionLayer->m_position = Vector2(0.0f, 0.0f * gameScale);
	mSlowMotionLayer->m_dimensions = Vector2(2048.0f, 1200.0f);
	mSlowMotionLayer->mRepeatTextureX = false;
	mSlowMotionLayer->mRepeatTextureY = false;
	mSlowMotionLayer->m_repeatWidth = 1920.0f;
	mSlowMotionLayer->m_cameraParallaxMultiplierX = 0.0f;
	mSlowMotionLayer->m_cameraParallaxMultiplierY = 0.0f;
	mSlowMotionLayer->m_followCamXPos = true;
	mSlowMotionLayer->m_followCamYPos = true;
	mSlowMotionLayer->m_autoScrollY = false;
	mSlowMotionLayer->m_autoScrollX = false;
	mSlowMotionLayer->m_autoScrollXSpeed = 0.0f;
	mSlowMotionLayer->m_autoScrollYSpeed = 0.0f;
	mSlowMotionLayer->EffectName = "effectlighttexture";
	mSlowMotionLayer->m_alpha = 0.0f;
	mSlowMotionLayer->mNoiseShaderIntensity = 0.01f;

	GameObjectManager::Instance()->AddGameObject(mSlowMotionLayer);
}

void GameObjectManager::GetSolidSpritesOnScreen(std::list<GameObject *> & toPopulate)
{
	toPopulate.clear();

	Camera2D * cam = Camera2D::GetInstance();

	for (auto & obj : m_gameObjects)
	{
		GameObject * objRawPtr = obj.get();
		GAME_ASSERT(objRawPtr);

		if (objRawPtr->IsSolidSprite() &&
			cam->IsObjectInView(objRawPtr))
		{
			toPopulate.push_back(objRawPtr);
		}
	}
}

void GameObjectManager::SetCurrencyOrbCollected(unsigned int orbId)
{
	mCurrentCurrencyOrbIdsCollected.push_back(orbId);
}

void GameObjectManager::SetBreakableBroken(unsigned int breakableId)
{
	mCurrentBreakablesBroken.push_back(breakableId);
}

void GameObjectManager::QuitLevel()
{
	SaveManager::GetInstance()->ResetSession();

	mLastLevel = "";
	mSwitchToLevel = false;
	mLevelToSwitch = "";
	mDoorIdCameFrom = "";
	mCachedPlayerHealth = -1.0f;
	DeleteGameObjects();
}

void GameObjectManager::SpawnHealthIncrease(const Vector2 & position)
{
	HealthIncreasePickup * p = new HealthIncreasePickup();
	p->SetTextureFilename("Media\\objects\\sake_bottle.png");
	p->SetIsNativeDimensions(true);
	p->SetCollisionDimensions(Vector2(25.0f, 90.0f));
	p->SetApplyGravity(true);
	p->SetGravityApplyAmount(1.0f);
	p->SetUpdateable(true);
	p->SetPassive(false);
	p->SetXY(position.X, position.Y + 20.0f);
	float xSign = (float)(rand() % 2 == 0 ? 1.0 : -1.0f);
	float randVelX = (rand() % 15) + 15.0f;
	float randVelY = (rand() % 10) + 5.0f;
	p->SetMaxVelocityXY(30.0f, 20.0f);
	p->SetVelocityXY(randVelX * xSign, randVelY);
	p->SetResistanceXY(0.95f, 1.0f);
	p->EffectName = "effectlighttexture";

	AddGameObject(p);
}
