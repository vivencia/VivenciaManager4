#include "tristatetype.h"

void triStateType::fromInt ( const int state )
{
	if ( state < 0 )
		m_state = TRI_UNDEF;
	else if ( state == 0 )
		m_state = TRI_OFF;
	else
		m_state = TRI_ON;
}

TRI_STATE triStateType::toggleNext ()
{
	switch ( m_state )
	{
		case TRI_UNDEF:	m_state = TRI_OFF;		break;
		case TRI_OFF:	m_state = TRI_ON;		break;
		case TRI_ON:	m_state = TRI_UNDEF;	break;
	}
	return m_state;
}

TRI_STATE triStateType::togglePrev ()
{
	switch ( m_state )
	{
		case TRI_ON:	m_state = TRI_OFF;		break;
		case TRI_OFF:	m_state = TRI_UNDEF;	break;
		case TRI_UNDEF:	m_state = TRI_ON;		break;
	}
	return m_state;
}

TRI_STATE triStateType::toggleOnOff ()
{
	switch ( m_state )
	{
		case TRI_UNDEF:
		case TRI_OFF:
			m_state = TRI_ON;	break;
		case TRI_ON:
			m_state = TRI_OFF;	break;
	}
	return m_state;
}
