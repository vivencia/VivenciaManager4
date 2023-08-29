#ifndef VMNUMBERFORMATS_H
#define VMNUMBERFORMATS_H

#include <QtCore/QString>
#include <QtCore/QDate>
#include <QtCore/QTime>

/*
 * NOTE:
 * As of version 2.9.41, the setters in this class do not check the string input. To keep code
 * clean and lean, they rely on the caller having made those checks, preferably by calling other
 * functions of this class to format the input. This was done because this was already the default
 * behaviour in VivenciaManager 2, so there was a loadful of code checking that was just sitting there,
 * unused and unecessary
 */

enum VM_NUMBER_TYPE
{
	VMNT_UNSET, VMNT_INT, VMNT_DOUBLE, VMNT_PRICE, VMNT_TIME, VMNT_PHONE, VMNT_DATE
};

static const QString TIME_FORMAT_DEFAULT ( QStringLiteral ( "HH:mm" ) );
static const QString DATE_FORMAT_DB ( QStringLiteral ( "yyyy/MM/dd" ) );
static const QString DATE_FORMAT_HUMAN ( QStringLiteral ( "dd/MM/yyyy" ) );

//constexpr const QStringLiteral DATE_FORMAT_LONG ( " dddd, dd 'de' MMMM 'de' yyyy" );

class vmNumber
{

	enum
	{
		VM_IDX_DAY = 0, VM_IDX_MONTH = 1, VM_IDX_YEAR = 2, VM_IDX_HOUR = 0, VM_IDX_MINUTE = 1, VM_IDX_SECOND = 2,
		VM_IDX_PREFIX = 0, VM_IDX_PHONE1 = 1, VM_IDX_PHONE2 = 2,
		VM_IDX_TENS = 0, VM_IDX_CENTS = 1, VM_IDX_STRFORMAT = 4
	};

	friend void vm_swap ( vmNumber& n1, vmNumber& n2 );
	
public:
	
	inline vmNumber () : m_type ( VMNT_UNSET ), mb_cached ( false ), mb_valid ( false ) {
		for ( unsigned int i ( 0 ); i < 5; ++i ) nbr_part[i] = nbr_upart[i] = 0;
	}
	inline explicit vmNumber ( const int n ) {
		fromInt ( n );
	}
	inline explicit vmNumber ( const unsigned int n ) {
		fromUInt ( n );
	}
	inline explicit vmNumber ( const double n ) {
		fromDoubleNbr ( n );
	}
	inline explicit vmNumber ( const QDate date ) {
		fromQDate ( date );
	}
	inline explicit vmNumber ( const QTime time ) {
		fromQTime ( time );
	}
	
	vmNumber ( const QString& str_number, const VM_NUMBER_TYPE type, const int format = -1 );
	vmNumber ( const vmNumber& other );

	inline const vmNumber& operator= ( vmNumber vmnumber )
	{
		//vmNumber temp ( vmnumber );
		vm_swap ( *this, vmnumber );
		return *this;
	}
	
	inline vmNumber ( vmNumber&& other ) : vmNumber ()
	{
		vm_swap ( *this, other );
	}

	inline ~vmNumber () {}

	void clear ( const bool b_unset_type = true );
	inline VM_NUMBER_TYPE type () const {
		return m_type;
	}
	// Force set type. Use with care under controlled circumstances. Internal use is preferred.
	inline void setType ( const VM_NUMBER_TYPE type ) const {
		m_type = type;
		mb_valid = true; //this line is one of the reasons this method should not be called directly
	}
	inline bool isValid () const { return mb_valid; }
	// Safe convertion
	bool convertTo ( const VM_NUMBER_TYPE new_type, const bool force = false );

	QString toString () const;

	void makeOpposite ();
	bool isNull () const;

	static const vmNumber emptyNumber;

//------------------------------------INT-unsigned int---------------------------------------
	inline bool isInt () const {
		return ( m_type == VMNT_INT ) && isValid ();
	}

	vmNumber& fromStrInt ( const QString& integer );
	vmNumber& fromTrustedStrInt ( const QString& integer );
	vmNumber& fromInt ( const int n );
	vmNumber& fromUInt ( const unsigned int n );
	int toInt () const;
	unsigned int toUInt () const;
	const QString& toStrInt () const;
//------------------------------------INT-unsigned int---------------------------------------

//-------------------------------------DOUBLE----------------------------------------
	inline bool isDouble () const {
		return ( m_type == VMNT_DOUBLE ) && isValid ();
	}

	vmNumber& fromStrDouble ( const QString& str_double );
	vmNumber& fromTrustedStrDouble ( const QString& str_double );
	vmNumber& fromDoubleNbr ( const double n );

	double toDouble () const;
	const QString& toStrDouble() const;
//-------------------------------------DOUBLE----------------------------------------

//-------------------------------------PHONE-----------------------------------------
	inline bool isPhone () const {
		return ( m_type == VMNT_PHONE ) && isValid ();
	}

	vmNumber& fromStrPhone ( const QString& phone );
	vmNumber& fromTrustedStrPhone ( const QString& phone, const bool cache = true );
	const QString& retrievePhoneBody ( const QString& full_phone, const bool numbers_only = false ) const;
	const QString& retrievePhonePrefix ( const QString& full_phone, const bool numbers_only = false ) const;

	const QString& toPhone () const;
	const QString& phoneBody () const;
	const QString& phonePrefix () const;

	void makePhone ( const QString& prefix, const QString& body, const bool trusted = true );
	const QString makePhoneBody () const;
	const QString makePhonePrefix () const;
//-------------------------------------PHONE-----------------------------------------

//-------------------------------------PRICE-----------------------------------------
	inline bool isPrice () const {
		return ( m_type == VMNT_PRICE ) && isValid ();
	}
	void setPrice ( const int tens, const int cents, const bool update = false );

	vmNumber& fromStrPrice ( const QString& price );
	vmNumber& fromTrustedStrPrice ( const QString& price, const bool cache = true );

	const QString& priceToDoubleString () const;
	const QString& toPrice () const;

	static const vmNumber zeroedPrice;
//-------------------------------------PRICE-----------------------------------------

//-------------------------------------DATE------------------------------------------
	enum VM_DATE_FORMAT {
		VDF_HUMAN_DATE = 1, VDF_DB_DATE, VDF_LONG_DATE, VDF_FILE_DATE, VDF_DROPBOX_DATE, VDF_MYSQL_DATE
	};

	inline bool isDate () const {
		return ( m_type == VMNT_DATE ) && isValid ();
	}
	void setDate ( const int day = 0, const int month = 0, const int year = 0, const bool update = false );

	vmNumber& fromStrDate ( const QString& date );
	vmNumber& fromTrustedStrDate ( const QString& date, const VM_DATE_FORMAT format, const bool cache = true );
	vmNumber& dateFromHumanDate ( const QString& date, const bool cache = true );
	vmNumber& dateFromDBDate ( const QString& date, const bool cache = true );
	vmNumber& dateFromDropboxDate ( const QString& date, const bool cache = true );
	vmNumber& dateFromMySQLDate ( const QString& date, const bool cache = true );
	vmNumber& dateFromFilenameDate ( const QString& date, const bool cache = true );
	vmNumber& dateFromLongString ( const QString& date, const bool cache = true );

	const QString& toDate ( const VM_DATE_FORMAT format ) const;

	void fromQDate ( const QDate date );
	const QDate toQDate () const;

	inline unsigned int day () const {
		return nbr_upart[VM_IDX_DAY];
	}
	inline unsigned int month () const {
		return nbr_upart[VM_IDX_MONTH];
	}
	inline unsigned int year () const {
		return nbr_upart[VM_IDX_YEAR];
	}

	inline int monthsSinceDate ( const vmNumber& date ) const
	{
		return monthsSinceDate ( date.month (), date.year () );
	}

	inline int monthsSinceDate ( const unsigned int month, const unsigned int year ) const
	{
		return static_cast<int>(( ( this->year () - year ) * 12 ) + this->month () - month);
	}

	bool isDateWithinRange ( const vmNumber& checkDate, const unsigned int years = 0, const unsigned int months = 0, const unsigned int days = 0 ) const;
	unsigned int julianDay () const;
	unsigned int dayOfYear () const;
	unsigned int dayOfWeek () const;
	unsigned int weekNumber () const;
	bool isDateWithinWeek ( const vmNumber& date ) const;

	static bool isLeapYear ( const unsigned int year );
	static unsigned int daysInMonth ( const unsigned int month, const unsigned int year );
	static unsigned int weeksInYear ( const unsigned int year ) {
		return isLeapYear ( year ) ? 52 : 53;
	}

	inline void setDay ( const int day, const bool update = false ) {
		setDate ( day, 0, 0, update  );
	}
	inline void setMonth ( const int month, const bool update = false ) {
		setDate ( 0, month, 0, update );
	}
	inline void setYear ( const int year, const bool update = false ) {
		setDate ( 0, 0, year, update );
	}

	static const vmNumber& currentDate ();
//-------------------------------------DATE------------------------------------------

//-------------------------------------TIME------------------------------------------
	enum VM_TIME_FORMAT {
		VTF_24_HOUR = 1, VTF_DAYS, VTF_FANCY
	};

	inline bool isTime () const {
		return ( m_type == VMNT_TIME ) && isValid ();
	}

	void setTime ( const int hours, const int minutes, const int seconds, const bool b_add, const bool b_reset_first = true );

	vmNumber& fromStrTime ( const QString& time );
	vmNumber& fromTrustedStrTime ( const QString& time, const VM_TIME_FORMAT format, const bool cache = true );

	void fromQTime ( const QTime time );

	const QString& toTime ( const VM_TIME_FORMAT format = VTF_DAYS ) const;
	const QTime toQTime () const;
	const QString& formatTime ( const int hour, const int min, const int sec, const VM_TIME_FORMAT format ) const;

	inline int hours () const {
		return nbr_part[VM_IDX_HOUR];
	}
	inline int minutes () const {
		return nbr_part[VM_IDX_MINUTE];
	}
	inline int seconds () const {
		return nbr_part[VM_IDX_SECOND];
	}
	
	inline void addDaysToTime ( const unsigned int days ) {
		setTime ( static_cast<int>( days * 24 ), 0, 0, true, false );
	}

	static const vmNumber& currentTime ();
//-------------------------------------TIME------------------------------------------

//-------------------------------------OPERATORS------------------------------------------
	//operators are added as needed//

	// These functions may change the type of the number
	vmNumber& operator= ( const QDate date );
	vmNumber& operator= ( const int n );
	vmNumber& operator= ( const unsigned int n );
	vmNumber& operator= ( const double n );

	bool operator!= ( const vmNumber& vmnumber ) const;
	bool operator== ( const vmNumber& vmnumber ) const;
	bool operator== ( const int number ) const;
	bool operator>= ( const vmNumber& vmnumber ) const;
	bool operator<= ( const vmNumber& vmnumber ) const;
	bool operator< ( const vmNumber& vmnumber ) const;
	bool operator< ( const int number ) const;
	bool operator> ( const int number ) const;
	bool operator> ( const vmNumber& vmnumber ) const;

	vmNumber& operator-= ( const vmNumber& vmnumber );
	vmNumber& operator+= ( const vmNumber& vmnumber );
	vmNumber& operator+= ( const double number );
	vmNumber& operator/= ( const vmNumber& vmnumber );
	vmNumber& operator/= ( const int number );
	vmNumber& operator*= ( const vmNumber& vmnumber );
	vmNumber& operator*= ( const int number );

	vmNumber operator- ( const vmNumber& vmnumber ) const;
	vmNumber operator+ ( const vmNumber& vmnumber ) const;
	vmNumber operator/ ( const vmNumber& vmnumber ) const;
	vmNumber operator/ ( const int number ) const;
	vmNumber operator/ ( const unsigned int number ) const;
	vmNumber operator* ( const vmNumber& vmnumber ) const;
//-------------------------------------OPERATORS------------------------------------------

private:
	QString useCalc ( const vmNumber& n1, const vmNumber& res, const QString& op ) const;
	void fixDate ();
	void fixTime ();
	bool isDate ( const QString& str );
	void makeCachedStringForPhone () const;

	inline bool isCached () const {
		return mb_cached;
	}
	inline void setCached ( const bool cached ) const {
		mb_cached = cached;
	}

	mutable VM_NUMBER_TYPE m_type;
	mutable int nbr_part[5];
	mutable unsigned int nbr_upart[5];
	mutable QString cached_str;
	mutable bool mb_cached;
	mutable bool mb_valid;

	mutable QDate mQDate;
	mutable QTime mQTime;
	mutable QString mQString;

	static vmNumber vmCurrentDateTime;
};


#endif // VMNUMBERFORMATS_H
