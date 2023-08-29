#ifndef TRISTATETYPE_H
#define TRISTATETYPE_H

#include <utility>

typedef enum { TRI_UNDEF = -1, TRI_ON = 1, TRI_OFF = 0 } TRI_STATE;

class triStateType
{

inline void tristate_swap ( triStateType& tri1, triStateType& tri2 )
	{ using std::swap; swap ( tri1.m_state, tri2.m_state ); }

public:
	inline constexpr triStateType ()
		: m_state ( TRI_UNDEF )
	{}

	inline triStateType ( const int initial_state )
	{
		fromInt ( initial_state );
	}

	inline constexpr triStateType ( const TRI_STATE initial_state )
		: m_state ( initial_state )
	{}

	inline constexpr triStateType ( const triStateType& other )
		: m_state ( other.m_state )
	{}

	inline triStateType ( triStateType&& other )
		: triStateType () 
	{ 
		tristate_swap ( *this, other );
	}
	
	inline const triStateType& operator= ( const bool state )
	{
		fromInt ( static_cast<int> ( state ) );
		return *this;
	}

	inline const triStateType& operator= ( const int state )
	{
		fromInt ( state );
		return *this;
	}

	inline const triStateType& operator= ( const TRI_STATE state )
	{
		m_state = state;
		return *this;
	}

	inline const triStateType& operator= ( triStateType other )
	{
		tristate_swap ( *this, other );
		return *this;
	}
	
	/*inline const triStateType& operator= ( const triStateType& other )
	{
		m_state = other.m_state;
		return *this;
	}*/

	inline constexpr bool operator== ( const int state ) const
	{
		return ( static_cast<int> ( m_state ) == state );
	}

	inline constexpr bool operator== ( const TRI_STATE state ) const
	{
		return ( m_state == state );
	}

	inline constexpr bool operator== ( const triStateType& other ) const
	{
		return ( m_state == other.m_state );
	}

	inline bool operator!= ( const TRI_STATE state ) const
	{
		return ( m_state != state );
	}

	inline void setState ( const TRI_STATE state )
	{
		m_state = state;
	}

	inline void setState ( const int state )
	{
		fromInt ( state );
	}

	TRI_STATE toggleNext ();
	TRI_STATE togglePrev ();
	TRI_STATE toggleOnOff ();

	inline constexpr TRI_STATE state () const
	{
		return m_state;
	}
	
	inline constexpr int toInt () const
	{
		return static_cast<int> ( m_state );
	}

	inline void setOn ()
	{
		setState ( TRI_ON );
	}
	
	inline void setOff ()
	{
		setState ( TRI_OFF );
	}
	
	inline void setUndefined ()
	{
		setState ( TRI_UNDEF );
	}

	inline constexpr bool isOn () const
	{
		return m_state == TRI_ON;
	}
	
	inline constexpr bool isOff () const
	{
		return m_state == TRI_OFF;
	}
	
	inline constexpr bool isUndefined () const
	{
		return m_state == TRI_UNDEF;
	}

	inline constexpr bool isTrue () const
	{
		return m_state == TRI_ON;
	}
	
	inline constexpr bool isFalse () const
	{
		return m_state == TRI_OFF;
	}
	
	inline constexpr bool isNeither () const
	{
		return m_state == TRI_UNDEF;
	}

private:
	void fromInt ( const int state );

	TRI_STATE m_state;
};

#endif // TRISTATETYPE_H
