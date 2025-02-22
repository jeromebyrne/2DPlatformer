#include "precompiled.h"
#include "ParticleEmitterManager.h"
#include "ParticleSpray.h"
#include "Game.h"

// radial blood properties
const unsigned int kBloodRadialMinSize = 30;
const unsigned int kBloodRadialMaxSize = 60;
const float kBloodRadialMinSpeed = 1.0f;
const float kBloodRadialMaxSpeed = 3.0f;
const unsigned int kBloodRadialGravity = 1.1f;
const char kBloodTextureFileName[] = "Media\\bloodparticle.png";
const float kBloodRadialScaleTo = 1.8f;
float kBloodRadialMinLiveTime = 0.5f;
float kBloodRadialMaxLiveTime = 0.7f;

// directed blood properties
const unsigned int kBloodDirectedMinLiveTime = 0.5f;
const unsigned int kBloodDirectedMaxLiveTime = 0.65f;
const unsigned int kBloodDirectedMinSize = 1;
const unsigned int kBloodDirectedMaxSize = 6;
const float kBloodDirectedMinSpeed = 1.0f;
const float kBloodDirectedMaxSpeed = 8.0f;
const unsigned int kBloodDirectedGravity = 1.3;
const float kBloodDirectedScaleTo = 12.0f;

ParticleEmitterManager* ParticleEmitterManager::m_instance = 0;

ParticleEmitterManager::ParticleEmitterManager(void)
{
}

ParticleEmitterManager::~ParticleEmitterManager(void)
{
}

void ParticleEmitterManager::Initialise(Graphics * graphicsSystem)
{
	m_graphicsSystem = graphicsSystem;
}

void ParticleEmitterManager::ClearParticles()
{
	list<ParticleSpray*>::iterator current = m_particleSprayList.begin();

	for (; current != m_particleSprayList.end(); current++)
	{
		if (*current)
		{
			delete (*current);
		}
	}

	m_particleSprayList.clear();
}

ParticleSpray * ParticleEmitterManager::CreateRadialBloodSpray(unsigned int numParticles,
													Vector2 position,
													bool loop,
													float loopTime)
{
	float creationTime = Timing::Instance()->GetTotalTimeSeconds();

	float gameScale = Game::GetGameScale().X;

	list<Particle> particleList;
	for(int i = 0; i < numParticles; i++)
	{
		Particle p;

		float randDirX = ((rand() % 100)+1) * 0.01f;
		float randDirY = ((rand() % 100)+1) * 0.01f;
		
		int directionXSign = rand() % 2;
		int directionYSign = rand() % 2;

		if(directionXSign == 0)
		{
			randDirX = -randDirX;
			// p.FlippedHorizontal = true; // just do this here for convenience
		}
		if(directionYSign)
		{
			randDirY = -randDirY;
			// p.FlippedVertical = true; // just do this here for convenience
		}
		p.DirectionX = randDirX;
		p.DirectionY = randDirY;

		p.MaxLiveTime = kBloodRadialMaxLiveTime - (((kBloodRadialMaxLiveTime - kBloodRadialMinLiveTime) / numParticles) * i);

		unsigned spawnSpread = 4 * numParticles;
		p.PosXOffset = rand() % spawnSpread;
		p.PosX = position.X + p.PosXOffset;
		p.StartPosX = position.X + p.PosXOffset;

		p.PosYOffset = (rand() % spawnSpread) * 1.4f;
		p.PosY = position.Y + p.PosYOffset;
		p.StartPosY = position.Y + p.PosYOffset;

		int randSize = ((rand() % (int)(kBloodRadialMaxSize - kBloodRadialMinSize)) + kBloodRadialMinSize + 1) * gameScale;
		p.Size = randSize;
		p.StartSize = randSize;

		float randSpeed = (((rand() % (int)(kBloodRadialMaxSpeed * 100 - kBloodRadialMinSpeed * 100)) * 0.01) + kBloodRadialMinSpeed + 0.01) * gameScale;
		p.Speed = randSpeed;
		p.StartSpeed = randSpeed; // our original start speed

		p.Gravity = kBloodRadialGravity;

		p.Brightness = 1; // this is determined in the shader
		
		particleList.push_back(p);
	}

	// create spray
	ParticleSpray * spray = new ParticleSpray(true,
												position,
												GameObject::kBloodSpray1,
												Vector2(3200.0f, 2500.0f), 
												kBloodTextureFileName,
												particleList,
												loop,
												loopTime,
												true,
												kBloodRadialScaleTo * gameScale,
												0.0f,
												0.0f);

	// add the spray to the game world
	GameObjectManager::Instance()->AddGameObject(spray);

	return spray;
}

ParticleSpray * ParticleEmitterManager::CreateDirectedBloodSpray(int numParticles,
																 Vector2 position, 
																 Vector2 direction,
																 float spread,
																 bool loop,
																 float loopTime)
{
	float creationTime = Timing::Instance()->GetTotalTimeSeconds();

	float gameScale = Game::GetGameScale().X;

	list<Particle> particleList;
	for(int i = 0; i < numParticles; i++)
	{
		Particle p;

		float randSpread = ((rand() % (int)(spread * 100)) * 0.01);

		// p.FlippedHorizontal = true; 
		// p.FlippedVertical = false; 
		p.DirectionX = direction.X + randSpread;
		p.DirectionY = direction.Y - randSpread;

		p.MaxLiveTime = kBloodDirectedMaxLiveTime - (((kBloodDirectedMaxLiveTime - kBloodDirectedMinLiveTime) / numParticles) * i);
		p.PosX = position.X;
		p.PosY = position.Y;
		p.StartPosX = position.X; // our original start position
		p.StartPosY = position.Y;

		int randSize = ((rand() % (int)(kBloodDirectedMaxSize - kBloodDirectedMinSize)) + kBloodDirectedMinSize + 1) * gameScale;
		p.Size = randSize;
		p.StartSize = randSize;

		float randSpeed = (((rand() % (int)(kBloodDirectedMaxSpeed * 100.0f - kBloodDirectedMinSpeed * 100.0f)) * 0.01f) + kBloodDirectedMinSpeed + 0.01f) * gameScale;
		p.Speed = randSpeed;
		p.StartSpeed = randSpeed; // our original start speed

		p.Gravity = kBloodDirectedGravity;
		p.Brightness = 1;

		particleList.push_back(p);
	}

	// create spray
	ParticleSpray * spray = new ParticleSpray(true,
												position,
												GameObject::kBloodSpray1,
												Vector2(3200.0f, 1200.0f),
												kBloodTextureFileName,
												particleList,
												loop,
												loopTime,
												true,
												kBloodDirectedScaleTo * gameScale,
												0.0f,
												0.0f);

	spray->SetGeneralDirectionValue(Vector2(direction.X, direction.Y));
	spray->SetSpreadValue(spread);
	spray->SetMinSpeedValue(kBloodDirectedMinSpeed);
	spray->SetMaxSpeedValue(kBloodDirectedMaxSpeed);
	spray->SetMinLiveTime(kBloodDirectedMinLiveTime);
	spray->SetMaxLiveTime(kBloodDirectedMaxLiveTime);
	spray->SetMinSize(kBloodDirectedMinSize);
	spray->SetMaxSize(kBloodDirectedMaxSize);
	spray->SetGravityValue(kBloodDirectedGravity);
	spray->SetMinBrightnessValue(1.0f);
	spray->SetMaxBrightnessValue(1.0f);
	spray->SetNumParticlesValue(numParticles);

	// add the spray to the game world
	GameObjectManager::Instance()->AddGameObject(spray);

	return spray;
}

ParticleSpray * ParticleEmitterManager::CreateRadialSpray(int numParticles,
												 Vector2 position,
												 GameObject::DepthLayer depthLayer,
												 Vector2 drawBoundingBox, // determines when we stop drawing particles (camera stops seeing them)
												 string textureFileName,
												 float minSpeed,
												 float maxSpeed,
												 float minLiveTime,
												 float maxLiveTime,
												 float minSize,
												 float maxSize,
												 float gravity,
												 bool loop,
												 float minBrightness,
												 float maxBrightness,
												 float loopTime,
												 bool scalesByLiveTime,
												 float scaleTo,
												 float fadeInPercentTime,
												 float fadeOutPercentTime,
												 float spawnSpreadX,
												 float spawnSpreadY)
{
	if(maxBrightness > 1)
	{
		maxBrightness = 1;
	}
	else if(maxBrightness < 0)
	{
		maxBrightness = 0;
	}

	//float creationTime = Timing::Instance()->GetTotalTimeSeconds();

	float gameScale = Game::GetGameScale().X;

	// create our individual particles
	list<Particle> particleList;
	for(int i = 0; i < numParticles; i++)
	{
		Particle p;

		// random direction ======================================
		float randDirX = ((rand() % 100)+1) * 0.01f;
		float randDirY = ((rand() % 100)+1) * 0.01f;
		
		int directionXSign = rand() % 2;
		int directionYSign = rand() % 2;

		if(directionXSign == 0)
		{
			randDirX = -randDirX;
			// p.FlippedHorizontal = true; // just do this here for convenience
		}
		if(directionYSign)
		{
			randDirY = -randDirY;
			// p.FlippedVertical = true; // just do this here for convenience
		}
		p.DirectionX = randDirX;
		p.DirectionY = randDirY;
		// =======================================================

		p.MaxLiveTime = maxLiveTime - (((maxLiveTime - minLiveTime) / numParticles) * i);
		
		if (spawnSpreadX == 0.0f)
		{
			p.PosX = position.X;
			p.StartPosX = position.X; // our original start position
		}
		else
		{
			p.PosXOffset = rand() % ((unsigned)(spawnSpreadX * 10.0f) + 1);

			if (rand() % 2 == 1)
			{
				p.PosXOffset *= -1;
			}

			p.PosX = position.X + p.PosXOffset;
			p.StartPosX = position.X + p.PosXOffset;
		}

		if (spawnSpreadY == 0.0f)
		{
			p.PosY = position.Y;
			p.StartPosY = position.Y;
		}
		else
		{
			p.PosYOffset = rand() % ((unsigned)(spawnSpreadY * 10.0f) + 1);

			if (rand() % 2 == 1)
			{
				p.PosYOffset *= -1;
			}

			p.PosY = position.Y + p.PosYOffset;
			p.StartPosY = position.Y + p.PosYOffset;
		}
		
		if(maxSize <= minSize)
		{
			p.Size = maxSize * gameScale;
			p.StartSize = maxSize * gameScale;
		}
		else
		{
			int randSize = ((rand() % (int)(maxSize - minSize)) + minSize + 1) * gameScale;
			p.Size = randSize;
			p.StartSize = randSize;
		}

		if(maxSpeed <= minSpeed)
		{
			p.Speed = maxSpeed * gameScale;
			p.StartSpeed = maxSpeed * gameScale; // our original start speed
		}
		else
		{
			float randSpeed = (((rand() % (int)(maxSpeed * 100.0f - minSpeed * 100.0f)) * 0.01f) + minSpeed + 0.01f) * gameScale;
			p.Speed = randSpeed;
			p.StartSpeed = randSpeed; // our original start speed
		}
		p.Gravity = gravity;

		if(maxBrightness <= minBrightness)
		{
			p.Brightness = maxBrightness;
		}
		else
		{
			// do random brightness
			p.Brightness = ((rand() % (int)(maxBrightness * 100 - minBrightness * 100)) * 0.01) + minBrightness + 0.01;
		}
		
		particleList.push_back(p);
	}
	// create spray
	ParticleSpray * spray = new ParticleSpray(false,
												position,
												depthLayer,
												drawBoundingBox,
												(char*)textureFileName.c_str(),
												particleList, 
												loop,
												loopTime,
												scalesByLiveTime, 
												scaleTo * gameScale,
												spawnSpreadX,
												spawnSpreadY);

	spray->SetFadeInPercentTime(fadeInPercentTime);
	spray->SetFadeOutPercentTime(fadeOutPercentTime);

	// add the spray to the game world
	GameObjectManager::Instance()->AddGameObject(spray);

	return spray;
}

ParticleSpray * ParticleEmitterManager::CreateDirectedSpray(int numParticles,
															 Vector2 position,
															 GameObject::DepthLayer depthLayer,
															 Vector2 direction,
															 float spread,
															 Vector2 drawBoundingBox, // determines when we stop drawing particles (camera stops seeing them)
															 string textureFileName,
															 float minSpeed,
															 float maxSpeed,
															 float minLiveTime,
															 float maxLiveTime,
															 float minSize,
															 float maxSize,
															 float gravity,
															 bool loop,
															 float minBrightness,
															 float maxBrightness,
															 float loopTime,
															 bool scalesByLiveTime,
															 float scaleTo,
															 float spawnSpreadX,
															 float spawnSpreadY,
															 float fadeInPercentTime,
															 float fadeOutPercentTime,
															 bool originalOrientation)
{
	// seed the random number generator
	// srand(timeGetTime());

	if(maxBrightness > 1)
	{
		maxBrightness = 1;
	}
	else if(maxBrightness < 0)
	{
		maxBrightness = 0;
	}

	if(spread > 1)
	{
		spread = 1;
	}
	else if(spread < 0)
	{
		spread = 0;
	}
	
	float creationTime = Timing::Instance()->GetTotalTimeSeconds();

	float gameScale = Game::GetGameScale().X;

	// create our individual particles
	list<Particle> particleList;
	for(int i = 0; i < numParticles; i++)
	{
		Particle p;

		float randSpread = spread != 0.0f ? ((rand() % (int)(spread * 100.0f)) * 0.01f) : 0.0f;

		p.DirectionX = direction.X + randSpread;
		p.DirectionY = direction.Y - randSpread;
			
		if(maxLiveTime == minLiveTime)
		{
			maxLiveTime += 0.1;
		}

		p.MaxLiveTime = maxLiveTime - (((maxLiveTime - minLiveTime) / numParticles) * i);

		if (spawnSpreadX == 0.0f)
		{
			p.PosX = position.X;
			p.StartPosX = position.X; // our original start position
		}
		else
		{
			p.PosXOffset = rand() % ((unsigned)(spawnSpreadX * 10.0f) + 1);

			if (rand() % 2 == 1)
			{
				p.PosXOffset *= -1;
			}

			p.PosX = position.X + p.PosXOffset;
			p.StartPosX = position.X + p.PosXOffset;
		}

		if (spawnSpreadY == 0.0f)
		{
			p.PosY = position.Y;
			p.StartPosY = position.Y;
		}
		else
		{
			// TODO: this can only be an INT
			p.PosYOffset = rand() % ((unsigned)(spawnSpreadY * 10.0f) + 1);
			if (rand() % 2 == 1)
			{
				p.PosYOffset *= -1;
			}

			p.PosY = position.Y + p.PosYOffset;
			p.StartPosY = position.Y + p.PosYOffset;
		}
		
		if(maxSize <= minSize)
		{
			p.Size = maxSize * gameScale;
			p.StartSize = maxSize * gameScale;
		}
		else
		{
			int randSize = ((rand() % (int)(maxSize - minSize)) + minSize + 1) * gameScale;
			p.Size = randSize;
			p.StartSize = randSize;
		}

		if(maxSpeed <= minSpeed)
		{
			p.Speed = maxSpeed * gameScale;
			p.StartSpeed = maxSpeed * gameScale; // our original start speed
		}
		else
		{
			float randSpeed = (((rand() % (int)(maxSpeed * 100 - minSpeed * 100)) * 0.01) + minSpeed + 0.01) * gameScale;
			p.Speed = randSpeed;
			p.StartSpeed = randSpeed; // our original start speed
		}
		p.Gravity = gravity;

		if(maxBrightness <= minBrightness)
		{
			p.Brightness = maxBrightness;
		}
		else
		{
			// do random brightness
			p.Brightness = ((rand() % (int)(maxBrightness * 100 - minBrightness * 100)) * 0.01) + minBrightness + 0.01;
		}
		
		particleList.push_back(p);
	}
	// create spray
	ParticleSpray * spray = new ParticleSpray(false,
											position,
											depthLayer,
											drawBoundingBox,
											(char*)textureFileName.c_str(),
											particleList,
											loop,
											loopTime,
											scalesByLiveTime, scaleTo * gameScale,
											spawnSpreadX,
											spawnSpreadY);

	spray->SetGeneralDirectionValue(Vector2(direction.X, direction.Y));
	spray->SetSpreadValue(spread);
	spray->SetMinSpeedValue(minSpeed);
	spray->SetMaxSpeedValue(maxSpeed);
	spray->SetMinLiveTime(minLiveTime);
	spray->SetMaxLiveTime(maxLiveTime);
	spray->SetMinSize(minSize);
	spray->SetMaxSize(maxSize);
	spray->SetGravityValue(gravity);
	spray->SetMinBrightnessValue(minBrightness);
	spray->SetMaxBrightnessValue(maxBrightness);
	spray->SetNumParticlesValue(numParticles);
	spray->SetFadeInPercentTime(fadeInPercentTime);
	spray->SetFadeOutPercentTime(fadeOutPercentTime);

	// add the spray to the game world
	GameObjectManager::Instance()->AddGameObject(spray);

	return spray;
}


ParticleSpray * ParticleEmitterManager::CreateDirectedSprayLoadTime(int numParticles,
																	 Vector2 position, 
																	 GameObject::DepthLayer depthLayer,
																	 Vector2 direction,
																	 float spread,
																	 Vector2 drawBoundingBox, // determines when we stop drawing particles (camera stops seeing them)
																	 string textureFileName,
																	 float minSpeed,
																	 float maxSpeed,
																	 float minLiveTime,
																	 float maxLiveTime,
																	 float minSize,
																	 float maxSize,
																	 float gravity,
																	 bool loop,
																	 float minBrightness,
																	 float maxBrightness,
																	 float loopTime,
																	 bool scalesByLiveTime,
																	 float scaleTo,
																	 float spawnSpreadX,
																	 float spawnSpreadY,
																	 float fadeInPercentTime,
																	 float fadeOutPercentTime)
{
	// seed the random number generator
	//srand(timeGetTime());

	if(maxBrightness > 1)
	{
		maxBrightness = 1;
	}
	else if(maxBrightness < 0)
	{
		maxBrightness = 0;
	}

	if(spread > 1)
	{
		spread = 1;
	}
	else if(spread < 0)
	{
		spread = 0;
	}
	
	unsigned long creationTime = Timing::Instance()->GetTotalTimeSeconds();

	float gameScale = Game::GetGameScale().X;

	// create our individual particles
	list<Particle> particleList;
	for(int i = 0; i < numParticles; i++)
	{
		Particle p;

		float randSpread = 0.0f; 
		if (spread > 0.0f)
		{
			((rand() % (int)(spread * 100)) * 0.01);
		}

		p.DirectionX = direction.X - randSpread;
		p.DirectionY = direction.Y + randSpread;
			
		if(maxLiveTime == minLiveTime)
		{
			maxLiveTime += 0.1;
		}

		p.MaxLiveTime = maxLiveTime - (((maxLiveTime - minLiveTime) / numParticles) * i);
		
		if (spawnSpreadX == 0.0f)
		{
			p.PosX = position.X;
			p.StartPosX = position.X; // our original start position
		}
		else
		{
			p.PosXOffset = rand() % ((unsigned)(spawnSpreadX * 10.0f) + 1);

			if (rand() % 2 == 1)
			{
				p.PosXOffset *= -1;
			}

			p.PosX = position.X + p.PosXOffset;
			p.StartPosX = position.X + p.PosXOffset;
		}

		if (spawnSpreadY == 0.0f)
		{
			p.PosY = position.Y;
			p.StartPosY = position.Y;
		}
		else
		{
			p.PosYOffset = rand() % ((unsigned)(spawnSpreadY * 10.0f) + 1);

			if (rand() % 2 == 1)
			{
				p.PosYOffset *= -1;
			}

			p.PosY = position.Y + p.PosYOffset;
			p.StartPosY = position.Y + p.PosYOffset;
		}

		if(maxSize <= minSize)
		{
			p.Size = maxSize * gameScale;
			p.StartSize = maxSize * gameScale;
		}
		else
		{
			int randSize = ((rand() % (int)(maxSize - minSize)) + minSize + 1) * gameScale;
			p.Size = randSize;
			p.StartSize = randSize;
		}

		if(maxSpeed <= minSpeed)
		{
			p.Speed = maxSpeed * gameScale;
			p.StartSpeed = maxSpeed * gameScale; // our original start speed
		}
		else
		{
			float randSpeed = (((rand() % (int)(maxSpeed * 100 - minSpeed * 100)) * 0.01) + minSpeed + 0.01) * gameScale;
			p.Speed = randSpeed;
			p.StartSpeed = randSpeed; // our original start speed
		}
		p.Gravity = gravity;

		if(maxBrightness <= minBrightness)
		{
			p.Brightness = maxBrightness;
		}
		else
		{
			// do random brightness
			p.Brightness = ((rand() % (int)(maxBrightness * 100 - minBrightness * 100)) * 0.01) + minBrightness + 0.01;
		}
		
		particleList.push_back(p);
	}
	// create spray
	ParticleSpray * spray = new ParticleSpray(false,
												position,
												depthLayer,
												drawBoundingBox,
												(char*)textureFileName.c_str(),
												particleList, 
												loop,
												loopTime,
												scalesByLiveTime,
												scaleTo * gameScale,
												spawnSpreadX,
												spawnSpreadY);

	spray->SetGeneralDirectionValue(Vector2(direction.X, direction.Y));
	spray->SetSpreadValue(spread);
	spray->SetMinSpeedValue(minSpeed);
	spray->SetMaxSpeedValue(maxSpeed);
	spray->SetMinLiveTime(minLiveTime);
	spray->SetMaxLiveTime(maxLiveTime);
	spray->SetMinSize(minSize);
	spray->SetMaxSize(maxSize);
	spray->SetGravityValue(gravity);
	spray->SetMinBrightnessValue(minBrightness);
	spray->SetMaxBrightnessValue(maxBrightness);
	spray->SetNumParticlesValue(numParticles);
	spray->SetFadeInPercentTime(fadeInPercentTime);
	spray->SetFadeOutPercentTime(fadeOutPercentTime);

	return spray;
}




