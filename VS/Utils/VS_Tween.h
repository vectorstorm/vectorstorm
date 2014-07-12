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

#endif // UT_TWEEN_H

