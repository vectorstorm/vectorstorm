/*
 *  UT_Tween.h
 *  MMORPG
 *
 *  Created by Trevor Powell on 21/09/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef UT_TWEEN_H
#define UT_TWEEN_H

template <typename T>
class vsTween
{
	T		m_start;
	T		m_end;

	T		m_current;
	float	m_tweenTimer;
	float	m_tweenDuration;
	bool	m_tweening;
	bool	m_smoothTween;

public:

	vsTween<T>(const T &value, bool smooth = true)
	{
		m_smoothTween = smooth;
		SetValue(value);
	}

	void SetValue(const T &value)
	{
		m_current = value;
		m_tweening = false;
	}

	bool		IsTweening()
	{
		return m_tweening;
	}

	const T &	GetValue()
	{
		return m_current;
	}

	const T &	GetTarget()
	{
		return m_end;
	}

	void TweenTo(const T &value, float time)
	{
		if ( value == m_current )
		{
			m_tweening = false;
		}
		else if ( time == 0.f )
		{
			m_start = m_end = m_current = value;
			m_tweening = false;
		}
		else if ( !m_tweening || value != m_end )
		{
			// don't re-start tweening to a value we were already tweening to.
			m_start = m_current;
			m_end = value;
			m_tweenDuration = time;
			m_tweenTimer = 0.f;
			m_tweening = true;
		}
	}

	void Update(float timeStep)
	{
		if ( m_tweening )
		{
			m_tweenTimer += timeStep;
			if ( m_tweenTimer > m_tweenDuration )
			{
				m_tweening = false;
				m_current = m_end;
				return;
			}
			float fraction = m_tweenTimer / m_tweenDuration;
			if ( m_smoothTween )
			{
				fraction = (3.0f * fraction * fraction) - (2.0f * fraction * fraction * fraction);
			}
			m_current = vsInterpolate( fraction, m_start, m_end );
		}
	}
};

#include "Math/VS_Spline.h"

// vsTweenSpline3D works for vsVector3D types.  Same interface as vsTween, but
// it uses a spline for its backing.  This means it handles changing
// destinations a lot more smoothly!
class vsTweenSpline3D
{
	vsSpline3D m_spline;
	vsVector3D m_current;
	float	m_tweenTimer;
	float	m_tweenDuration;
	bool m_tweening;
public:
	vsTweenSpline3D( const vsVector3D& ini ):
		m_spline(ini, vsVector3D::Zero, ini, vsVector3D::Zero),
		m_tweenTimer(0.f),
		m_tweenDuration(1.f),
		m_tweening(false)
	{
	}

	void SetValue(const vsVector3D &value)
	{
		m_spline.Set(value, vsVector3D::Zero, value, vsVector3D::Zero);
		m_current = value;
		m_tweening = false;
	}

	bool		IsTweening()
	{
		return m_tweening;
	}

	const vsVector3D&	GetValue()
	{
		return m_current;
	}

	vsVector3D GetTarget()
	{
		return m_spline.PositionAtTime( 1.f );
	}

	void TweenTo(const vsVector3D&value, float time)
	{
		if ( value == m_current )
		{
			m_tweening = false;
		}
		else if ( time == 0.f )
		{
			SetValue(value);
		}
		else
		{
			float timeNow = m_tweenTimer / m_tweenDuration;
			vsVector3D velocityNow = m_tweening ?
				m_spline.VelocityAtTime(timeNow) : vsVector3D::Zero;
			vsVector3D positionNow = m_spline.PositionAtTime(timeNow);

			m_spline.Set( positionNow, velocityNow, value, vsVector3D::Zero );
			m_tweenTimer = 0.f;
			m_tweenDuration = time;
			m_tweening = true;
		}
	}

	void Update(float timeStep)
	{
		if ( m_tweening )
		{
			m_tweenTimer += timeStep;
			if ( m_tweenTimer > m_tweenDuration )
			{
				m_tweening = false;
				m_current = GetTarget();
				return;
			}
			float time = m_tweenTimer / m_tweenDuration;
			m_current = m_spline.PositionAtTime(time);
		}
	}
};

#endif // UT_TWEEN_H

