#include "precompiled.h"
#include "SolidLineStrip.h"
#include "DrawUtilities.h"
#include "Projectile.h"

SolidLineStrip::SolidLineStrip(float x, float y, DepthLayer depthLayer, float width, float height) :
	SolidMovingSprite(x, y, depthLayer, width, height),
	mHasHardRightEdge(false),
	mHasHardLeftEdge(false),
	mHardRightEdgeOffsetX(0.0f),
	mHardLeftEdgeOffsetX(0.0f),
	mCanDropDown(false)
{
	mIsSolidLine = true;
	mDrawable = false;
	mIsSolidLineStrip = true;
	m_drawAtNativeDimensions = false;
	m_applyGravity = false;
}

SolidLineStrip::~SolidLineStrip(void)
{
}

void SolidLineStrip::Scale(float xScale, float yScale, bool scalePosition)
{
	SolidMovingSprite::Scale(xScale, yScale, scalePosition);
}

void SolidLineStrip::Update(float delta)
{
	SolidMovingSprite::Update(delta);
}

void SolidLineStrip::Initialise()
{
	// update the base classes
	SolidMovingSprite::Initialise();

	// all solid line strips should be passive 
	m_passive = true;
}
void SolidLineStrip::XmlRead(TiXmlElement * element)
{
	// update base classes
	SolidMovingSprite::XmlRead(element);

	int numPoints = XmlUtilities::ReadAttributeAsInt(element, "points", "num_points");

	for (int i = 0; i < numPoints; ++i)
	{
		SolidLinePoint point;

		string xPrefix = "x_";
		string yPrefix = "y_";
		point.LocalPosition.X = XmlUtilities::ReadAttributeAsFloat(element, "points", (xPrefix + std::to_string(i)).c_str());
		point.LocalPosition.Y = XmlUtilities::ReadAttributeAsFloat(element, "points", (yPrefix + std::to_string(i)).c_str());
		mPoints.push_back(point);
	}

	CalculateLines();

	mHasHardRightEdge = XmlUtilities::ReadAttributeAsBool(element, "edge_properties", "has_right_hard_edge");
	mHasHardLeftEdge = XmlUtilities::ReadAttributeAsBool(element, "edge_properties", "has_left_hard_edge");
	mHardRightEdgeOffsetX = XmlUtilities::ReadAttributeAsFloat(element, "edge_properties", "right_hard_edge_offset_x");
	mHardLeftEdgeOffsetX = XmlUtilities::ReadAttributeAsFloat(element, "edge_properties", "left_hard_edge_offset_x");

	mCanDropDown = XmlUtilities::ReadAttributeAsBool(element, "edge_properties", "can_drop_down");
}

void SolidLineStrip::XmlWrite(TiXmlElement * element)
{
	SolidMovingSprite::XmlWrite(element);

	// path points
	TiXmlElement * points = new TiXmlElement("points"); 
	int pointsCount = 0;
	for (auto p : mPoints)
	{
		string attributeKeyX = "x_" + std::to_string(pointsCount);
		string attributeKeyY = "y_" + std::to_string(pointsCount);

		points->SetDoubleAttribute(attributeKeyX.c_str(), p.LocalPosition.X);
		points->SetDoubleAttribute(attributeKeyY.c_str(), p.LocalPosition.Y);
		++pointsCount;
	}
	points->SetAttribute("num_points", pointsCount);

	element->LinkEndChild(points);

	TiXmlElement * edgeProperties = new TiXmlElement("edge_properties");
	edgeProperties->SetAttribute("has_right_hard_edge", mHasHardRightEdge ? "true" : "false");
	edgeProperties->SetAttribute("has_left_hard_edge", mHasHardLeftEdge ? "true" : "false");
	edgeProperties->SetDoubleAttribute("right_hard_edge_offset_x", mHardRightEdgeOffsetX);
	edgeProperties->SetDoubleAttribute("left_hard_edge_offset_x", mHardLeftEdgeOffsetX);
	edgeProperties->SetAttribute("can_drop_down", mCanDropDown ? "true" : "false");
	element->LinkEndChild(edgeProperties);
}

void SolidLineStrip::LoadContent(ID3D10Device * graphicsdevice)
{
	SolidMovingSprite::LoadContent(graphicsdevice);
}

bool SolidLineStrip::OnCollision(SolidMovingSprite * object)
{
	if (object->IsButterfly() ||
		object->IsProjectile() ||
		object->IsCurrencyOrb())
	{
		return false;
	}

	if (mCanDropDown && object->IsCharacter())
	{
		// get the last solidline strip and uhdccb
		GAME_ASSERT((dynamic_cast<Character *>(object) != nullptr));
		Character * character = static_cast<Character *>(object);

		if (character->GetCurrentSolidLineDroppingDownThroughId() == ID())
		{
			return false;
		}
	}

	auto player = GameObjectManager::Instance()->GetPlayer();

	if (!object->IsPassive())
	{
		for (auto & l : mLines)
		{
			if (!BoxHitCheck(l, object))
			{
				if (!object->IsOnSolidlineThisFrame())
				{
					object->SetIsOnSolidLine(false, nullptr);
				}

				continue;
			}

			Vector2 intersectPoint;
			bool intersect = Intersect(l,
										Vector2(object->CollisionCentreX(), object->CollisionBottomLastFrame() + 30.0f),
										Vector2(object->CollisionCentreX(), object->CollisionBottom()), 
										intersectPoint);
			if (intersect)
			{
				if (object->VelocityY() <= 0.0f) // if not moving upwards (example: jumping)
				{
					float sinkValue = 5.0f;
					// Set 5.0f below the intersection point so the object remains connected
					object->SetY(intersectPoint.Y + (((object->CollisionDimensions().Y * 0.5f) + object->CollisionBoxOffset().Y) - sinkValue));

					if (!object->IsBouncable() || object->VelocityY() >= -2.0f)
					{
						object->StopYAccelerating();
					}
					else 
					{
						if (object->VelocityY() < -2.0f)
						{
							object->SetVelocityY(object->VelocityY() *  (-object->GetBounceDampening()));
							object->SetY(object->Y() + (sinkValue + 1.0f));
						}
					}

					if (!object->IsDebris())
					{
						object->SetIsCollidingOnTopOfObject(true);

						object->SetIsOnSolidLine(true, this);
					}

					if (object->IsCharacter())
					{
						GAME_ASSERT((dynamic_cast<Character *>(object) != nullptr));
						Character * character = static_cast<Character *>(object);

						character->SetCurrentSolidLineDroppingDownThroughId(-1);
					}

					if (object == player)
					{
						if (player->DirectionX() > 0)
						{
							player->SetCurrentSolidLineDirection(Vector2(l.LineDirection.X * player->DirectionX(), l.LineDirection.Y * -player->DirectionY()));
						}
						else
						{
							player->SetCurrentSolidLineDirection(Vector2(l.LineDirection.X * player->DirectionX(), l.LineDirection.Y * player->DirectionY()));
						}
					}

					return true;
				}
				break;
			}
			else
			{
				if (!object->IsOnSolidlineThisFrame())
				{
					object->SetIsOnSolidLine(false, nullptr);
				}
			}
		}
	}

	return true;
}

void SolidLineStrip::DebugDraw(ID3D10Device *  device)
{
	SolidMovingSprite::DebugDraw(device);

	unsigned count = 0;
	for (auto & l : mLines)
	{
		DrawUtilities::DrawLine(l.StartPoint.WorldPosition, l.EndPoint.WorldPosition);

		if (count == 0 && GetHasHardLeftEdge())
		{
			DrawUtilities::DrawLine(l.StartPoint.WorldPosition, Vector2(l.StartPoint.WorldPosition.X - 10.0f, l.StartPoint.WorldPosition.Y + 50.0f));
		}
		if (count == (mLines.size() - 1) && GetHasHardRightEdge())
		{
			DrawUtilities::DrawLine(l.EndPoint.WorldPosition, Vector2(l.EndPoint.WorldPosition.X + 10.0f, l.EndPoint.WorldPosition.Y + 50.0f));
		}

		if (mCanDropDown)
		{
			DrawUtilities::DrawLine(l.EndPoint.WorldPosition, Vector2(l.EndPoint.WorldPosition.X, l.EndPoint.WorldPosition.Y - 70.0f));
		}

		if (count == 0)
		{
			DrawUtilities::DrawTexture(Vector3(l.StartPoint.WorldPosition.X, l.StartPoint.WorldPosition.Y, 3.0f),
										Vector2(50.0f, 50.0f),
										"Media\\editor\\circle.png");
		}

		DrawUtilities::DrawTexture(Vector3(l.EndPoint.WorldPosition.X, l.EndPoint.WorldPosition.Y, 3.0f),
									Vector2(50.0f, 50.0f),
									"Media\\editor\\circle.png");
		++count;
	}
}

void SolidLineStrip::CalculateLines()
{
	mLines.clear();

	if (mPoints.size() < 2)
	{
		GAME_ASSERT(false);
		return;
	}

	int count = 0;
	float maxX = 0.0f;
	float minX = 0.0f;
	float maxY = 0.0f;
	float minY = 0.0f;

	for (auto & p : mPoints)
	{
		if (count == 0)
		{
			maxX = p.LocalPosition.X;
			minX = p.LocalPosition.X;
			maxY = p.LocalPosition.Y;
			minY = p.LocalPosition.Y;
			++count;
			continue;
		}

		if (p.LocalPosition.X < minX)
		{
			minX = p.LocalPosition.X;
		}
		if (p.LocalPosition.X > maxX)
		{
			maxX = p.LocalPosition.X;
		}
		if (p.LocalPosition.Y > maxY)
		{
			maxY = p.LocalPosition.Y;
		}
		if (p.LocalPosition.Y < minY)
		{
			minY = p.LocalPosition.Y;
		}

		SolidLinePoint startPoint = mPoints[count -1];
		SolidLinePoint endPoint = p;
		
		startPoint.WorldPosition = Vector2(m_position.X + startPoint.LocalPosition.X, m_position.Y + startPoint.LocalPosition.Y);
		endPoint.WorldPosition = Vector2(m_position.X + endPoint.LocalPosition.X, m_position.Y + endPoint.LocalPosition.Y);

		// assign back
		mPoints[count -1].WorldPosition = startPoint.WorldPosition;
		p.WorldPosition = endPoint.WorldPosition;

		SolidLine solidLine;
		solidLine.StartPoint = startPoint;
		solidLine.EndPoint = endPoint;

		Vector2 lineDiff = endPoint.LocalPosition - startPoint.LocalPosition;

		solidLine.Length = lineDiff.Length();

		if (solidLine.Length <= 0)
		{
			GAME_ASSERT(false);
			return;
		}

		solidLine.LineDirection = Vector2(lineDiff.X / solidLine.Length, lineDiff.Y / solidLine.Length);
		solidLine.Normal = Vector2(-solidLine.LineDirection.Y, solidLine.LineDirection.X);

		solidLine.BoundingBox.X = std::abs(endPoint.LocalPosition.X - startPoint.LocalPosition.X);
		solidLine.BoundingBox.Y = std::abs(endPoint.LocalPosition.Y - startPoint.LocalPosition.Y);

		solidLine.MidPointWorld = solidLine.StartPoint.WorldPosition + (solidLine.LineDirection * (solidLine.Length * 0.5f));

		mLines.push_back(solidLine);

		++count;
	}

	m_collisionBoxDimensions.X = std::abs(maxX - minX) + 50.0f;
	m_collisionBoxDimensions.Y = std::abs(maxY - minY) + 100.0f;

	if (m_collisionBoxDimensions.Y < 400.0f)
	{
		m_collisionBoxDimensions.Y = 400.0f;
	}
	if (m_collisionBoxDimensions.X < 100.0f)
	{
		m_collisionBoxDimensions.X = 100.0f;
	}

	mCollisionBoxOffset.X = (maxX + minX) * 0.5f;
	mCollisionBoxOffset.Y = (maxY + minY) * 0.5f;

	m_dimensions = Vector2(m_collisionBoxDimensions.X + std::abs(mCollisionBoxOffset.X)  * 2.0f, 
							m_collisionBoxDimensions.Y + std::abs(mCollisionBoxOffset.Y) * 2.0f);
}

bool SolidLineStrip::Intersect(SolidLine & solidLine, Vector2 & otherStart, Vector2 & otherEnd, Vector2 & intersectPointOut)
{
	// Calculate matrix determinants
	float det1 = (solidLine.StartPoint.WorldPosition.X * solidLine.EndPoint.WorldPosition.Y) - 
						(solidLine.StartPoint.WorldPosition.Y * solidLine.EndPoint.WorldPosition.X);
	float det2 = (otherStart.X * otherEnd.Y) - (otherStart.Y * otherEnd.X);
	float det3 = ((solidLine.StartPoint.WorldPosition.X - solidLine.EndPoint.WorldPosition.X) * (otherStart.Y - otherEnd.Y)) -
						((solidLine.StartPoint.WorldPosition.Y - solidLine.EndPoint.WorldPosition.Y) * (otherStart.X - otherEnd.X));

	// If third determinant is close to zero then abort: two lines are nearly parallel:
	if (det3 >= -0.00000001f && det3 <= 0.00000001f) return false;

	// Otherwise calculate the point of intersection:
	intersectPointOut.X = (det1 * (otherStart.X - otherEnd.X)) - ((solidLine.StartPoint.WorldPosition.X - solidLine.EndPoint.WorldPosition.X) * det2);
	intersectPointOut.X /= det3;
	intersectPointOut.Y = (det1 * (otherStart.Y - otherEnd.Y)) - ((solidLine.StartPoint.WorldPosition.Y - solidLine.EndPoint.WorldPosition.Y) * det2);
	intersectPointOut.Y /= det3;

	// Make sure the point is along both lines: get it relative to the start point of both lines
	Vector2 r1 = intersectPointOut - solidLine.StartPoint.WorldPosition;
	Vector2 r2 = intersectPointOut - otherStart;

	Vector2 otherLineDistance = otherEnd - otherStart;
	Vector2 otherLineDirection = otherLineDistance;
	otherLineDirection.Normalise(); // TODO: optimisation opportunity

	// Do a dot product with both line directions to see if it is past the end of either of the two lines:
	float dot1 = r1.Dot(solidLine.LineDirection);
	float dot2 = r2.Dot(otherLineDirection);

	// If either dot is negative then the point is past the beginning of one of the lines:
	if (dot1 < 0 || dot2 < 0)
	{
		return false;
	}

	// If either dot exceeds the length of the line then point is past the end of the line:
	if (dot1 > solidLine.Length)
	{
		return false;
	}

	if (dot2 > otherLineDistance.Length()) // TODO: optimisation opportunity
	{
		return false;
	}

	return true;
}

bool SolidLineStrip::BoxHitCheck(SolidLine & solidLine, SolidMovingSprite * object)
{
	return Utilities::IsSolidSpriteInRectangle(object,
												solidLine.MidPointWorld.X,
												solidLine.MidPointWorld.Y,
												solidLine.BoundingBox.X,
												solidLine.BoundingBox.Y);
}

void SolidLineStrip::RecalculateLines(std::vector<SolidLinePoint> & points)
{
	mPoints.clear();
	mLines.clear();

	mPoints = points;
	CalculateLines();
}

bool SolidLineStrip::GetProjectileCollisionData(Projectile * projectile, Vector2 & position, unsigned int & lineIndex)
{
	Vector2 projectileRayStart = projectile->GetLastFrameCollisionRayStart();
	Vector2 projectileRayEnd = projectile->GetCollisionRayEnd();

	unsigned int count = 0;
	for (auto & l : mLines)
	{
		if (!BoxHitCheck(l, projectile))
		{
			projectile->SetIsOnSolidLine(false, nullptr);
			continue;
		}

		Vector2 intersectPoint;
		bool intersect = Intersect(l,
									projectileRayStart,
									projectileRayEnd,
									intersectPoint);
		if (intersect)
		{
			projectile->SetIsOnSolidLine(true, this);

			position.X = m_position.X - projectileRayEnd.X;
			position.Y = m_position.Y - projectileRayEnd.Y;

			lineIndex = count;

			return true;	
		}
		else
		{
			projectile->SetIsOnSolidLine(false, nullptr);
		}

		++count;
	}

	return false;
}

bool SolidLineStrip::GetBombProjectileCollisionData(Projectile * projectile, Vector2 & position)
{
	Vector2 projectileRayStart = projectile->GetCollisionRayStart();
	Vector2 projectileRayEnd = projectile->GetCollisionRayEnd();

	// for the bomb projectile, also get the down vector as it will bounce off the ground

	for (auto & l : mLines)
	{
		if (!BoxHitCheck(l, projectile))
		{
			projectile->SetIsOnSolidLine(false, nullptr);
			continue;
		}

		Vector2 intersectPoint;
		bool intersect = Intersect(l,
			projectileRayStart,
			projectileRayEnd,
			intersectPoint);

		if (!intersect)
		{
			// The current ray check may fail but check the last frames ray to make sure we didn't skip the collision
			Vector2 projectileRayStart = projectile->GetLastFrameCollisionRayStart();

			intersect = Intersect(l,
									projectileRayStart,
									projectileRayEnd,
									intersectPoint);
		}
		if (!intersect)
		{
			// check the down vector

			intersect = Intersect(l,
									Vector2(projectile->X(), projectile->CollisionTop()),
									Vector2(projectile->X(), projectile->CollisionBottom()),
									intersectPoint);
			
		}
		if (intersect)
		{
			projectile->SetIsOnSolidLine(true, this);

			position.X = m_position.X - projectileRayEnd.X;
			position.Y = m_position.Y - projectileRayEnd.Y;

			return true;
		}
		else
		{
			projectile->SetIsOnSolidLine(false, nullptr);
		}
	}

	return false;
}
Vector2 SolidLineStrip::GetRightMostPoint() const
{
	unsigned int linesSize = (unsigned int)mLines.size();
	GAME_ASSERT(linesSize > 0);

	if (linesSize == 0)
	{
		return Vector2(0, 0);
	}

	SolidLine solidLine = mLines[linesSize - 1];
	return solidLine.EndPoint.WorldPosition;
}

Vector2 SolidLineStrip::GetLeftMostPoint() const
{
	unsigned int linesSize = (unsigned int)mLines.size();
	GAME_ASSERT(linesSize > 0);

	if (linesSize == 0)
	{
		return Vector2(0, 0);
	}

	SolidLine solidLine = mLines[0];
	return solidLine.StartPoint.WorldPosition; 
}

const Vector2 & SolidLineStrip::GetNormalForLineIndex(unsigned int lineIndex)
{
	return mLines[lineIndex].Normal;
}

