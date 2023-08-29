#ifndef HEAPMANAGER_H
#define HEAPMANAGER_H

#include "vmlist.h"

template <typename T>
bool deletePointer ( TIsPointerType*, T* ptr )
{
	if ( ptr != nullptr ) {
		delete ptr;
		return true;
	}
	return false;
}

template <typename T>
bool deletePointerArray ( TIsPointerType*, T* ptr )
{
	if ( ptr != nullptr ) {
		delete [] ptr;
		return true;
	}
	return false;
}

template <typename T>
bool deletePointer ( TIsNotPointerType*, T* ) {
	return false;
}

template <typename T>
bool deletePointerArray ( TIsNotPointerType*, T* ) {
	return false;
}

template <typename T>
inline void heap_del ( T& x )
{
	typename vmList<T>::template IsPointer<T>::Result r;
	deletePointer ( &r, x );
	x = nullptr;
}

template <typename T>
inline void heap_dellarr ( T& x )
{
	typename vmList<T>::template IsPointer<T>::Result r;
	if ( deletePointerArray ( &r, x ) )
		x = nullptr;
}

template <typename T>
class heapManager
{

public:
	void register_use ( const T* ptr, const size_t size = 1 );
	void unregister_use ( T* ptr );

	T* exists ( const T* ptr ) const;
	T* at ( const uint pos ) const;
	inline uint count () const
	{
		return ptr_use.count ();
	}

	heapManager ();
	~heapManager ();

private:
	struct ptr_info {
		T* ptr;
		size_t m_size;
		int use_count;

		ptr_info () : ptr ( nullptr ), m_size ( 0 ), use_count ( 0 ) {}
	};

	pointersList<ptr_info*> ptr_use;
};

template<typename T>
inline heapManager<T>::heapManager ()
	: ptr_use ( 5 )
{}

template<typename T>
heapManager<T>::~heapManager ()
{
	if ( ptr_use.last () ) {
		while ( ptr_use.current () ) {
			ptr_info* p_info = ptr_use.current ();
			if ( p_info->m_size == 1 )
				heap_del ( p_info->ptr );
			else
				heap_dellarr ( p_info->ptr );
			ptr_use.prev ();
		}
		ptr_use.clear ( false );
	}
}

template <typename T>
void heapManager<T>::register_use ( const T* ptr, const size_t size )
{
	for ( uint i ( 0 ); i < ptr_use.count (); ++i )
	{
		if ( ptr_use.at ( i )->ptr == ptr )
		{
			++( ptr_use.at ( i )->use_count );
			return;
		}
	}
	ptr_info* p_info ( new ptr_info );
	p_info->m_size = size;
	p_info->ptr = const_cast<T*> ( ptr );
	p_info->use_count = 1;
	ptr_use.append ( p_info );
}

template <typename T>
void heapManager<T>::unregister_use ( T* ptr )
{
	for ( uint i ( 0 ); i < ptr_use.count (); ++i )
	{
		if ( ptr_use.at ( i )->ptr == ptr )
		{
			ptr_info* p_info = ptr_use.at ( i );
			--( p_info->use_count );
			if ( p_info->use_count == 0 )
			{
				if ( p_info->m_size == 1 )
					heap_del ( p_info->ptr );
				else
					heap_dellarr ( p_info->ptr );
				ptr_use.remove ( i );
			}
		}
	}
}

template <typename T>
T* heapManager<T>::exists ( const T* ptr ) const
{
	for ( uint i ( 0 ); i < ptr_use.count (); ++i )
	{
		if ( ptr_use.at ( i )->ptr == ptr )
			return ptr;
	}
	return nullptr;
}

template <typename T>
inline T* heapManager<T>::at ( const uint pos ) const
{
	return ptr_use.at ( pos );
}

#endif // HEAPMANAGER_H
