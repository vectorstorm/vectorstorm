/*
 *  COL_System.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_CollisionSystem.h"
#include "VS_PhysicsSprite.h"

#include "VS_DisableDebugNew.h"
#include <Box2D/Box2D.h>
#include "VS_EnableDebugNew.h"

#include "VS_Vector.h"

vsCollisionSystem *		vsCollisionSystem::s_instance = NULL;

#define DEFAULT_WORLD_SIZE (10000.f)

float vsCollisionSystem::s_left = -DEFAULT_WORLD_SIZE;
float vsCollisionSystem::s_right = DEFAULT_WORLD_SIZE;
float vsCollisionSystem::s_top = -DEFAULT_WORLD_SIZE;
float vsCollisionSystem::s_bottom = DEFAULT_WORLD_SIZE;
bool vsCollisionSystem::s_border = true;

class vsCollisionSystemDestructionListener : public b2DestructionListener
	{
		virtual void SayGoodbye(b2Joint* joint)
		{
			vsAssert(!joint, "Implicit destruction of joints not supported!");
		}

		virtual void SayGoodbye(b2Fixture* fixture)
		{
			UNUSED(fixture);
		}
	};

/*class vsCollisionSystemBoundaryListener : public b2BoundaryListener
	{
		virtual void Violation(b2Body* body)
		{
			if ( body->GetUserData() )
			{
				vsCollisionObject *o = (vsCollisionObject *)body->GetUserData();

				o->SetDestroyed();
			}
		}

	};*/

class vsCollisionSystemContactListener : public b2ContactListener
	{
		virtual void BeginContact(b2Contact* point)
		{
			b2Vec2 vec = point->GetManifold()->localPoint;
			vsVector2D where( vec.x, vec.y );

			b2Body *bOne = point->GetFixtureA()->GetBody();
			b2Body *bTwo = point->GetFixtureB()->GetBody();

			vsCollisionObject *cOne = NULL;
			vsCollisionObject *cTwo = NULL;

			if ( bOne )
				cOne = (vsCollisionObject *)bOne->GetUserData();
			if ( bTwo )
				cTwo = (vsCollisionObject *)bTwo->GetUserData();

			if ( cOne )
			{
				cOne->NotifyCollision(cTwo, where);
			}
			if ( cTwo )
			{
				cTwo->NotifyCollision(cOne, where);
			}
		}
	};

vsCollisionSystem::vsCollisionSystem()
{
	s_instance = this;
}

vsCollisionSystem::~vsCollisionSystem()
{
	s_instance = NULL;
}

void
vsCollisionSystem::Init()
{
	m_timeBucket = 0.f;

	b2AABB worldAABB;
    worldAABB.lowerBound.Set(s_left*2.0f, s_top*2.0f);
    worldAABB.upperBound.Set(s_right*2.0f, s_bottom*2.0f);

	/*float width = s_right - s_left;
	float height = s_bottom - s_top;
	float averageX = (s_right+s_left)*0.5f;
	float averageY = (s_bottom+s_top)*0.5f;*/

	b2Vec2 gravity(0.0f, 0.0f);
    //bool doSleep = true;

	m_boundaryListener = NULL;//new vsCollisionSystemBoundaryListener;
	m_destructionListener = new vsCollisionSystemDestructionListener;
	m_contactListener = new vsCollisionSystemContactListener;
	m_world = new b2World(gravity);
	//m_world->SetListener(m_boundaryListener);
	m_world->SetDestructionListener(m_destructionListener);
	m_world->SetContactListener(m_contactListener);

	/*if ( s_border )
	{
		b2PolygonDef horizontalGroundBoxDef;
		horizontalGroundBoxDef.SetAsBox(width*.5f, 10.f);
		horizontalGroundBoxDef.density = 0.0f;
		horizontalGroundBoxDef.categoryBits = 0xffff;

		b2PolygonDef verticalGroundBoxDef;
		verticalGroundBoxDef.SetAsBox(10.f, height*.5f);
		verticalGroundBoxDef.density = 0.0f;
		verticalGroundBoxDef.categoryBits = 0xffff;

		b2BodyDef bottomGroundBodyDef;
		bottomGroundBodyDef.position.Set(averageX, s_bottom+10.f);
		//bottomGroundBodyDef.AddShape( &horizontalGroundBoxDef );

		b2BodyDef topGroundBodyDef;
		topGroundBodyDef.position.Set(averageX, s_top-10.f);
		//topGroundBodyDef.AddShape( &horizontalGroundBoxDef );

		b2BodyDef leftGroundBodyDef;
		leftGroundBodyDef.position.Set(s_left-10.f, averageY);
		//leftGroundBodyDef.AddShape( &verticalGroundBoxDef );

		b2BodyDef rightGroundBodyDef;
		rightGroundBodyDef.position.Set(s_right+10.f, averageY);
		//rightGroundBodyDef.AddShape( &verticalGroundBoxDef );

		b2Body* b = m_world->CreateStaticBody(&bottomGroundBodyDef);
		b2Body* t = m_world->CreateStaticBody(&topGroundBodyDef);
		b2Body* l = m_world->CreateStaticBody(&leftGroundBodyDef);
		b2Body* r = m_world->CreateStaticBody(&rightGroundBodyDef);

		b->CreateShape( &horizontalGroundBoxDef );
		t->CreateShape( &horizontalGroundBoxDef );
		l->CreateShape( &verticalGroundBoxDef );
		r->CreateShape( &verticalGroundBoxDef );
	}*/

	m_timeBucket = 0.f;
}

void
vsCollisionSystem::Deinit()
{
	vsDelete(m_world);
	vsDelete(m_destructionListener);
	//vsDelete(m_boundaryListener);
	vsDelete(m_contactListener);

	s_left = -DEFAULT_WORLD_SIZE;
	s_right = DEFAULT_WORLD_SIZE;
	s_top = -DEFAULT_WORLD_SIZE;
	s_bottom = DEFAULT_WORLD_SIZE;
	s_border = true;
}

void
vsCollisionSystem::SetGravity( const vsVector2D &gravity )
{
	b2Vec2 g(gravity.x,gravity.y);

	m_world->SetGravity( g );
}
/*
void
vsCollisionSystem::RegisterObject( vsCollisionObject *object )
{
	object->RegisterObject();
}

void
vsCollisionSystem::DeregisterObject( vsCollisionObject *object )
{
	object->DeregisterObject();
}
*/

void
vsCollisionSystem::Update(float timeStep)
{
	const float c_stepIncrement = ( 1.0f / 60.0f );
	const int32_t iterations = 10;

	m_timeBucket += timeStep;

	if ( m_timeBucket > 1.0f )
		m_timeBucket = c_stepIncrement;

	//while ( m_timeBucket >= c_stepIncrement )
	//{
		m_world->Step( c_stepIncrement, iterations, iterations );
		m_timeBucket -= c_stepIncrement;
	//}
}

class vsCollisionSystemQueryCallback: public b2QueryCallback
{
	vsCollisionObject **m_resultArray;
	int m_maxResults;
	int m_results;
public:
	vsCollisionSystemQueryCallback(vsCollisionObject **resultArray, int maxResults):
		m_resultArray(resultArray),
		m_maxResults(maxResults),
		m_results(0)
	{
	}

	virtual bool ReportFixture(b2Fixture* fixture)
	{
		if ( m_results >= m_maxResults )
		{
			return false;
		}

		b2Body *body = fixture->GetBody();
		if ( body->GetUserData() )
		{
			vsCollisionObject *o = (vsCollisionObject *)body->GetUserData();

			for ( int j = 0; j < m_results ; j++ )
			{
				if ( m_resultArray[j] == o )
				{
					o = NULL;
					break;
				}
			}

			if ( o )
			{
				m_resultArray[m_results ++] = o;
			}
		}

		m_results++;

		if ( m_results >= m_maxResults )
		{
			return false;
		}
		return true;
	}

	int GetResultsFound() { return m_results; }
};

int
vsCollisionSystem::FindColObjectsUnder( const vsVector2D &where, float radius, vsCollisionObject **resultArray, int maxResults )
{
    b2AABB aabb;
    aabb.lowerBound.Set(where.x-radius, where.y-radius);
    aabb.upperBound.Set(where.x+radius, where.y+radius);

	vsCollisionSystemQueryCallback callback(resultArray, maxResults);

	m_world->QueryAABB( &callback, aabb);

	return callback.GetResultsFound();
}

