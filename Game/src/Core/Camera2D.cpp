#include "precompiled.h"
#include "Camera2D.h"
#include "MovingSprite.h"
#include "Game.h"
#include "UIManager.h"
#include "NPCManager.h"

Camera2D * Camera2D::mInstance = 0;

Camera2D::Camera2D(int screenWidth, int screenHeight, float x, float y, float z): 
	m_position(x,y,z), 
	m_width(screenWidth),
	m_height(screenHeight),
	mZoomPercent(1.0f),
	mCurrentlyShaking(false),
	mCurrentShakeIntensity(1.0f),
	mShakeStartTime(0.0f),
	mCurrentShakeDuration(0.0f),
	mTargetObject(nullptr),
	mFollowX(true),
	mFollowY(true)
{
	// Initialize the world matrix
    D3DXMatrixIdentity( &m_world );
	D3DXMatrixIdentity( &m_view );

	D3DXMatrixTranslation(&m_view, -m_position.X, -m_position.Y, m_position.Z);

	// create orthographic projection
	D3DXMatrixOrthoLH(&m_projection, (float)screenWidth, (float)screenHeight, 0.0f, (std::numeric_limits<float>::max)());

	mInstance = this;
}

Camera2D::~Camera2D(void)
{
}

bool Camera2D::IsObjectInView(GameObject * object)
{
#if _DEBUG
	if (Game::GetInstance()->GetIsLevelEditMode())
	{
		// this is to force draw everythign when in level edit mode
		return true;
	}
#endif
	bool inView = false;

	float objectX = object->X() - object->GetCurrentParallaxOffsetX();
	float objectY = object->Y() - object->GetCurrentParallaxOffsetY();
	float screenLeft = m_position.X - m_width * 0.5f;
	float screenRight = m_position.X + m_width * 0.5f;
	float screenBottom = m_position.Y - m_height * 0.5f;
	float screenTop = m_position.Y + m_height * 0.5f;

	// first check if the objects origin is in view
	if(objectX > screenLeft && objectX < screenRight &&
	   objectY > screenBottom && objectY < screenTop)
	{
		inView = true;
	}
	// ok so the origin might not be in view but part of the object may be in view
	if(!inView)
	{
		float objectRight = (object->GetRotationAngle() == 0.0f ? object->Right() : object->GetRightRotated()) - object->GetCurrentParallaxOffsetX();
		float objectLeft = (object->GetRotationAngle() == 0.0f ? object->Left() : object->GetLeftRotated()) - object->GetCurrentParallaxOffsetX();
		float objectTop = (object->GetRotationAngle() == 0.0f ? object->Top() : object->GetTopRotated()) + object->GetCurrentParallaxOffsetY();
		float objectBottom = (object->GetRotationAngle() == 0.0f ? object->Bottom() : object->GetBottomRotated()) - object->GetCurrentParallaxOffsetY();

		// check if we are partially in view on the x plane
		if(objectRight > screenLeft && objectLeft < screenRight)
		{
			// now check if we are partially in view on the y plane
			if(objectTop > screenBottom && objectBottom < screenTop)
			{
				// we are viewable
				inView = true;
			}
		}
	}
	return inView;
}

bool Camera2D::IsWorldPosInView(Vector2 & pos, float parallaxXOffset, float parallaxYOffset)
{
	bool inView = false;

	float objectX = pos.X - parallaxXOffset;
	float objectY = pos.Y - parallaxYOffset;
	float screenLeft = m_position.X - m_width * 0.5f;
	float screenRight = m_position.X + m_width * 0.5f;
	float screenBottom = m_position.Y - m_height * 0.5f;
	float screenTop = m_position.Y + m_height * 0.5f;

	// first check if the objects origin is in view
	if (objectX > screenLeft && objectX < screenRight &&
		objectY > screenBottom && objectY < screenTop)
	{
		inView = true;
	}

	return inView;
}

void Camera2D::SetBounds(float left, float right, float top, float bottom)
{
	mBoundsTopLeft.X = left;
	mBoundsTopLeft.Y = top;
	mBoundsBottomRight.X = right;
	mBoundsBottomRight.Y = bottom;
}

void Camera2D::FollowObjectWithOffset(GameObject * object)
{
	m_position.X = object->X() + mTargetOffset.X;
	m_position.Y = object->Y() + mTargetOffset.Y;
}

void Camera2D::Update()
{
	D3DXMatrixTranslation(&m_view, -m_position.X, -m_position.Y, m_position.Z);

	if (mCurrentlyShaking)
	{
		float currentTime = Timing::Instance()->GetTotalTimeSeconds();

		float timeDiff = (mShakeStartTime + mCurrentShakeDuration) - currentTime;

		if (timeDiff <= 0.0f)
		{
			mCurrentlyShaking = false;
		}
		else
		{
			float shakePercentTime = timeDiff / mCurrentShakeDuration;

			if (mFollowX == true)
			{
				bool minusX = (rand() % 2) == 1;

				if (minusX)
				{
					m_position.X += mCurrentShakeIntensity * shakePercentTime * Timing::Instance()->GetTimeModifier();
				}
				else
				{
					m_position.X -= mCurrentShakeIntensity * shakePercentTime * Timing::Instance()->GetTimeModifier();
				}
			}

			if (mFollowY == true)
			{
				bool minusY = (rand() % 2) == 1;

				if (minusY)
				{
					m_position.Y += mCurrentShakeIntensity * shakePercentTime * Timing::Instance()->GetTimeModifier();
				}
				else
				{
					m_position.Y -= mCurrentShakeIntensity * shakePercentTime  * Timing::Instance()->GetTimeModifier();
				}
			}
		}
	}

#if _DEBUG
	CheckDebugCamerCommands();
#endif
}

void Camera2D::FollowTargetObjectWithLag(bool forceUpdate, float overrideLagX, float overrideLagY)
{
#if _DEBUG
	if (Game::GetInstance()->GetIsLevelEditMode())
	{
		return;
	}
#endif

	if (!mTargetObject)
	{
		return;
	}

	if (mFollowX)
	{
		// get the x and y distance between the camera and the object
		float distanceX = 0.0f;

		distanceX = m_position.X - (mTargetObject->X() + (mTargetOffset.X * mZoomPercent));

		bool enemiesInWorld = NPCManager::Instance()->IsAnyEnemyNPCInWorld();

		// increased lag when fighting enemies
		float xLag = enemiesInWorld ? 40.0f * mZoomPercent : mTargetLag.X * mZoomPercent;

		if (overrideLagX != 0.0f)
		{
			xLag = overrideLagX;
		}

		if (xLag < 1.0f)
		{
			xLag = 1.0f;
		}
		m_position.X -= distanceX / xLag;
	}
	
	if (mFollowY)
	{
		float yLag = mTargetLag.Y;

		if (overrideLagY != 0.0f)
		{
			yLag = overrideLagY;
		}

		if (yLag < 1.0f)
		{
			yLag = 1.0f;
		}

		float distanceY = m_position.Y - (mTargetObject->Y() + mTargetOffset.Y * mZoomPercent);

		m_position.Y -= distanceY / yLag;
	}
}

bool Camera2D::IsCameraOriginInsideObject(GameObject * object)
{
	float right = object->Right();
	float left = object->Left();
	float top = object->Top();
	float bottom = object->Bottom();

	if (m_position.X < object->Right() &&
		m_position.X > object->Left() &&
		m_position.Y < object->Top() &&
		m_position.Y > object->Bottom())
	{
		return true;
	}

	return false;
}

bool Camera2D::IsCameraOriginInsideRect(Vector2 pos, Vector2 dimensions)
{
	if (m_position.X < pos.X + dimensions.X * 0.5f &&
		m_position.X > pos.X - dimensions.X * 0.5f &&
		m_position.Y < pos.Y + dimensions.Y * 0.5f && 
		m_position.Y > pos.Y - dimensions.Y * 0.5f)
	{
		return true;
	}

	return false;
}

void Camera2D::SetZoomLevel(float value)
{
	if (!Game::GetInstance()->GetIsLevelEditMode())
	{
		if (value > 1.0f)
		{
			value = 1.0f;
		}
	}
	
	if (value < 0.2f)
	{
		value = 0.2f;
	}

	mZoomPercent = value;

	D3DXMatrixOrthoLH(&m_projection, m_width * value, m_height * value, 0.0f, (std::numeric_limits<float>::max)());
}

void Camera2D::DoSmallShake()
{
	DoShake(5.0f, 0.10f);
}

void Camera2D::DoMediumShake()
{
	DoShake(15.0f, 0.30f);
}

void Camera2D::DoBigShake()
{
	DoShake(30.0f, 0.45f);
}

void Camera2D::DoShake(float intensity, float shakeDuration)
{
	if (mCurrentlyShaking)
	{
		return;
	}

	mCurrentShakeDuration = shakeDuration;
	mCurrentlyShaking = true;
	mShakeStartTime = Timing::Instance()->GetTotalTimeSeconds();
	mCurrentShakeIntensity = intensity;
}

void Camera2D::CheckBoundaryCollisions()
{
#if _DEBUG
	if (Game::GetInstance()->GetIsLevelEditMode())
	{
		// we need to manipulate the camera when in edit mode
		return;
	}
#endif

	// TODO: alot of these variables can be precalculated
	float bottom = Bottom();
	float top = Top();

	if (bottom <= mBoundsBottomRight.Y)
	{
		m_position.Y = mBoundsBottomRight.Y + (m_height * 0.5f);
	}
	else if (top >= mBoundsTopLeft.Y)
	{
		m_position.Y = mBoundsTopLeft.Y  - (m_height * 0.5f);
	}

	if (Left() <= mBoundsTopLeft.X)
	{
		m_position.X = mBoundsTopLeft.X + (m_width * 0.5f);
	}
	else if (Right() >= mBoundsBottomRight.X)
	{
		m_position.X = mBoundsBottomRight.X - (m_width * 0.5f);
	}
}

void Camera2D::CheckDebugCamerCommands()
{
	int movespeed = 40;
	// test
	if (GetAsyncKeyState(VK_RIGHT) < 0)
	{
		if (!UIManager::Instance()->IsObjectEditorDisplaying())
		{
			m_position.X += movespeed;

			if (Game::GetInstance()->GetIsLevelEditMode())
			{
				list<shared_ptr<GameObject> > & gameObjects = GameObjectManager::Instance()->GetGameObjectList();
				for (auto & obj : gameObjects)
				{
					if (obj->GetParallaxMultiplierX() != 1.0f ||
						obj->GetParallaxMultiplierY() != 1.0f)
					{
						obj->Update(1.0f);
					}
				}
			}
		}
	}
	else if (GetAsyncKeyState(VK_LEFT) < 0)
	{
		if (!UIManager::Instance()->IsObjectEditorDisplaying())
		{
			m_position.X -= movespeed;
			if (Game::GetInstance()->GetIsLevelEditMode())
			{
				list<shared_ptr<GameObject> > & gameObjects = GameObjectManager::Instance()->GetGameObjectList();
				for (auto & obj : gameObjects)
				{
					if (obj->GetParallaxMultiplierX() != 1.0f ||
						obj->GetParallaxMultiplierY() != 1.0f)
					{
						obj->Update(1.0f);
					}
				}
			}
		}
	}

	if (GetAsyncKeyState(VK_UP) < 0)
	{
		if (!UIManager::Instance()->IsObjectEditorDisplaying())
		{
			m_position.Y += movespeed;
			if (Game::GetInstance()->GetIsLevelEditMode())
			{
				list<shared_ptr<GameObject> > & gameObjects = GameObjectManager::Instance()->GetGameObjectList();
				for (auto & obj : gameObjects)
				{
					if (obj->GetParallaxMultiplierX() != 1.0f ||
						obj->GetParallaxMultiplierY() != 1.0f)
					{
						obj->Update(1.0f);
					}
				}
			}
		}
	}
	else if (GetAsyncKeyState(VK_DOWN) < 0 && GetAsyncKeyState(VK_CONTROL) >= 0)
	{
		if (!UIManager::Instance()->IsObjectEditorDisplaying())
		{
			m_position.Y -= movespeed;
			if (Game::GetInstance()->GetIsLevelEditMode())
			{
				list<shared_ptr<GameObject> > & gameObjects = GameObjectManager::Instance()->GetGameObjectList();
				for (auto & obj : gameObjects)
				{
					if (obj->GetParallaxMultiplierX() != 1.0f ||
						obj->GetParallaxMultiplierY() != 1.0f)
					{
						obj->Update(1.0f);
					}
				}
			}
		}
	}
}

