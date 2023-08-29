#ifndef vmList_H
#define vmList_H

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <type_traits>
#include <algorithm>

//using namespace std;

typedef unsigned int uint;

typedef char TIsPointerType;
typedef char TIsNotPointerType[2];
typedef char TIsFuncPointerType[4];

template <typename T>
class vmList
{

	template <typename TT> friend class pointersList;
	template <typename TT> friend class podList;

public:

	static void vmList_swap ( vmList<T>& list1, vmList<T>& list2 );
	
	template<typename X>
	struct IsPointer
	{
		typedef TIsNotPointerType Result;
	};

	template<typename X>
	struct IsPointer<X*>
	{
		typedef TIsPointerType Result;
	};

	template<typename X>
	struct IsPointer<X (*) ()>
	{
		typedef TIsFuncPointerType Result;
	};

	vmList ();
	explicit vmList ( const T& default_value, const uint n_prealloc = 10 );
	vmList ( const vmList<T>& other );
	const vmList<T>& operator= ( vmList<T> other );

	inline vmList<T> ( vmList<T>&& other ) : vmList<T> ()
	{
		vmList_swap ( *this, other );
	}
	
	inline virtual ~vmList () { clear (); }

	int
		contains ( const T&, int from_pos = 0 ) const,
		insert ( const uint pos, const T& item ), //returns pos
		replace ( const uint pos, const T& item ), //returns pos
		removeOne ( const T& item, int from_pos = 0, const bool delete_item = false ), //removes an item from pos
		remove ( const int = 0, const bool delete_item = false ); //returns nItems

	static void
		deletePointer ( TIsNotPointerType*, const uint pos ),
		deletePointer ( TIsFuncPointerType*, const uint pos );
	
	void
		clear ( const uint from, const uint to, const bool keep_memory = false, const bool delete_items = false ),
		setAutoDeleteItem ( const bool bauto_del ),
		deletePointer ( TIsPointerType*, const uint pos ),
		resize ( const uint n ),
		setPreAllocNumber ( const uint n ),
		setDefaultValue ( const T& default_value ),
		setCurrent ( const int pos ) const,
		setCurrent ( const uint pos ) const,
		setIsPointer ( const bool b_ispointer );

	T
		&operator[] ( const int pos ),
		&operator[] ( const uint pos );

	inline T
		defaultValue () const { return end_value; }

	const T
		&at ( int ) const,
		&at ( uint ) const,
		&operator[] ( const int pos ) const,
		&operator[] ( const uint pos ) const,
		&first () const,
		&last () const,
		&next () const,
		&prev () const,
		&current () const,

		&peekFirst () const,
		&peekLast () const,
		&peekNext () const,
		&peekPrev () const,

		&begin () const,
		&end () const;

	inline uint
		count () const { return nItems; }

	inline int
		currentIndex () const { return ptr; }

	inline uint
		preallocNumber () const { return m_nprealloc; }

	bool
		getAlwaysPrealloc () const,
		isEmpty () const,
		currentIsLast () const,
		currentIsFirst () const,
		TIsPointer () const,
		autoDeleteItem () const ;
	
	virtual bool
		operator!= ( const vmList<T>& other ) const,
		operator== ( const vmList<T>& other ) const;
	
	virtual uint
		realloc ( const uint );

	virtual void
		clear ( const bool delete_items = false ),
		clearButKeepMemory ( const bool delete_items = false ),
		reserve ( const uint ),
		moveItems ( const uint to, const uint from, const uint amount ),
		copyItems ( T* dest, const T* src, const uint amount ),
		resetMemory ( const T& , uint = 0 );

	int
		append ( const T& item ),
		prepend ( const T& item );

	uint
		getCapacity () const,
		getMemCapacity () const;

protected:
	void
		setCapacity ( const uint i_capacity ),
		setMemCapacity ( const uint i_memcapacity ),
		setNItems ( const uint nitems ),
		setData ( T* data_ptr );

	inline T*
		data () const { return _data; }

	bool
		checkIsPointer ( TIsPointerType* ) const,
		checkIsPointer ( TIsNotPointerType* ) const,
		checkIsPointer ( TIsFuncPointerType* ) const;

private:
	T* _data;
	T end_value;

	int
		ptr;

	uint
		nItems,
		capacity,
		memCapacity, // only when memory is allocated
		m_nprealloc;

	bool
		mb_ispointer,
		mb_autodel;

	typename IsPointer<T>::Result r;
};

template <typename T>
void vmList<T>::vmList_swap ( vmList<T>& list1, vmList<T>& list2 )
{
	using std::swap;
	
	swap ( list1._data, list2._data );
	swap ( list1.end_value, list2.end_value );
	swap ( list1.ptr, list2.ptr );
	swap ( list1.nItems, list2.nItems );
	swap ( list1.capacity, list2.capacity );
	swap ( list1.memCapacity, list2.memCapacity );
	swap ( list1.m_nprealloc, list2.m_nprealloc );
	swap ( list1.mb_ispointer, list2.mb_ispointer );
	swap ( list1.mb_autodel, list2.mb_autodel );
	swap ( list1.r, list2.r );
}

template <typename T>
inline bool vmList<T>::checkIsPointer ( TIsPointerType* ) const
{
	return true;
}

template <typename T>
inline bool vmList<T>::checkIsPointer ( TIsNotPointerType* ) const
{
	return false;
}

template <typename T>
inline bool vmList<T>::checkIsPointer ( TIsFuncPointerType* ) const
{
	return false;
}

template <typename T>
inline void vmList<T>::setPreAllocNumber ( const uint n )
{
	m_nprealloc = n;
}

template <typename T>
inline void vmList<T>::setDefaultValue ( const T &default_value )
{
	end_value = default_value;
}

template <typename T>
inline bool vmList<T>::getAlwaysPrealloc () const
{
	return static_cast<bool> ( m_nprealloc > 0 );
}

template <typename T>
inline bool vmList<T>::isEmpty () const
{
	return static_cast<bool> ( nItems == 0 );
}

template <typename T>
inline bool vmList<T>::currentIsLast () const
{
	return ptr == ( nItems - 1 );
}

template <typename T>
inline bool vmList<T>::currentIsFirst () const
{
	return ptr == 0;
}

template <typename T>
inline bool vmList<T>::TIsPointer () const
{
	return mb_ispointer;
}

template <typename T>
inline bool vmList<T>::autoDeleteItem () const
{
	return mb_autodel;
}

template <typename T>
inline uint vmList<T>::getCapacity () const
{
	return capacity;
}

template <typename T>
inline uint vmList<T>::getMemCapacity () const
{
	return memCapacity;
}

template <typename T>
inline void vmList<T>::setCapacity ( const uint i_capacity )
{
	capacity = i_capacity;
}

template <typename T>
inline void vmList<T>::setMemCapacity ( const uint i_memcapacity )
{
	memCapacity = i_memcapacity;
}

template <typename T>
inline void vmList<T>::setNItems ( const uint nitems )
{
	nItems = nitems;
}

template <typename T>
inline void vmList<T>::setData ( T* data_ptr )
{
	_data = data_ptr;
}

template <typename T>
inline int vmList<T>::append ( const T& item )
{
	return insert ( nItems, item );
}

template <typename T>
inline int vmList<T>::prepend ( const T& item )
{
	return insert ( 0, item );
}

template <typename T>
void vmList<T>::moveItems ( const uint to, const uint from, const uint amount )
{
	if ( to > from )
	{
		int i (static_cast<int>(amount) - 1);
		while ( i >= 0 )
		{
			_data[to + i] = _data[from + i];
			_data[from + i] = end_value;
			--i;
		};
	}
	else
	{
		uint i ( 0 );
		while ( i < amount )
		{
			_data[to+i] = _data[from + i];
			_data[from + i] = end_value;
			++i;
		}
	}
}

template <typename T>
void vmList<T>::resetMemory ( const T& value, uint length )
{
	if ( value != end_value )
		end_value = value;

	if ( length == 0 )
		length = capacity;
	else if ( length > capacity )
	{
		reserve ( length ); //already resets the memory
		return;
	}

	std::fill ( _data, _data + length, value );
}

template <typename T>
void vmList<T>::reserve ( const uint length )
{
	if ( length <= capacity )
		return;

	const int prev_capacity = capacity;
	this->realloc ( length );

	std::fill ( _data + prev_capacity, _data + capacity, end_value );
}

template <typename T>
inline void vmList<T>::copyItems ( T* dest, const T* src, const uint amount )
{
	 std::copy ( src, src + amount, dest );
}

template <typename T>
inline const T& vmList<T>::first () const
{
	if ( nItems >= 1 )
	{
		const_cast<vmList<T>*> ( this )->ptr = 0;
		return _data[0];
	}
	return end_value;
}

template <typename T>
inline const T& vmList<T>::last () const
{
	if ( nItems >= 1 )
	{
		const_cast<vmList<T>*> ( this )->ptr = nItems - 1;
		return _data[ptr];
	}
	return end_value;
}

template <typename T>
inline const T& vmList<T>::next () const
{
	if ( ptr < static_cast<int>( nItems - 1 ) && nItems >= 1 )
	{
		++( const_cast<vmList<T>*> ( this )->ptr );
		return _data[ptr];
	}
	return end_value;
}

template <typename T>
inline const T& vmList<T>::prev () const
{
	if ( ptr > 0 && nItems >= 1 )
	{
		--( const_cast<vmList<T>*> ( this )->ptr );
		return _data[ptr];
	}
	return end_value;
}

template <typename T>
inline const T& vmList<T>::current () const
{
	if ( ptr >= 0 && ptr < static_cast<int>( nItems ) )
		return _data[ptr];
	return end_value;
}

template <typename T>
inline void vmList<T>::setCurrent ( const int pos ) const
{
	if ( pos < static_cast<int> ( nItems ) )
		const_cast<vmList<T>*> ( this )->ptr = pos;
}

template <typename T>
inline void vmList<T>::setCurrent ( const uint pos ) const
{
	if ( pos < nItems )
		const_cast<vmList<T>*> ( this )->ptr = pos;
}

template <typename T>
inline void vmList<T>::setIsPointer ( const bool b_ispointer )
{
	mb_ispointer = b_ispointer;
}

template <typename T>
inline const T& vmList<T>::peekFirst () const
{
	if ( nItems >= 1 )
		return _data[0];
	return end_value;
}

template <typename T>
inline const T& vmList<T>::peekLast () const
{
	if ( nItems >= 1 )
		return _data[nItems - 1];
	return end_value;
}

template <typename T>
inline const T& vmList<T>::peekNext () const
{
	if ( ptr < static_cast<int>( nItems - 1 ) && nItems >= 1 )
		return _data[ptr + 1];
	return end_value;
}

template <typename T>
inline const T& vmList<T>::peekPrev () const
{
	if ( ptr >= 1 && nItems >= 1 )
		return _data[ptr - 1];
	return end_value;
}

template <typename T>
inline const T& vmList<T>::begin () const
{
	return end_value;
}

template <typename T>
inline const T& vmList<T>::end () const
{
	return end_value;
}

template <typename T>
inline const T& vmList<T>::operator[] ( const int pos ) const
{
	if ( (pos >= 0) && (pos < static_cast<int>( nItems )) ) // only return actually inserted items; therefore we use nItems
		return const_cast<T&> ( ( const_cast<vmList<T>*> ( this ) )->_data[pos] );
	return const_cast<T&> ( ( const_cast<vmList<T>*> ( this ) )->end_value );
}

template <typename T>
inline T& vmList<T>::operator[] ( const int pos )
{
	if ( pos >= 0 )
	{
		if ( pos >= static_cast<int>( capacity ) ) // capacity was used because the non-const operator[] may be used to insert items into the list
			realloc ( pos + m_nprealloc );
		if ( pos >= static_cast<int>( nItems ) )
			nItems = pos + 1;
		return _data[pos];
	}
	return end_value;
}

template <typename T>
inline const T& vmList<T>::operator[] ( const uint pos ) const
{
	if ( pos < nItems ) // only return actually inserted items; therefore we use nItems
		return const_cast<T&> ( ( const_cast<vmList<T>*> ( this ) )->_data[pos] );
	return const_cast<T&> ( ( const_cast<vmList<T>*> ( this ) )->end_value );
}

template <typename T>
inline T& vmList<T>::operator[] ( const uint pos )
{
	if ( pos >= capacity ) // capacity was used because the non-const operator[] may be used to insert items into the list
		realloc ( pos + m_nprealloc );
	if ( pos >= nItems )
		nItems = pos + 1;
	return _data[pos];
}

template <typename T>
inline const T& vmList<T>::at ( const int pos ) const
{
	if ( (pos >= 0) && (pos < static_cast<int>( nItems )) ) // only return actually inserted items; therefore we use nItems
		return const_cast<T&> ( ( const_cast<vmList<T>*> ( this ) )->_data[pos] );
	return const_cast<T&> ( ( const_cast<vmList<T>*> ( this ) )->end_value );
}

template <typename T>
inline const T& vmList<T>::at ( const uint pos ) const
{
	if ( pos < nItems ) // only return actually inserted items; therefore we use nItems
		return const_cast<T&> ( ( const_cast<vmList<T>*> ( this ) )->_data[pos] );
	return const_cast<T&> ( ( const_cast<vmList<T>*> ( this ) )->end_value );
}

template <typename T>
inline vmList<T>::vmList ()
	: _data ( nullptr ), ptr ( -1 ), nItems ( 0 ), capacity ( 0 ),
	  memCapacity ( 0 ), m_nprealloc ( 10 ), mb_autodel ( false )
{
	mb_ispointer = checkIsPointer ( &r );
}

template <typename T>
vmList<T>::vmList ( const T& default_value, const uint n_prealloc )
	: vmList ()
{
	m_nprealloc = n_prealloc;
	capacity = m_nprealloc;
	memCapacity = capacity * sizeof ( T );
	end_value = default_value;
	mb_ispointer = checkIsPointer ( &r );

	if ( capacity > 0 )
	{
		_data = new T[capacity];
		std::fill ( _data, _data + capacity, end_value );
	}
}

template <typename T>
vmList<T>::vmList ( const vmList<T>& other )
	: vmList ()
{
	nItems = other.nItems;
	capacity = other.capacity;
	memCapacity = other.memCapacity;
	m_nprealloc = other.m_nprealloc;
	mb_autodel = other.mb_autodel;
	ptr = other.ptr;
	end_value = other.end_value;
	mb_ispointer = other.mb_ispointer;
	::strcpy ( r, other.r );
	_data = new T[this->capacity];
	copyItems ( this->_data, other._data, this->capacity );
}

template<typename T>
inline const vmList<T>& vmList<T>::operator= ( vmList<T> other )
{
	vmList_swap ( *this, other );
	return *this;
}

template<typename T>
bool vmList<T>::operator!= ( const vmList<T>& other ) const //simple and incomplete
{
	if ( other.count () != count () )
		return ( other.preallocNumber () != preallocNumber () );
	return false;
}

template<typename T>
bool vmList<T>::operator== ( const vmList<T>& other ) const //simple and incomplete
{
	if ( other.count () == count () )
		return ( other.preallocNumber () == preallocNumber () );
	return true;
}

template <typename T>
uint vmList<T>::realloc ( const uint newsize )
{
	if ( newsize == capacity )
		return newsize;

	if ( newsize < nItems )
		nItems = newsize;

	T* new_data = new T[newsize];
	if ( nItems > 0 )
	{
		copyItems ( new_data, _data, nItems );
		delete [] _data;
	}
	_data = new_data;

	if ( newsize >= capacity )
	{
		std::fill ( _data + capacity, _data + newsize, end_value );
		/*uint i ( capacity );
		do {
			_data[i] = end_value;
		} while ( ++i < newsize );*/
	}

	memCapacity = newsize * sizeof ( T );
	capacity = newsize;
	return memCapacity;
}

template <typename T>
void vmList<T>::clear ( const bool delete_items )
{
	if ( _data != nullptr )
	{
		if ( delete_items || mb_autodel )
		{
			for ( uint pos ( 0 ); pos < nItems; ++pos )
				deletePointer ( &r, pos );
		}
		delete [] _data;
		_data = nullptr;
		nItems = capacity = memCapacity = 0;
		ptr = -1;
	}
}

template <typename T>
void vmList<T>::deletePointer ( TIsPointerType*, const uint pos )
{
	if ( _data[pos] )
	{
		delete _data[pos];
		_data[pos] = nullptr;
	}
}

template <typename T>
void vmList<T>::deletePointer ( TIsNotPointerType*, const uint ) {}

template <typename T>
void vmList<T>::deletePointer ( TIsFuncPointerType*, const uint ) {}

template <typename T>
void vmList<T>::clear ( const uint from, const uint to, const bool keep_memory, const bool delete_items )
{
	if ( _data != nullptr && from < capacity && to < capacity && from < to )
	{
		uint pos ( from );
		for ( ; pos < to; ++pos )
		{
			if ( delete_items || mb_autodel )
				deletePointer ( &r, pos );
			if ( !keep_memory )
				delete _data[pos];
			_data[pos] = nullptr;
		}
		if ( ptr != -1 )
		{
			if ( unsigned ( ptr ) >= from )
			{
				if ( from != 0 )
					ptr = from - 1;
				else
					ptr = -1;
			}
		}
		pos = to - from;
		nItems -= pos;
		capacity -= pos;
		memCapacity -= pos;
	}
}

template<typename T>
inline void vmList<T>::setAutoDeleteItem ( const bool bauto_del )
{
	mb_autodel = bauto_del;
}

template <typename T>
void vmList<T>::clearButKeepMemory ( const bool delete_items )
{
	if ( _data != nullptr )
	{
		if ( delete_items || mb_autodel )
		{
			uint i ( 0 );
			while ( i < nItems )
				deletePointer ( &r, i++ );
		}
		std::fill ( _data, _data + nItems, end_value );
	}
	nItems = 0;
	ptr = -1;
}

template <typename T>
void vmList<T>::resize ( const uint n )
{
	if ( n >= nItems )
	{
		if ( n > capacity )
			reserve ( n );
		nItems = n;
	}
	else
	{
		realloc ( n );
		nItems = n;
	}
}

template <typename T>
int vmList<T>::remove ( const int pos, const bool delete_item )
{
	int ret ( -1 );
	if ( (pos >= 0) && (pos < static_cast<int>( nItems ) ) )
	{
		if ( delete_item )
			deletePointer ( &r, pos );
		if ( pos < static_cast<int>( nItems - 1 ) )
			moveItems ( pos, pos + 1, nItems - pos - 1 );
		_data[nItems-1] = end_value;
		--nItems;
		ret = nItems;
		if ( ptr >= pos )
			ptr = nItems - 1;
	}
	return ret;
}

template <typename T>
int vmList<T>::removeOne ( const T& item, int pos, const bool delete_item )
{
	int ret ( -1 );
	while ( (pos >= 0) && (pos < static_cast<int>( nItems ) ) )
	{
		if ( _data[pos] == item )
		{
			ret = remove ( pos, delete_item );
			break;
		}
		++pos;
	}
	return ret;
}

template <typename T>
int vmList<T>::replace ( const uint pos, const T &item )
{
	int ret ( -1 );
	if ( pos < nItems )
	{
		_data[pos] = item;
		ret = static_cast<int>(pos);
	}
	return ret;
}

template <typename T>
int vmList<T>::contains ( const T& item, int from_pos ) const
{
	if ( nItems > 0 )
	{
		bool forward ( true );

		if ( from_pos < 0 || from_pos >= static_cast<int>( nItems ) )
		{
			from_pos = nItems - 1;
			forward = false;
		}
		int i = from_pos;
		if ( forward )
		{
			for ( ; i < static_cast<int>( nItems ); ++i )
			{
				if ( _data[i] == item )
					return i;
			}
		}
		else
		{
			for ( ; i >= 0; --i )
			{
				if ( _data[i] == item )
					return i;
			}
		}
	}
	return -1;
}

template <typename T>
int vmList<T>::insert ( const uint pos, const T& item )
{
	int ret ( 1 );

	if ( pos <= capacity )
	{
		if ( ( m_nprealloc > 0 ) && ( pos == capacity ) )
		{
			if ( nItems != 0 )
				ret = static_cast<int>( this->realloc ( capacity + m_nprealloc ) );
			else
			{
				capacity = m_nprealloc;
				memCapacity = capacity * sizeof ( T );
				_data = new T[capacity];
				resetMemory ( end_value, capacity );
			}
		}
	}
	else
	{
		if ( m_nprealloc > 0 )
			ret = static_cast<int>( this->realloc ( pos + m_nprealloc ) );
	}

	if ( ret != 0 )
	{
		if ( pos <= (nItems - 1) )
			moveItems ( pos + 1, pos, nItems - pos );
		_data[pos] = item;
		ret = static_cast<int>(pos);
		ptr = pos; // the inserted item becomes the current item
		++nItems;
	}
	return ret;
}

//-------------------------------------------------NUMBERS-LIST------------------------------------------------

template<typename T>
class podList : public vmList<T>
{

public:
	podList ();
	explicit podList ( const T& default_value, const uint n_prealloc );
	podList ( const podList<T>& other );

	const podList<T>& operator= ( podList<T> other );
	
	inline podList<T> ( podList<T>&& other ) : podList<T> ()
	{
		vmList<T>::vmList_swap ( static_cast<vmList<T>&>( *this ), static_cast<vmList<T>&>( other ) );
	}
	
	virtual inline ~podList () { this->clear (); }

	uint
	realloc ( const uint );

	void
	clear ( const bool = false ),
		  clearButKeepMemory ( const bool = false );

	void
	resetMemory ( const T& value, uint length = 0 ),
				moveItems ( const uint to, const uint from, const uint amount ),
				copyItems ( T* dest, const T* src, const uint amount ),
				reserve ( const uint );
};

template<typename T>
inline podList<T>::podList ()
	: vmList<T> ()
{
	vmList<T>::setDefaultValue ( 0 );
	vmList<T>::setPreAllocNumber ( 10 );
}

template<typename T>
podList<T>::podList ( const T& default_value, const uint n_prealloc )
	: podList<T> ()
{
	vmList<T>::setDefaultValue ( default_value );
	vmList<T>::setPreAllocNumber ( n_prealloc );
	vmList<T>::setCapacity ( n_prealloc );
	vmList<T>::setMemCapacity ( n_prealloc * sizeof ( T ) );
	vmList<T>::setData ( static_cast<T*>( ::malloc ( vmList<T>::getMemCapacity () ) ) );
	::memset ( vmList<T>::data (), default_value, vmList<T>::getMemCapacity () );
}

template<typename T>
podList<T>::podList ( const podList<T>& other )
	: podList<T> ()
{
	vmList<T>::setDefaultValue ( other.defaultValue () );
	vmList<T>::setPreAllocNumber ( other.preallocNumber () );
	vmList<T>::setCapacity ( other.getCapacity () );
	vmList<T>::setMemCapacity ( other.getMemCapacity () );
	vmList<T>::setNItems ( other.count () );
	vmList<T>::setCurrent ( other.currentIndex () );
	vmList<T>::setData ( static_cast<T*>( ::malloc ( vmList<T>::getMemCapacity () ) ) );
	::memcpy ( vmList<T>::data (), other.data (), vmList<T>::getMemCapacity () );
}

template<typename T>
const podList<T>& podList<T>::operator= ( podList<T> other )
{
	vmList<T>::vmList_swap ( static_cast<vmList<T>&>( *this ), static_cast<vmList<T>>( other ) );
	return *this;	
}

template <typename T>
inline void podList<T>::moveItems ( const uint to, const uint from, const uint amount )
{
	if ( to > from ) {
		::memmove ( static_cast<void*>( vmList<T>::data () + to ),
					static_cast<void*>( vmList<T>::data () + from ), amount * sizeof ( T ) );
		::memset ( static_cast<void*>( vmList<T>::data () + from ),
				   vmList<T>::defaultValue (), ( ( to - from - 1 ) * sizeof ( T ) ) );
	}
	else {
		::memmove ( static_cast<void*>( vmList<T>::data () + to ),
					static_cast<void*>( vmList<T>::data () + from ), amount * sizeof ( T ) );
		::memset ( static_cast<void*>( vmList<T>::data () + from ),
				   vmList<T>::defaultValue (), ( ( from - to - 1 ) * sizeof ( T ) ) );
	}
}

template <typename T>
inline void podList<T>::copyItems ( T* dest, const T* src, const uint amount )
{
	::memcpy ( static_cast<void*>( dest ), static_cast<void*>( const_cast<T*> ( src ) ), amount * sizeof ( T ) );
}

template <typename T>
void podList<T>::resetMemory ( const T& value, uint length )
{
	if ( value != vmList<T>::defaultValue () )
		vmList<T>::setDefaultValue ( value );

	if ( length == 0 )
		length = vmList<T>::getCapacity ();
	else if ( length > vmList<T>::getCapacity () ) {
		/*
		 * cheat: this function is called many times after the declaration of the list object.
		 * By setting the capacity to 0, reserve () will actually reset all items, from 0 to length, to de default value
		 */
		if ( vmList<T>::count () == 0 )
			vmList<T>::setCapacity ( 0 );
		reserve ( length ); //already resets the memory
		return;
	}

	::memset ( static_cast<void*>( vmList<T>::data () ), vmList<T>::defaultValue (), length * sizeof ( T ) );
}

template <typename T>
uint podList<T>::realloc ( const uint newsize )
{
	if ( newsize == vmList<T>::getCapacity () )
		return newsize;

	const uint newMemCapacity ( newsize * sizeof ( T ) );

	vmList<T>::setData ( static_cast<T*> (
							 ::realloc ( static_cast<void*>( vmList<T>::data () ), newMemCapacity ) ) );
	if ( newsize < vmList<T>::count () )
		vmList<T>::setNItems ( newsize );
	else
	{
		const uint i ( vmList<T>::getCapacity () );
		::memset ( ( vmList<T>::data () + i ), vmList<T>::defaultValue (), newsize - i );
	}
	vmList<T>::setMemCapacity ( newMemCapacity );
	vmList<T>::setCapacity ( newsize );
	return newMemCapacity;
}

template <typename T>
void podList<T>::reserve ( const uint length )
{
	if ( length <= vmList<T>::getCapacity () )
		return;

	uint newMemCapacity ( length * sizeof ( T ) );
	vmList<T>::setData ( static_cast<T*> ( ::realloc ( static_cast<void*>( vmList<T>::data () ), newMemCapacity ) ) );
	::memset ( vmList<T>::data (), vmList<T>::defaultValue (), newMemCapacity );
	vmList<T>::setMemCapacity ( newMemCapacity );
	vmList<T>::setCapacity ( length );
}

template <typename T>
void podList<T>::clear ( const bool )
{
	if ( vmList<T>::data () != nullptr )
	{
		::free ( static_cast<void*>( vmList<T>::data () ) );
		vmList<T>::setData ( nullptr );
		vmList<T>::setMemCapacity ( 0 );
		vmList<T>::setCapacity ( 0 );
		vmList<T>::setNItems ( 0 );
	}
}

template <typename T>
void podList<T>::clearButKeepMemory ( const bool )
{
	if ( vmList<T>::data () != nullptr )
	{
		::memset ( vmList<T>::data (), vmList<T>::defaultValue (), vmList<T>::getMemCapacity () );
	}
	vmList<T>::setNItems ( 0 );
	vmList<T>::setCurrent ( -1 );
}
//-------------------------------------------------NUMBERS-LIST------------------------------------------------

//-------------------------------------------------POINTERS-LIST------------------------------------------------
template<typename T>
class pointersList : public vmList<T>
{

public:
	pointersList ();
	explicit pointersList ( const uint n_prealloc );
	pointersList ( const pointersList<T>& other );

	const pointersList<T>& operator= ( pointersList<T> other );
	
	inline pointersList<T> ( pointersList<T>&& other ) : pointersList<T> ()
	{
		vmList<T>::vmList_swap ( static_cast<vmList<T>&>( *this ), static_cast<vmList<T>&>( other ) );
	}
	
	virtual inline ~pointersList () { this->clear (); }

	uint
		realloc ( const uint );

	void
		clear ( const bool delete_items = false ),
		clearButKeepMemory ( const bool delete_items = false );

	void
		resetMemory ( const T&, uint length = 0 ),
		moveItems ( const uint to, const uint from, const uint amount ),
		copyItems ( T* dest, const T* src, const uint amount ),
		reserve ( const uint );

private:
	typename vmList<T>::template IsPointer<T>::Result r;
};

template <typename T>
inline void pointersList<T>::moveItems ( const uint to, const uint from, const uint amount )
{
	if ( to > from )
	{
		int i (static_cast<int>(amount) - 1);
		while ( i >= 0 )
		{
			vmList<T>::data ()[to + i] = vmList<T>::data ()[from + i];
			vmList<T>::data ()[from + i] = vmList<T>::defaultValue ();
			--i;
		};
	}
	else
	{
		uint i ( 0 );
		while ( i < amount )
		{
			vmList<T>::data ()[to + i] = vmList<T>::data ()[from + i];
			vmList<T>::data ()[from + i] = vmList<T>::defaultValue ();
			++i;
		}
	}
	/*if ( to > from )
	{
		::memmove ( static_cast<void*>( vmList<T>::data () + to ),
					static_cast<void*>( vmList<T>::data () + from ), amount * sizeof ( T ) );
		::memset ( static_cast<void*>( vmList<T>::data () + from ),
				   0, ( ( to - from - 1 ) * sizeof ( T ) ) );
	}
	else
	{
		::memmove ( static_cast<void*>( vmList<T>::data () + to ),
					static_cast<void*>( vmList<T>::data () + from ), amount * sizeof ( T ) );
		::memset ( static_cast<void*>( vmList<T>::data () + from ),
				   0, ( ( from - to - 1 ) * sizeof ( T ) ) );
	}*/
}

template<typename T>
inline pointersList<T>::pointersList ()
	: vmList<T> ()
{
	vmList<T>::setDefaultValue ( nullptr );
	vmList<T>::setPreAllocNumber ( 100 );
}

template<typename T>
pointersList<T>::pointersList ( const uint n_prealloc )
	: pointersList<T> ()
{
	vmList<T>::setPreAllocNumber ( n_prealloc );
	vmList<T>::setCapacity ( n_prealloc );
	vmList<T>::setMemCapacity ( n_prealloc * sizeof ( T ) );
	vmList<T>::setData ( static_cast<T*>( ::malloc ( vmList<T>::getMemCapacity () ) ) );
	::memset ( static_cast<void*>(vmList<T>::data ()), 0, vmList<T>::getMemCapacity () );
}

template<typename T>
pointersList<T>::pointersList ( const pointersList<T>& other )
	: pointersList<T> ()
{
	vmList<T>::setDefaultValue ( other.defaultValue () );
	vmList<T>::setPreAllocNumber ( other.preallocNumber () );
	vmList<T>::setCapacity ( other.getCapacity () );
	vmList<T>::setMemCapacity ( other.getMemCapacity () );
	vmList<T>::setNItems ( other.count () );
	vmList<T>::setCurrent ( other.currentIndex () );
	vmList<T>::setData ( static_cast<T*>( ::malloc ( vmList<T>::getMemCapacity () ) ) );
	::memcpy (static_cast<void*>(vmList<T>::data ()), static_cast<void*>(other.data ()), vmList<T>::getMemCapacity () );
}

template<typename T>
const pointersList<T>& pointersList<T>::operator= ( pointersList<T> other )
{
	vmList<T>::vmList_swap ( *this, other );
	return *this;	
}

template <typename T>
inline void pointersList<T>::copyItems ( T* dest, const T* src, const uint amount )
{
	::memcpy ( static_cast<void*>( dest ), static_cast<void*>( const_cast<T*> ( src ) ), amount * sizeof ( T ) );
}

template <typename T>
void pointersList<T>::resetMemory ( const T&, uint length )
{
	if ( length == 0 )
		length = vmList<T>::getCapacity ();
	else if ( length > vmList<T>::getCapacity () )
	{
		/*
		 * cheat: this function is called many times after the declaration of the list object.
		 * By setting the capacity to 0, reserve () will actually reset all items, from 0 to length, to de default value
		 */
		if ( vmList<T>::count () == 0 )
			vmList<T>::setCapacity ( 0 );
		reserve ( length ); //already resets the memory
		return;
	}

	::memset ( static_cast<void*>( vmList<T>::data () ), 0, length * sizeof ( T ) );
}

template <typename T>
uint pointersList<T>::realloc ( const uint newsize )
{
	if ( newsize == vmList<T>::getCapacity () )
		return newsize;

	const uint newMemCapacity ( newsize * sizeof ( T ) );

	T* __restrict new_data ( static_cast<T*> (
							::realloc ( static_cast<void*>( vmList<T>::data () ), newMemCapacity ) ) );
	if ( new_data )
	{
		vmList<T>::setData ( new_data );
		if ( newsize < vmList<T>::count () )
			vmList<T>::setNItems ( newsize );
		else
		{
			const uint i ( vmList<T>::getCapacity () );
			::memset ( ( vmList<T>::data () + i ), 0, newsize - i );
		}
		vmList<T>::setMemCapacity ( newMemCapacity );
		vmList<T>::setCapacity ( newsize );
		return newMemCapacity;
	}
	return 0;
}

template <typename T>
void pointersList<T>::reserve ( const uint length )
{
	if ( length <= vmList<T>::getCapacity () )
		return;

	const uint newMemCapacity ( length * sizeof ( T ) );
	vmList<T>::setData ( static_cast<T*> ( ::realloc ( static_cast<void*>( vmList<T>::data () ), newMemCapacity ) ) );
	::memset ( vmList<T>::data (), 0, newMemCapacity );
	vmList<T>::setMemCapacity ( newMemCapacity );
	vmList<T>::setCapacity ( length );
}

template <typename T>
void pointersList<T>::clear ( const bool delete_items )
{
	if ( vmList<T>::data () != nullptr )
	{
		if ( delete_items || vmList<T>::mb_autodel )
		{
			for ( uint pos ( 0 ); pos < vmList<T>::count (); ++pos )
				this->deletePointer ( &r, pos );
		}
		::free ( static_cast<void*>( vmList<T>::data () ) );
		vmList<T>::setData ( nullptr );
		vmList<T>::setMemCapacity ( 0 );
		vmList<T>::setCapacity ( 0 );
		vmList<T>::setNItems ( 0 );
	}
}

template <typename T>
void pointersList<T>::clearButKeepMemory ( const bool delete_items )
{
	if ( vmList<T>::data () != nullptr )
	{
		if ( delete_items || vmList<T>::mb_autodel )
		{
			for ( uint pos ( 0 ); pos < vmList<T>::count (); ++pos )
				this->deletePointer ( &r, pos );
		}
		::memset ( vmList<T>::data (), 0, vmList<T>::getMemCapacity () );
	}
	vmList<T>::setNItems ( 0 );
	vmList<T>::setCurrent ( -1 );
}
//-------------------------------------------------POINTERS-LIST------------------------------------------------

#endif // vmList_H
