#ifndef FAST_LIBRARY_FUNCTIONS_H
#define FAST_LIBRARY_FUNCTIONS_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>

#include <functional>

namespace VM_LIBRARY_FUNCS
{
	inline void copyToClipboard ( const QString& str )
	{
		QApplication::clipboard ()->setText ( str, QClipboard::Clipboard );
	}
}

template <typename T>
inline void setBit ( T& __restrict var, const unsigned char bit )
{
	if ( ( bit - 1 ) >= 0 )
		var |= ( 2 << ( bit - 1 ) );
	else
		var |= 1;
}

template <typename T>
inline void unSetBit ( T& __restrict var, const unsigned char bit )
{
	if ( ( bit - 1 ) >= 0 )
		var &= ~ ( 2 << ( bit - 1 ) );
	else
		var &= ~ 1;
}

template <typename T>
inline bool isBitSet ( const T& __restrict var, const unsigned char bit )
{
	if ( ( bit - 1 ) >= 0 )
		return static_cast<bool> ( var & ( 2 << ( bit - 1 ) ) );
	else
		return static_cast<bool> ( var & 1 );
}

template <typename T>
inline void setAllBits ( T& __restrict var )
{
	var = static_cast<T> ( 0xFFFFFFFF ); // all bits set to 1
}

template <typename T>
inline bool allBitsSet ( T& __restrict var )
{
	return ( var == static_cast<T> ( 0xFFFFFFFF ) );
}

template <typename T>
int insertStringIntoContainer ( const T& list, const QString& text,
								const std::function<QString( const int i)>& get_func,
								const std::function<void( const int i, const QString& str)>& insert_func,
								const bool b_insert_if_exists = false )
{
	if ( text.isEmpty () )
		return -1;

	QString str;
	int x ( 0 );
	int i ( 0 );
	const uint n_list_items ( static_cast<uint>( list.count () ) );

	for ( ; static_cast<uint>( i ) < n_list_items; ++i )
	{
		str = get_func ( i );
		// Insert item alphabetically
		for ( x = 0; x < text.count (); ++x )
		{
			if ( x >= str.count () )
			{
				if ( x == text.count () && !b_insert_if_exists )
					return -1;
				else
					break;
			}
			if ( text.at ( x ).toLower () > str.at ( x ).toLower () )
			{
				break;
			}
			else if ( text.at ( x ).toLower () == str.at ( x ).toLower () )
			{
				continue;
			}
			else
			{
				insert_func ( i, text );
				return i;
			}
		}
	}
	insert_func ( i, text ); // append
	return n_list_items;
}

#endif // FAST_LIBRARY_FUNCTIONS_H
