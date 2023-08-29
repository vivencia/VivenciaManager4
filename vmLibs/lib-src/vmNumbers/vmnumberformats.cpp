#include "vmnumberformats.h"
#include "vmlibs.h"

#include <vmCalculator/calculator.h>

#include <QtCore/QCoreApplication>

#include <cmath>
#include <climits>

void vm_swap ( vmNumber& n1, vmNumber& n2 )
{
	using std::swap;
	swap ( n1.nbr_part, n2.nbr_part );
	swap ( n1.nbr_upart, n2.nbr_upart );
	swap ( n1.m_type, n2.m_type );
	swap ( n1.mb_cached, n2.mb_cached );
	swap ( n1.mb_valid, n2.mb_valid );
	swap ( n1.cached_str, n2.cached_str );
}

/* you cannot use QStringLiteral outside a function in all compilers,
 * since GCC statement expressions don’t support that.
 * Moreover, the code for QT4 would work, but isn’t read-only sharable:
 */
static const auto CURRENCY = QStringLiteral ( "R$" );

static const QString MONTHS[13] = { QString (),
			QStringLiteral ( "Janeiro" ), QStringLiteral ( "Fevereiro" ), QStringLiteral ( "Março" ), QStringLiteral ( "Abril" ), 
			QStringLiteral ( "Maio" ), QStringLiteral ( "Junho" ), QStringLiteral ( "Julho" ), QStringLiteral ( "Agosto" ),
			QStringLiteral ( "Setembro" ), QStringLiteral ( "Outubro" ), QStringLiteral ( "Novembro" ), QStringLiteral ( "Dezembro" )
};

// Source: http://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
constexpr const int months_table[13] = { -1, 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };

vmNumber vmNumber::vmCurrentDateTime;
const vmNumber vmNumber::zeroedPrice;
const vmNumber vmNumber::emptyNumber;

vmNumber::vmNumber ( const QString& str, const VM_NUMBER_TYPE type, const int format )
	: m_type ( type ), mb_cached ( false ), mb_valid ( false )
{
	if ( type < VMNT_PHONE )
	{
		std::fill ( nbr_part, nbr_part+5, 0 );
		//qfor ( auto &i : nbr_part )
		//	i = 0;
		//for ( unsigned int i ( 0 ); i < 5; ++i )
		//	nbr_part[i] = 0;
	}
	else
	{
		std::fill ( nbr_upart, nbr_upart+5, 0 );
		//for ( auto &i : nbr_upart )
		//	i = 0;
		//for ( unsigned int i ( 0 ); i < 5; ++i )
		//	nbr_upart[i] = 0;
	}

	switch ( type )
	{
		case VMNT_UNSET:
		break;
		case VMNT_INT:
			format > 0 ? fromTrustedStrInt ( str ) : fromStrInt ( str );
		break;
		case VMNT_DOUBLE:
			format > 0 ? fromTrustedStrDouble ( str ) : fromStrDouble ( str );
		break;
		case VMNT_PHONE:
			format > 0 ? fromTrustedStrPhone ( str ) : fromStrPhone ( str );
		break;
		case VMNT_PRICE:
			format > 0 ? fromTrustedStrPrice ( str ) : fromStrPrice ( str );
		break;
		case VMNT_TIME:
			format > 0 ? fromTrustedStrTime ( str, static_cast<VM_TIME_FORMAT> ( format ) ) : fromStrTime ( str );
		break;
		case VMNT_DATE:
			format > 0 ? fromTrustedStrDate ( str, static_cast<VM_DATE_FORMAT> ( format ) ) : fromStrDate ( str );
		break;
	}
}

vmNumber::vmNumber ( const vmNumber& other )
{
	std::copy ( other.nbr_part, other.nbr_part + 5, nbr_part );
	std::copy ( other.nbr_upart, other.nbr_upart + 5, nbr_upart );
	m_type = other.m_type;
	mb_cached = other.mb_cached;
	mb_valid = other.mb_valid;
	cached_str = other.cached_str;
	mQString = other.mQString;
}

QString vmNumber::useCalc ( const vmNumber& n1, const vmNumber& res, const QString& op ) const
{
	QString expression ( QString::number ( res.toDouble (), 'f', 2 ) +
						 op + QString::number ( n1.toDouble (), 'f', 2 ) );

	vmCalculator calc;
	calc.setExpression ( expression );
	calc.eval ( expression );
	return expression;
}

void vmNumber::clear ( const bool b_unset_type )
{
	switch ( type () )
	{
		case VMNT_INT:
		case VMNT_DOUBLE:
		case VMNT_PRICE: 
		case VMNT_TIME:
			std::fill ( nbr_part, nbr_part+5, 0 );
			//for ( unsigned int i ( 0 ); i < 5; ++i )
			//	nbr_part[i] = 0;
		break;
		case VMNT_PHONE:
		case VMNT_DATE:
			std::fill ( nbr_upart, nbr_upart+5, 0 );
			//for ( unsigned int i ( 0 ); i < 5; ++i )
			//	 nbr_upart[i] = 0;
		break;
		case VMNT_UNSET:
		break;
	}
	if ( b_unset_type )
		setType ( VMNT_UNSET );
	setCached ( false );
	mb_valid = false;
	cached_str.clear ();
}

// needs improvement to make some of these actually useful
// so far, ret is useless, but it is kept here for future use
bool vmNumber::convertTo ( const VM_NUMBER_TYPE new_type, const bool force )
{
	mb_valid = false;
	if ( new_type != m_type )
	{
		if ( m_type == VMNT_UNSET )
		{
			clear ();
			return true;
		}
		switch ( type () )
		{
			case VMNT_INT:
			case VMNT_UNSET:
				switch ( new_type )
				{
					case VMNT_PHONE:
					case VMNT_DATE:
						if ( force )
							mb_valid = true;
					break;
					case VMNT_TIME:
						if ( force )
							mb_valid = true;
					break;
					case VMNT_DOUBLE:
					case VMNT_INT:
					case VMNT_PRICE:
					case VMNT_UNSET:
						mb_valid = true;
					break;
				}
				break;
			case VMNT_DATE:
			case VMNT_PHONE:
				switch ( new_type )
				{
					case VMNT_INT:
						if ( !force )
							break;
						nbr_part[0] = static_cast<int> ( nbr_upart[0] );
						mb_valid = true;
					break;
					case VMNT_DOUBLE:
					case VMNT_PRICE:
					case VMNT_TIME:
						if ( !force )
							break;
						nbr_part[0] = static_cast<int> ( nbr_upart[0] );
						nbr_part[1] = static_cast<int> ( nbr_upart[1] );
						mb_valid = true;
					break;
					case VMNT_DATE:
					case VMNT_PHONE:
					case VMNT_UNSET:
						mb_valid = true;
					break;
				}
			break;
			case VMNT_PRICE:
			case VMNT_DOUBLE:
			case VMNT_TIME:
				switch ( new_type )
				{
					case VMNT_INT:
						if ( nbr_part[1] != 0 )
						{
							if ( !force )
								break;
							nbr_part[1] = 0;
						}
						mb_valid = true;
					break;
					case VMNT_DOUBLE:
					case VMNT_PRICE:
					case VMNT_TIME:
					case VMNT_UNSET:
						mb_valid = true;
					break;
					case VMNT_DATE:
					case VMNT_PHONE:
					if ( !force )
						break;
					nbr_upart[0] = static_cast<unsigned int> ( nbr_part[0] );
					nbr_upart[1] = static_cast<unsigned int> ( nbr_part[1] );
					break;
				}
			break;
		}
	}
	if ( mb_valid )
	{
		setType ( new_type );
		setCached ( false );
	}
	return mb_valid;
}

QString vmNumber::toString () const
{
	if ( isCached () )
		return cached_str;

	switch ( type () )
	{
		case VMNT_PRICE:
			return toPrice ();
		
		case VMNT_DATE:
			return toDate ( VDF_HUMAN_DATE );
		
		case VMNT_PHONE:
			return toPhone ();
		
		case VMNT_TIME:
			return toTime ( VTF_DAYS );
		
		case VMNT_INT:
			return toStrInt ();
		
		case VMNT_DOUBLE:
			return toStrDouble ();
	
		case VMNT_UNSET:
			return emptyString;
	}
	return emptyString;
}

void vmNumber::makeOpposite ()
{
	switch ( type () )
	{
		case VMNT_PRICE:
		case VMNT_INT:
		case VMNT_DOUBLE:
		case VMNT_TIME:
			{
				for ( int& i : nbr_part )
					i = 0 - i;
				//for ( unsigned int i ( 0 ); i < 5; ++i )
				//	nbr_part[i] = 0 - nbr_part[i];
			}
		break;
		case VMNT_PHONE:
		case VMNT_DATE:
		case VMNT_UNSET:
		return;
	}
}

bool vmNumber::isNull () const
{
	switch ( m_type )
	{
		case VMNT_INT:
		case VMNT_DOUBLE:
		case VMNT_PRICE:
		case VMNT_TIME:
			return !( nbr_part[0] | nbr_part[1] );
		case VMNT_PHONE:
		case VMNT_DATE:
			return !( nbr_upart[0] | nbr_upart[1] | nbr_upart[2] );
		case VMNT_UNSET:
			return true;
	}
	return true;
}
//------------------------------------INT-unsigned int---------------------------------------
//TODO: check for values over the limit. VMNT_INT uses an internal int but accepts unsigned ints, so this might cause trouble

vmNumber& vmNumber::fromStrInt ( const QString& integer )
{
	if ( !integer.isEmpty () )
	{
		QChar qchr;
		int chr ( integer.indexOf ( CHR_HYPHEN ) );
		const bool b_negative ( chr != -1 );
		const int len ( integer.length () );
		clear ( false );
		while ( ++chr < len )
		{
			qchr = integer.at ( chr );
			if ( qchr.isDigit () )
			{
				if ( nbr_part[0] != 0 )
					nbr_part[0] *= 10;
				nbr_part[0] += qchr.digitValue ();
			}
		}
		if ( b_negative )
			nbr_part[0] = 0 - nbr_part[0];
		setType ( VMNT_INT );
		setCached ( false );
	}
	return *this;
}

vmNumber& vmNumber::fromTrustedStrInt ( const QString& integer )
{
	clear ( false );
	mb_valid = false;
	nbr_part[0] = integer.toInt ( &mb_valid );
	if ( mb_valid ) {
		setType ( VMNT_INT );
		setCached ( true );
		cached_str = integer;
	}
	return *this;
}

vmNumber& vmNumber::fromInt ( const int n )
{
	setType ( VMNT_INT );
	setCached ( false );
	nbr_part[0] = n;
	return *this;
}

vmNumber& vmNumber::fromUInt ( const unsigned int n )
{
	setType ( VMNT_INT );
	setCached ( false );
	nbr_part[0] = static_cast<int>( n );
	return *this;
}

int vmNumber::toInt () const
{
	switch ( type () )
	{
		case VMNT_INT:
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			return nbr_part[0];
		
		case VMNT_PHONE: // all digits
			return static_cast<int>(nbr_upart[VM_IDX_PREFIX] * 10000000000 + nbr_upart[VM_IDX_PHONE1] * 10000 + nbr_upart[VM_IDX_PHONE2]);
		
		case VMNT_TIME: // time in minutes
			return static_cast<int>(nbr_part[VM_IDX_HOUR] * 60 + nbr_part[VM_IDX_MINUTE]);
			
		case VMNT_DATE: // days since Oct 10, 1582
			return static_cast<int>(julianDay ());
			
		case VMNT_UNSET:
			return 0;
	}
	return 0;
}

unsigned int vmNumber::toUInt () const
{
	// This does not work under several circumstances
	return static_cast<unsigned int> ( toInt () );
}

const QString& vmNumber::toStrInt () const
{
	if ( isInt () )
	{
		if ( !isCached () )
		{
			cached_str = QString::number ( nbr_part[0] );
			setCached ( true );
		}
		return cached_str;
	}

	mQString = QString::number ( toInt () );
	return mQString;
}
//------------------------------------INT-unsigned int---------------------------------------

//-------------------------------------DOUBLE----------------------------------------
vmNumber& vmNumber::fromStrDouble ( const QString& str_double )
{
	int chr ( 0 );
	const int len ( str_double.length () );
	if ( chr < len )
	{
		QChar qchr;
		int idx ( 0 );
		bool negative ( false );
		do
		{
			qchr = str_double.at ( chr );
			if ( qchr.isDigit () )
			{
				if ( nbr_part[idx] != 0 )
					nbr_part[idx] *= 10;
				nbr_part[idx] += qchr.digitValue ();
				if ( nbr_part[VM_IDX_CENTS] >= 10 )
					break; // only two decimal places after separator
			}
			else if ( qchr == CHR_DOT || qchr == CHR_COMMA )
				idx = 1;
			else if ( qchr == CHR_HYPHEN )
			{
				if ( nbr_part[VM_IDX_TENS] == 0 )
					negative = true;
			}
		} while ( ++chr < len );
		if ( negative )
			nbr_part[VM_IDX_TENS] = 0 - nbr_part[VM_IDX_TENS];
		if ( nbr_part[VM_IDX_CENTS] < 10 )
			nbr_part[VM_IDX_CENTS] *= 10;
		setType ( VMNT_DOUBLE );
		setCached ( false );
		return *this;
	}
	clear ();
	return *this;
}

vmNumber& vmNumber::fromTrustedStrDouble ( const QString& str_double )
{
	//This is a hack. Certain methods rely on calling trusted string formats, but the old
	//database is formatted wrongly. Since the migration code did not resolve this, we must
	//treat the string as having an unknown format is its length does not reflect the minimum
	// number of characters the correct format must have ( 0.00 = 4, 1.00 = 4, etc. )
	if ( str_double.length () < 4 )
	{
		return fromStrDouble ( str_double );
	}
	
	clear ( false );
	nbr_part[0] = str_double.leftRef ( str_double.indexOf ( CHR_DOT ) ).toInt ( &mb_valid );
	
	if ( mb_valid )
	{
		setType ( VMNT_DOUBLE );
		setCached ( true );
		cached_str = str_double;
		nbr_part[1] = str_double.rightRef ( 2 ).toInt ();
	}
	return *this;
}

vmNumber& vmNumber::fromDoubleNbr ( const double n )
{
	nbr_part[VM_IDX_TENS] = static_cast<int> ( n );
	const double temp ( ( n - static_cast<double> ( nbr_part[VM_IDX_TENS] ) ) * 100.111 );
	nbr_part[VM_IDX_CENTS] = static_cast<int> ( temp );
	setCached ( false );
	setType ( VMNT_DOUBLE );
	return *this;
}

double vmNumber::toDouble () const
{
	switch ( type () ) {
		case VMNT_PRICE:
		case VMNT_DOUBLE:
		{
			const double decimal ( static_cast<double> ( nbr_part[1] ) / 100.0 );
			const double val ( static_cast<double> ( nbr_part[0] ) + ( nbr_part[0] >= 0 ? decimal : 0 - decimal ) );
			return val;
		}
		
		case VMNT_INT:
			return static_cast<double> ( nbr_part[0] );
		
		case VMNT_PHONE: // all digits
			return static_cast<double> (
				   static_cast<double> ( nbr_upart[VM_IDX_PREFIX] * 1000000000.0 ) +
				   static_cast<double> ( nbr_upart[VM_IDX_PHONE1] * 10000.0 ) +static_cast<double> ( nbr_upart[VM_IDX_PHONE2] ) );
		
		case VMNT_TIME: // time in minutes
			return static_cast<double> ( nbr_part[0] * 60.0 + nbr_part[1] );
			
		case VMNT_DATE:
			return static_cast<double> ( julianDay () );
			
		case VMNT_UNSET:
			return 0.0;
	}
	return 0.0;
}

const QString& vmNumber::toStrDouble () const
{
	if ( isDouble () )
	{
		if ( !isCached () )
		{
			cached_str = QString::number ( toDouble (), 'f', 2 );
			setCached ( true );
		}
		return cached_str;
	}

	mQString = QString::number ( toDouble (), 'f', 2 );
	return mQString;
}
//-------------------------------------DOUBLE----------------------------------------

//-------------------------------------DATE------------------------------------------
void vmNumber::fixDate ()
{
	auto m ( static_cast<int>( month () ) ), y ( static_cast<int>( year () ) );
	int d ( 0 ), days_in_month ( 0 );
	if ( nbr_part[VM_IDX_DAY] > 0 )
	{
		d = static_cast<int>( nbr_upart[VM_IDX_DAY] ) + static_cast<int>( ::fabs ( static_cast<double>( nbr_part[VM_IDX_DAY] ) ) );
		do
		{
			days_in_month = static_cast<int>( daysInMonth ( static_cast<unsigned int>( m ), static_cast<unsigned int>( y ) ) );
			if ( d > days_in_month )
			{
				d -= days_in_month;
				if ( m == 12 )
				{
					m = 1;
					y++;
				}
				else
					m++;
			}
			else
				break;
		} while ( true );
	}
	else
	{
		d = static_cast<int>(day ()) + nbr_part[VM_IDX_DAY];
		if ( d <= 0 )
		{
			if ( m != 1 )
				m--;
			else
			{
				m = 12;
				y--;
			}
			do
			{
				days_in_month = static_cast<int>(daysInMonth ( static_cast<unsigned int>(m), static_cast<unsigned int>(y) ));
				d += days_in_month;
				if ( d <= 0 )
				{
					if ( m == 1 )
					{
						m = 12;
						y--;
					}
					else
						m--;
				}
				else
					break;
			} while ( true );
		}
	}

	const auto years_by_months ( static_cast<int>(nbr_part[VM_IDX_MONTH] / 12 ) );
	const auto remaining_months ( static_cast<int>(nbr_part[VM_IDX_MONTH] % 12) );
	y += years_by_months;
	m += remaining_months;

	if ( m > 12 )
	{
		++y;
		m -= 12;
	}
	else if ( m <= 0 )
	{
		--y;
		m += 12;
	}
	y += nbr_part[VM_IDX_YEAR];

	nbr_upart[VM_IDX_DAY] = static_cast<unsigned int>(d);
	nbr_upart[VM_IDX_MONTH] = static_cast<unsigned int>(m);
	nbr_upart[VM_IDX_YEAR] = static_cast<unsigned int>(y);
}

void vmNumber::setDate ( const int day, const int month, const int year, const bool update )
{
	if ( !isDate () )
	{
		nbr_upart[VM_IDX_DAY] = nbr_upart[VM_IDX_MONTH] = nbr_upart[VM_IDX_YEAR] = 0;
		setType ( VMNT_DATE );
	}
	else
	{
		if ( !update )
			nbr_upart[VM_IDX_DAY] = nbr_upart[VM_IDX_MONTH] = nbr_upart[VM_IDX_YEAR] = 0;
	}

	nbr_part[VM_IDX_DAY] = day;
	nbr_part[VM_IDX_MONTH] = month;
	nbr_part[VM_IDX_YEAR] = year;

	if ( !update && year != 0 )
	{
		if ( nbr_part[VM_IDX_YEAR] < 100 )
			nbr_part[VM_IDX_YEAR] += 2000;
	}
	if ( !update && day == 0 )
		nbr_part[VM_IDX_DAY] = 1;

	fixDate ();
	mb_valid = true;
	setCached ( false );
}

vmNumber& vmNumber::fromStrDate ( const QString& date )
{
	if ( !date.isEmpty () )
	{
		QLatin1Char sep_chr ( CHR_F_SLASH );
		int n ( date.indexOf ( sep_chr ) ); //DB_DATE or HUMAN_DATE
		if ( n == -1 )
		{
			sep_chr = CHR_HYPHEN;
			n = date.indexOf ( sep_chr ); // DROPBOX or MYSQL
		}
		
		if ( n != -1 ) 
		{
			const int idx ( date.indexOf ( sep_chr, n + 1 ) );

			if ( n == 4 ) // day starts with year
			{
				nbr_upart[VM_IDX_YEAR] = date.leftRef ( n ).toUInt ();
				nbr_upart[VM_IDX_DAY] = date.rightRef ( date.length () - idx - 1 ).toUInt ();
				nbr_upart[VM_IDX_STRFORMAT] = sep_chr == CHR_F_SLASH ? VDF_DB_DATE : VDF_DROPBOX_DATE;
			}
			else // date may start with day or a two digit year
			{
				nbr_upart[VM_IDX_DAY] = date.leftRef ( n ).toUInt ();
				if ( nbr_upart[VM_IDX_DAY] > 31 ) // two-digit year
				{
					nbr_upart[VM_IDX_YEAR] = nbr_upart[VM_IDX_DAY];
					nbr_upart[VM_IDX_DAY] = date.rightRef ( 2 ).toUInt ();
				}
				else
				{
					nbr_upart[VM_IDX_YEAR] = date.rightRef ( date.length () -
														  date.lastIndexOf ( sep_chr ) - 1 ).toUInt ();
				}
				// Most-most likely not a dropbox date. But we use it here as a fallback
				nbr_upart[VM_IDX_STRFORMAT] = sep_chr == CHR_F_SLASH ? VDF_HUMAN_DATE : VDF_DROPBOX_DATE;
			}
			++n;
			nbr_upart[VM_IDX_MONTH] = date.midRef ( n, idx - n ).toUInt ();
			if ( nbr_upart[VM_IDX_YEAR] < 100 )
			{
				nbr_upart[VM_IDX_YEAR] += 2000;
			}

			setType ( VMNT_DATE );
		}
		else
		{
			if ( date.contains ( QStringLiteral ( "de" ) ) )
			{
				return dateFromLongString ( date );
			}
			return dateFromFilenameDate ( date );
		}
		setCached ( false );
	}
	else
	{
		clear ( false );
		nbr_upart[VM_IDX_YEAR] = 2000;
		nbr_upart[VM_IDX_MONTH] = 1;
		nbr_upart[VM_IDX_DAY] = 1;
	}
	return *this;
}

vmNumber& vmNumber::fromTrustedStrDate ( const QString& date, const VM_DATE_FORMAT format, const bool cache )
{
	clear ( false );
	if ( !date.isEmpty () )
	{
		switch ( format )
		{
			case VDF_HUMAN_DATE:
				return dateFromHumanDate ( date, cache );
			
			case VDF_DB_DATE:
				return dateFromDBDate ( date, cache );
			
			case VDF_FILE_DATE:
				return dateFromFilenameDate ( date, cache );
			
			case VDF_LONG_DATE:
				return dateFromLongString ( date, cache );
			
			case VDF_DROPBOX_DATE:
				return dateFromDropboxDate ( date, cache );
				
			case VDF_MYSQL_DATE:
				return dateFromMySQLDate ( date, cache );
		}
	}
	else
	{
		nbr_upart[VM_IDX_YEAR] = 2000;
		nbr_upart[VM_IDX_MONTH] = 1;
		nbr_upart[VM_IDX_DAY] = 1;
	}
	return *this;
}

vmNumber& vmNumber::dateFromHumanDate ( const QString& date, const bool cache )
{
	nbr_upart[VM_IDX_YEAR] = date.rightRef ( 4 ).toUInt ();
	nbr_upart[VM_IDX_MONTH] = date.midRef ( 3, 2 ).toUInt ();
	nbr_upart[VM_IDX_DAY] = date.leftRef ( 2 ).toUInt ();
	setType ( VMNT_DATE );
	nbr_upart[VM_IDX_STRFORMAT] = VDF_HUMAN_DATE;
	if ( cache )
	{
		setCached ( true );
		cached_str = date;
	}
	return *this;
}

vmNumber& vmNumber::dateFromDBDate ( const QString& date, const bool cache )
{
	nbr_upart[VM_IDX_YEAR] = date.leftRef ( 4 ).toUInt ();
	nbr_upart[VM_IDX_MONTH] = date.midRef ( 5, 2 ).toUInt ();
	nbr_upart[VM_IDX_DAY] = date.rightRef ( 2 ).toUInt ();
	setType ( VMNT_DATE );
	nbr_upart[VM_IDX_STRFORMAT] = VDF_DB_DATE;
	if ( cache )
	{
		setCached ( true );
		cached_str = date;
	}
	return *this;
}

vmNumber& vmNumber::dateFromDropboxDate ( const QString& date, const bool cache )
{
	nbr_upart[VM_IDX_YEAR] = date.leftRef ( 4 ).toUInt ();
	nbr_upart[VM_IDX_MONTH] = date.midRef ( 5, 2 ).toUInt ();
	nbr_upart[VM_IDX_DAY] = date.rightRef ( 2 ).toUInt ();
	setType ( VMNT_DATE );
	nbr_upart[VM_IDX_STRFORMAT] = VDF_DROPBOX_DATE;
	if ( cache )
	{
		setCached ( true );
		cached_str = date;
	}
	return *this;
}

vmNumber& vmNumber::dateFromMySQLDate ( const QString& date, const bool cache )
{
	// Mysql and dropbox have both the same date format
	static_cast<void>( dateFromDropboxDate ( date, cache ) );
	nbr_upart[VM_IDX_STRFORMAT] = VDF_MYSQL_DATE;
	return *this;
}

/* Won't try to guess if the date string starts with year or day. Assume it's year 
 * because that's what it should be anyway
 */
vmNumber& vmNumber::dateFromFilenameDate ( const QString& date, const bool cache )
{
	if ( date.length () >= 6 )
	{
		const int year_end_idx ( date.length () > 6 ? 4 : 2 );
		nbr_upart[VM_IDX_YEAR] = date.leftRef ( year_end_idx ).toUInt ();
		nbr_upart[VM_IDX_MONTH] = date.midRef ( year_end_idx, 2 ).toUInt ();
		nbr_upart[VM_IDX_DAY] = date.rightRef ( 2 ).toUInt ();
		setType ( VMNT_DATE );
		nbr_upart[VM_IDX_STRFORMAT] = VDF_FILE_DATE;
		if ( cache )
		{
			setCached ( true );
			cached_str = date;
		}
	}
	return *this;
}

vmNumber& vmNumber::dateFromLongString ( const QString& date, const bool cache )
{
	const QString strDay ( date.section ( QStringLiteral ( "de" ), 0, 0 ).remove ( CHR_SPACE ) );
	const QString strMonth ( date.section ( QStringLiteral ( "de" ), 1, 1 ).remove ( CHR_SPACE ) );
	const QString strYear ( date.section ( QStringLiteral ( "de" ), 2, 2 ).remove ( CHR_SPACE ) );
	const vmNumber& current_date ( vmNumber::currentDate () );

	if ( !strDay.isEmpty () )
	{
		nbr_upart[VM_IDX_DAY] = strDay.toUInt ( &mb_valid );
		if ( !mb_valid )
		{	// day is not a number, but a word
			if ( strDay.startsWith ( QStringLiteral ( "vi" ) ) )
			{
				if ( strDay.contains ( QStringLiteral ( "um" ) ) ) nbr_upart[VM_IDX_DAY] = 21;
				else if ( strDay.contains ( QStringLiteral ( "do" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 22;
				else if ( strDay.contains ( QStringLiteral ( "qu" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 24;
				else if ( strDay.contains ( QStringLiteral ( "tr" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 23;
				else if ( strDay.contains ( QStringLiteral ( "ci" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 25;
				else if ( strDay.contains ( QStringLiteral ( "is" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 26;
				else if ( strDay.contains ( QStringLiteral ( "te" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 27;
				else if ( strDay.contains ( QStringLiteral ( "oi" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 28;
				else nbr_upart[VM_IDX_DAY] = 29;
			}
			else if ( strDay.startsWith ( QStringLiteral ( "prim" ), Qt::CaseInsensitive ) ||
					  strDay.startsWith ( QStringLiteral ( "um" ), Qt::CaseInsensitive ) )
				nbr_upart[VM_IDX_DAY] = 1;
			else if ( strDay.startsWith ( QStringLiteral ( "do" ), Qt::CaseInsensitive ) )
			{
				if ( strDay.startsWith ( QStringLiteral ( "doi" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 2;
				else nbr_upart[VM_IDX_DAY] = 12;
			}
			else if ( strDay.startsWith ( QStringLiteral ( "tr" ), Qt::CaseInsensitive ) )
			{
				if ( strDay.startsWith ( QStringLiteral ( "treze" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 13;
				else if ( strDay.startsWith ( QStringLiteral ( "tri" ), Qt::CaseInsensitive ) )
				{
					if ( strDay.contains ( QStringLiteral ( "um" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 31;
					else nbr_upart[VM_IDX_DAY] = 30;
				}
				else nbr_upart[VM_IDX_DAY] = 3;
			}
			else if ( strDay.startsWith ( QStringLiteral ( "quat" ), Qt::CaseInsensitive ) )
			{
				if ( strDay.startsWith ( QStringLiteral ( "quatro" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 4;
				else nbr_upart[VM_IDX_DAY] = 14;
			}
			else if ( strDay.startsWith ( QStringLiteral ( "ci" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 5;
			else if ( strDay.startsWith ( QStringLiteral ( "sei" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 6;
			else if ( strDay.startsWith ( QStringLiteral ( "set" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 7;
			else if ( strDay.startsWith ( QStringLiteral ( "oi" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 8;
			else if ( strDay.startsWith ( QStringLiteral ( "no" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 9;
			else if ( strDay.startsWith ( QStringLiteral ( "dez" ), Qt::CaseInsensitive ) )
			{
				if ( strDay.compare ( QStringLiteral ( "dez" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 10;
				else if ( strDay.contains ( QStringLiteral ( "set" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 17;
				else if ( strDay.contains ( QStringLiteral ( "oit" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 18;
				else if ( strDay.contains ( QStringLiteral ( "nov" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 19;
				else nbr_upart[VM_IDX_DAY] = 16;
			}
			else if ( strDay.startsWith ( QStringLiteral ( "on" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 11;
			else if ( strDay.startsWith ( QStringLiteral ( "qui" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_DAY] = 15;
			else
				nbr_upart[VM_IDX_DAY] = 0;
		}
	}
	else // day was not provided
		nbr_upart[VM_IDX_DAY] = current_date.day ();

	if ( !strMonth.isEmpty () )
	{
		nbr_upart[VM_IDX_MONTH] = strMonth.toUInt ( &mb_valid );
		if ( !mb_valid )
		{	// month is not a number, but a word
			if ( strMonth.startsWith ( QStringLiteral ( "ja" ) ) ) nbr_upart[VM_IDX_MONTH] = 1;
			else if ( strMonth.startsWith ( QStringLiteral ( "fe" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 2;
			else if ( strMonth.startsWith ( QStringLiteral ( "mar" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 3;
			else if ( strMonth.startsWith ( QStringLiteral ( "abr" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 4;
			else if ( strMonth.startsWith ( QStringLiteral ( "mai" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 5;
			else if ( strMonth.startsWith ( QStringLiteral ( "jun" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 6;
			else if ( strMonth.startsWith ( QStringLiteral ( "jul" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 7;
			else if ( strMonth.startsWith ( QStringLiteral ( "ago" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 8;
			else if ( strMonth.startsWith ( QStringLiteral ( "set" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 9;
			else if ( strMonth.startsWith ( QStringLiteral ( "out" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 10;
			else if ( strMonth.startsWith ( QStringLiteral ( "nov" ), Qt::CaseInsensitive ) ) nbr_upart[VM_IDX_MONTH] = 11;
			else nbr_upart[VM_IDX_MONTH] = 12;
		}
	}
	else
	{ // month was not provided
		if ( nbr_upart[VM_IDX_DAY] == 0 ) // str is not a date string at all
			nbr_upart[VM_IDX_MONTH] = 0;
		else
			nbr_upart[VM_IDX_MONTH] = current_date.month ();
	}

	if ( !strYear.isEmpty () )
	{
		nbr_upart[VM_IDX_YEAR] = strYear.toUInt ( &mb_valid );
		if ( mb_valid )
		{
			if ( nbr_upart[VM_IDX_YEAR] < 30 )
				nbr_upart[VM_IDX_YEAR] += 2000;
		}
		else // year is not a number, but a word
			nbr_upart[VM_IDX_YEAR] = current_date.year (); // too many possibilities. Eventually I will return to this code
	}
	else
	{ // year was not provided
		if ( nbr_upart[VM_IDX_MONTH] == 0 )
			nbr_upart[VM_IDX_YEAR] = 0;
		else
			nbr_upart[VM_IDX_YEAR] = current_date.year ();
	}
	if ( nbr_upart[VM_IDX_YEAR] != 0 )
	{
		nbr_upart[VM_IDX_STRFORMAT] = VDF_LONG_DATE;
		setType ( VMNT_DATE );
		if ( cache )
		{
			setCached ( true );
			cached_str = date;
		}
	}
	mb_valid = false;
	return *this;
}

const QString& vmNumber::toDate ( const VM_DATE_FORMAT format ) const
{
	if ( m_type == VMNT_UNSET )
		setType ( VMNT_DATE );

	if ( isDate () )
	{
		if ( isCached () && format == nbr_upart[VM_IDX_STRFORMAT] )
			return cached_str;

		const QString strYear ( year () < 100 ? QStringLiteral ( "20" ) + QString::number ( year () ) : QString::number ( year () ) );
		const QString strDay ( day () < 10 ? QStringLiteral ( "0" ) + QString::number ( day () ) : QString::number ( day () ) );
		const QString strMonth ( month () < 10 ? QStringLiteral ( "0" ) + QString::number ( month () ) : QString::number ( month () ) );
		nbr_upart[VM_IDX_STRFORMAT] = format;

		switch ( format )
		{
			case VDF_HUMAN_DATE:
				cached_str = strDay + CHR_F_SLASH + strMonth + CHR_F_SLASH + strYear;
			break;
			case VDF_DB_DATE:
				cached_str = strYear + CHR_F_SLASH + strMonth + CHR_F_SLASH + strDay;
			break;
			case VDF_FILE_DATE:
				cached_str = strYear + strMonth + strDay;
			break;
			case VDF_LONG_DATE:
			{
				const auto m_month ( (month () >= 1 && month () <= 12) ? month () : 1 );
				cached_str = strDay + QStringLiteral ( " de " ) + MONTHS[m_month] +
						 QStringLiteral ( " de " ) + strYear;
			}
			break;
			case VDF_DROPBOX_DATE:
			case VDF_MYSQL_DATE:
				cached_str = strYear + CHR_HYPHEN + strMonth + CHR_HYPHEN + strDay;
			break;
		}
		setCached ( true );
		return cached_str;
	}
	return emptyString;
}

void vmNumber::fromQDate ( const QDate date )
{
	if ( !isDate () )
	{
		nbr_upart[VM_IDX_DAY] = nbr_upart[VM_IDX_MONTH] = nbr_upart[VM_IDX_YEAR] = 0;
		setType ( VMNT_DATE );
	}
	nbr_upart[VM_IDX_DAY] = static_cast<unsigned int>(date.day ());
	nbr_upart[VM_IDX_MONTH] = static_cast<unsigned int>(date.month ());
	nbr_upart[VM_IDX_YEAR] = static_cast<unsigned int>(date.year ());
	setCached ( false );
}

const QDate vmNumber::toQDate () const
{
	if ( isDate () )
	{
		mQDate.setDate ( static_cast<int>(year ()), static_cast<int>(month ()), static_cast<int>(day ()) );
	}
	else
	{
		mQDate.setDate ( 2000, 1, 1 );
	}
	return mQDate;
}

bool vmNumber::isDateWithinRange ( const vmNumber& checkDate, const unsigned int years, const unsigned int months, const unsigned int days ) const
{
	vmNumber tempDate ( checkDate );
	tempDate.setDate ( static_cast<int>(days), static_cast<int>(months), static_cast<int>(years), true );
	if ( *this > tempDate )
		return false;
	tempDate = checkDate;
	tempDate.setDate ( static_cast<int>(0 - days), static_cast<int>(0 - months), static_cast<int>(0 - years), true );
	return *this >= tempDate;
	/*if ( *this < tempDate )
		return false;
	return true;*/
}

unsigned int vmNumber::julianDay () const
{
	if ( isDate () )
	{
		const unsigned int a ( ( 14 - month () ) / 12 );
		const unsigned int y ( year () + 4800 - a );
		const unsigned int m ( month () + 12 * a - 3 );
		/* This is a business application. A gregorian date is the only date we'll ever need */
		//if ( year () > 1582 || ( year () == 1582 && month () > 10 ) || ( year () == 1582 && month () == 10 && day () >= 15 ) )
		return day () + ( 153 * m + 2 ) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
		//else
		//	return day () + ( 153 * m + 2 ) / 5 + 365 * y + y / 4 - 32083;
	}
	return 0;
}

unsigned int vmNumber::dayOfYear () const
{
	unsigned int n ( 0 );
	if ( isDate () )
	{
		n = day ();
		for ( unsigned int i ( 1 ); i < month (); ++i )
			n += daysInMonth ( i, year () );
	}
	return n;
}


// Source: http://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
unsigned int vmNumber::dayOfWeek () const
{
	if ( isDate () )
	{

		const unsigned int y ( year () - ( ( year () >= 2000 ) ? 2000 : 1900 ) );
		const unsigned int c ( ( year () >= 2000 ) ? 6 : 0 );

		const auto m_month ( (month () >= 1 && month () <= 12) ? month () : 1 );
		const auto dow ( ( day () + static_cast<unsigned int>(months_table[m_month]) + y + ( y / 4 ) + c ) % 7 );
		return dow;
	}
	return 0;
}

// Source: http://en.wikipedia.org/wiki/ISO_week_date
unsigned int vmNumber::weekNumber () const
{
	return ( ( dayOfYear () - dayOfWeek () ) + 10 ) / 7;
}

bool vmNumber::isDateWithinWeek ( const vmNumber& date ) const
{
	vmNumber tempDate ( date );
	tempDate.setDate ( -7, 0, 0, true );
	if ( *this >= tempDate )
	{
		tempDate.setDate ( 14, 0 ,0, true );
		return ( *this <= tempDate );
	}
	return false;
}

bool vmNumber::isLeapYear ( const unsigned int year )
{
	return ( ( year % 4 ) == 0 );
}

unsigned int vmNumber::daysInMonth ( const unsigned int month, const unsigned int year )
{
	unsigned int days ( 31 );
	switch ( month )
	{
		case VMNT_UNSET:
		break;
		case 2:
			days = isLeapYear ( year ) ? 29 : 28;
		break;
		case 4:
		case 6:
		case 9:
		case 11:
			days = 30;
		break;
	}
	return days;
}

const vmNumber& vmNumber::currentDate ()
{
	const time_t t ( time ( nullptr ) );   // get time now
	const struct tm* __restrict now ( localtime( &t ) );
	vmCurrentDateTime.nbr_upart[VM_IDX_DAY] = static_cast<unsigned int>( now->tm_mday );
	vmCurrentDateTime.nbr_upart[VM_IDX_MONTH] = static_cast<unsigned int>( now->tm_mon + 1 );
	vmCurrentDateTime.nbr_upart[VM_IDX_YEAR] = static_cast<unsigned int>( now->tm_year + 1900 );
	vmCurrentDateTime.mb_valid = true;
	vmCurrentDateTime.setType ( VMNT_DATE );
	return vmCurrentDateTime;
}
//-------------------------------------DATE------------------------------------------

//-------------------------------------PHONE-----------------------------------------

vmNumber& vmNumber::fromStrPhone ( const QString& phone )
{
	if ( !phone.isEmpty () )
	{		
		unsigned int idx ( VM_IDX_PREFIX );
		unsigned int tens ( 10 );
		unsigned int chr ( 0 );
		const auto len ( static_cast<unsigned int>( phone.length () ) );

		do
		{
			if ( phone.at ( static_cast<int>( chr ) ).isDigit () )
			{
				if ( idx > VM_IDX_PHONE2 )
				{
					break;
				}
				nbr_upart[idx] += static_cast<unsigned int>( phone.at ( static_cast<int>( chr ) ).digitValue () ) * tens;
				tens /= 10;
			
				switch ( idx )
				{
					case VM_IDX_PREFIX:
						if ( tens == 0 )
						{
							tens = 10000;
							idx++;
						}
					break;
					case VM_IDX_PHONE1:
						if ( tens == 1000 )
						{
							 if ( nbr_upart[VM_IDX_PHONE1] < 50000 )
							 {
								tens = 100;
								nbr_upart[VM_IDX_PHONE1] /= 10;
							 }
						}
						else if ( tens == 0 )
						{
							tens = 1000;
							idx++;
						}
					break;
					case VM_IDX_PHONE2:
					break;
				}
			}
		} while ( ++chr < len );

		setType ( VMNT_PHONE );
		setCached ( false );
		return *this;
	}
	clear ();
	return *this;
}

vmNumber& vmNumber::fromTrustedStrPhone ( const QString& phone, const bool cache )
{
	clear ( false );
	nbr_upart[VM_IDX_PREFIX] = retrievePhonePrefix ( phone, true ).toUInt ();
	const QString phone_body ( retrievePhoneBody ( phone, true ) );
	nbr_upart[VM_IDX_PHONE1] = phone_body.leftRef ( phone_body.count () == 8 ? 4 : 5 ).toUInt ();
	nbr_upart[VM_IDX_PHONE2] = phone_body.rightRef ( 4 ).toUInt ();
	setType ( VMNT_PHONE );
	if ( cache )
	{
		setCached ( true );
		cached_str = phone;
	}
	return *this;
}

const QString& vmNumber::retrievePhoneBody ( const QString& full_phone, const bool numbers_only ) const
{
	const int idx ( full_phone.indexOf ( CHR_SPACE ) );
	if ( idx != -1 )
	{
		mQString = full_phone.mid ( idx + 1, full_phone.length () - idx );
		if ( numbers_only )
			mQString.remove ( CHR_HYPHEN );
		return mQString;
	}
	return emptyString;
}

const QString& vmNumber::retrievePhonePrefix ( const QString& full_phone, const bool numbers_only ) const
{
	const int idx ( full_phone.indexOf ( CHR_R_PARENTHESIS ) );
	if ( idx != -1 )
	{
		mQString = full_phone.mid ( ( !numbers_only ? 0 : 1 ), ( !numbers_only ? idx + 1 : idx - 1 ) );
		return mQString;
	}
	return emptyString;
}

void vmNumber::makeCachedStringForPhone () const
{
	cached_str = makePhonePrefix () + CHR_SPACE + makePhoneBody () ;
	setCached ( true );
}

const QString& vmNumber::toPhone () const
{
	switch ( type () )
	{
		case VMNT_INT: // will not even attempt to convert these
		case VMNT_DOUBLE:
		case VMNT_PRICE:
		case VMNT_TIME:
		case VMNT_DATE:
			return emptyString;
		
		case VMNT_UNSET:
		case VMNT_PHONE:
			m_type = VMNT_PHONE;
			if ( !isCached () )
			{
				makeCachedStringForPhone ();
			}
		break;
	}
	return cached_str;
}

const QString& vmNumber::phoneBody () const
{
	if ( isPhone () )
	{
		if ( !isCached () )
			makeCachedStringForPhone ();
		return retrievePhoneBody ( cached_str );
	}
	return emptyString;
}

const QString& vmNumber::phonePrefix () const
{
	if ( isPhone () )
	{
		if ( !isCached () )
			makeCachedStringForPhone ();
		return retrievePhonePrefix ( cached_str );
	}
	return emptyString;
}

void vmNumber::makePhone ( const QString& prefix, const QString& body, const bool trusted )
{
	if ( trusted )
		fromTrustedStrPhone ( prefix + CHR_SPACE + body );
	else
		fromStrPhone ( prefix + body );
}

const QString vmNumber::makePhoneBody () const
{
	if ( nbr_upart[VM_IDX_PHONE1] > 0 )
	{
		if ( nbr_upart[VM_IDX_PHONE1] >= 5000 && nbr_upart[VM_IDX_PHONE1] < 10000 )
			mQString = QStringLiteral ( "9" );
		mQString += QString::number ( nbr_upart[VM_IDX_PHONE1] ) + CHR_HYPHEN;
	}

	if ( nbr_upart[VM_IDX_PHONE2] > 0 )
		mQString += QString::number ( nbr_upart[VM_IDX_PHONE2] );

	return mQString;
}

const QString vmNumber::makePhonePrefix () const
{
	mQString = CHR_L_PARENTHESIS + QString::number ( nbr_upart[VM_IDX_PREFIX] ) + CHR_R_PARENTHESIS;
	return mQString;
}
//-------------------------------------PHONE-----------------------------------------

//-------------------------------------PRICE-----------------------------------------
static void formatPrice ( QString& new_price, const int part1, const unsigned int part2 )
{
	new_price = CURRENCY + CHR_SPACE + QString::number ( part1 ) + CHR_COMMA +
				( part2 >= 10 ? QString::number ( part2 ) : CHR_ZERO + QString::number ( part2 )
				);
	if ( part1 < 0 )
	{
		new_price.remove ( CHR_HYPHEN );
		new_price.prepend ( CHR_L_PARENTHESIS );
		new_price += CHR_R_PARENTHESIS;
	}
}

static void formatPrice ( QString& new_price, const double n )
{
	new_price = CURRENCY + CHR_SPACE + QString::number ( n, 'f', 2 );
	new_price.replace ( CHR_DOT, CHR_COMMA );
	if ( n < 0.0 )
	{
		new_price.remove ( CHR_HYPHEN );
		new_price.prepend ( CHR_L_PARENTHESIS );
		new_price += CHR_R_PARENTHESIS;
	}
}

void vmNumber::setPrice ( const int tens, const int cents, const bool update )
{
	setType ( VMNT_PRICE );
	setCached ( false );
	if ( !update )
	{
		nbr_part[VM_IDX_TENS] = tens;
		nbr_part[VM_IDX_CENTS] = cents <= 99 ? cents : 0;
	}
	else
	{
		if ( tens != 0 || cents != 0 )
		{
			double price ( static_cast<double>(nbr_part[VM_IDX_TENS]) + static_cast<double>(nbr_part[VM_IDX_CENTS] / 100.0) );
			const double other_price ( static_cast<double>(tens) + static_cast<double>(cents / 100.0) );
			price += other_price;
			nbr_part[VM_IDX_TENS] = static_cast<int>(price);
			const double new_cents ( ( price - static_cast<double>(nbr_part[VM_IDX_TENS]) ) * 100.111 );
			nbr_part[VM_IDX_CENTS] = static_cast<int>(new_cents);
		}
	}
}

vmNumber& vmNumber::fromStrPrice ( const QString& price )
{
	if ( !price.isEmpty () )
	{
		QString simple_price ( price );
		simple_price.remove ( CHR_SPACE );
		unsigned int chr ( 0 ), idx ( VM_IDX_TENS );
		const auto len ( static_cast<unsigned int>(simple_price.length ()) );
		QChar qchr;
		bool is_negative ( false );
		nbr_part[VM_IDX_TENS] = nbr_part[VM_IDX_CENTS] = 0;
		nbr_upart[VM_IDX_CENTS] = 0;
		while ( chr < len )
		{
			qchr = simple_price.at ( static_cast<int>(chr) );
			if ( qchr.isDigit () )
			{
				if ( idx == VM_IDX_CENTS )
				{
					if ( nbr_upart[VM_IDX_CENTS] == 0 )
					{
						nbr_part[VM_IDX_CENTS] += qchr.digitValue ();
						nbr_part[VM_IDX_CENTS] *= 10;
						++nbr_upart[VM_IDX_CENTS];
					}
					else
					{
						nbr_part[VM_IDX_CENTS] += qchr.digitValue ();
						break; // up to two digits in the cents
					}
				}
				else
				{
					nbr_part[VM_IDX_TENS] *= 10;
					nbr_part[VM_IDX_TENS] += qchr.digitValue ();
				}
			}
			else if ( qchr == CHR_DOT || qchr == CHR_COMMA )
				idx = VM_IDX_CENTS;
			else if ( qchr == CHR_HYPHEN || qchr == CHR_L_PARENTHESIS )
			{
				if ( nbr_part[VM_IDX_TENS] == 0 ) // before any number, accept those chars, after, it is just garbage
					is_negative = true;
			}
			++chr;
		}
		if ( is_negative )
			nbr_part[VM_IDX_TENS] = 0 - nbr_part[VM_IDX_TENS];
		setType ( VMNT_PRICE );
		setCached ( false );
		return *this;
	}
	clear ( false );
	return *this;
}

vmNumber& vmNumber::fromTrustedStrPrice ( const QString& price, const bool cache )
{
	QString newStrPrice;
	if ( !price.isEmpty () )
	{
		clear ( false );
		const int idx_sep ( price.indexOf ( CHR_COMMA ) );
		const int idx_space ( price.indexOf ( CHR_SPACE ) );
		nbr_part[VM_IDX_TENS] = price.midRef ( idx_space + 1, idx_sep - idx_space - 1 ).toInt ();
		nbr_part[VM_IDX_CENTS] = price.rightRef ( 2 ).toInt ();
		if ( price.at ( 0 ) == CHR_L_PARENTHESIS ) // negative
			nbr_part[VM_IDX_TENS] = 0 - nbr_part[VM_IDX_TENS];
	}
	else
	{
		nbr_part[VM_IDX_TENS] = 0;
		nbr_part[VM_IDX_CENTS] = 0;
		formatPrice ( newStrPrice, 0, 0 );
	}
	setType ( VMNT_PRICE );
	if ( cache ) {
		setCached ( true );
		cached_str = newStrPrice.isEmpty () ? price : newStrPrice;
	}
	return *this;
}

const QString& vmNumber::priceToDoubleString () const
{
	if ( isPrice () )
	{
		if ( !isCached () )
		{
			mQString = CURRENCY + CHR_SPACE + QString::number ( nbr_part[VM_IDX_TENS] ) + CHR_COMMA +
						( nbr_part[VM_IDX_CENTS] >= 10 ? QString::number ( nbr_part[VM_IDX_TENS] ) : CHR_ZERO + QString::number ( nbr_part[VM_IDX_TENS] )
						);
			if ( nbr_part[VM_IDX_TENS] < 0 )
			{
				mQString.remove ( CHR_HYPHEN );
				mQString.prepend ( CHR_L_PARENTHESIS );
				mQString += CHR_R_PARENTHESIS;
			}
		}
		else
		{
			mQString.replace ( CHR_COMMA, CHR_DOT );
			mQString.remove ( CURRENCY );
		}
		return mQString;
	}
	return emptyString;
}

const QString& vmNumber::toPrice () const
{
	switch ( type () )
	{
		case VMNT_DATE: // will not even attempt to convert these
		case VMNT_PHONE:
		case VMNT_TIME:
			return emptyString;
	
		case VMNT_INT:
			formatPrice ( mQString, nbr_part[0], 0 );
		break;
		case VMNT_DOUBLE:
			formatPrice ( mQString, toDouble () );
		break;
		case VMNT_UNSET:
		case VMNT_PRICE:
			m_type = VMNT_PRICE;
			if ( !isCached () )
			{
				formatPrice ( cached_str, nbr_part[VM_IDX_TENS], static_cast<unsigned int>(nbr_part[VM_IDX_CENTS]) );
				setCached ( true );
			}
			return cached_str;
	}
	return mQString;
}
//-------------------------------------PRICE-----------------------------------------

//-------------------------------------TIME------------------------------------------
static void numberToTime ( const unsigned int n, int& hours, int& minutes )
{
	unsigned int abs_hour ( 0 );

	abs_hour = n / 60;
	minutes = n % 60;
	if ( minutes > 59 )
	{
		++abs_hour;
		minutes = 0;
	}
	if ( abs_hour > 9999 )
		hours = 9999;
	else if ( hours < 0 )
		hours = static_cast<int>(0 - abs_hour);
	else
		hours = static_cast<int>(abs_hour);
}

/*void getTimeFromFancyTimeString ( const QString& str_time, vmNumber& time, const bool check = false )
{

}*/

void vmNumber::setTime ( const int hours, const int minutes, const int seconds, const bool b_add , const bool b_reset_first  )
{
	if ( b_add )
	{
		if ( b_reset_first )
			nbr_part[VM_IDX_HOUR ] = nbr_part[VM_IDX_MINUTE ] = nbr_part[VM_IDX_SECOND ] = 0;

		nbr_part[VM_IDX_HOUR ] += hours;
		nbr_part[VM_IDX_MINUTE ] += minutes;
		nbr_part[VM_IDX_SECOND ] += seconds;
	}
	else
	{
		nbr_part[VM_IDX_HOUR ] = hours;
		nbr_part[VM_IDX_MINUTE ] = minutes;
		nbr_part[VM_IDX_SECOND ] = seconds;
	}
	fixTime ();
}

void vmNumber::fixTime ()
{
	setType ( VMNT_TIME );
	setCached ( false );
	
	int n ( 0 );
	if ( nbr_part[VM_IDX_SECOND] > 59 )
	{
		n = static_cast<int>( nbr_part[VM_IDX_SECOND] / 60 );
		nbr_part[VM_IDX_MINUTE] += n;
		nbr_part[VM_IDX_SECOND] -= ( n * 60 );
	}
	else if ( nbr_part[VM_IDX_SECOND] < 0 )
	{
		n = static_cast<int>( ::fabs ( static_cast<double>( nbr_part[VM_IDX_SECOND] ) / 60) ) + 1;
		nbr_part[VM_IDX_MINUTE] -= n;
		nbr_part[VM_IDX_SECOND] += ( n * 60 );
	}

	if ( nbr_part[VM_IDX_MINUTE] > 59 )
	{
		n = static_cast<int>( nbr_part[VM_IDX_MINUTE] / 60 );
		nbr_part[VM_IDX_HOUR] += n;
		nbr_part[VM_IDX_MINUTE] -= ( n * 60 );
	}
	else if ( nbr_part[VM_IDX_MINUTE] < 0 )
	{
		n = static_cast<int>( ::fabs ( static_cast<double>( nbr_part[VM_IDX_MINUTE] ) / 60) ) + 1;
		nbr_part[VM_IDX_HOUR] -= n;
		nbr_part[VM_IDX_MINUTE] += ( n * 60 );
	}

	// Do we need these limits??
	if ( ( nbr_part[VM_IDX_HOUR] ) > 10000 )
		nbr_part[VM_IDX_HOUR] = 10000;
	else if ( nbr_part[VM_IDX_HOUR] < -10000 )
		nbr_part[VM_IDX_HOUR] = -10000;
}

vmNumber& vmNumber::fromStrTime ( const QString& time )
{
	if ( !time.isEmpty () )
	{
		bool is_negative ( false );
		int hours ( 0 ), mins ( 0 ), secs ( 0 );
		int idx ( time.indexOf ( CHR_COLON ) );
		if ( idx != -1 )
		{
			QString temp_time ( time );
			if ( temp_time.remove ( CHR_SPACE ).startsWith ( CHR_HYPHEN ) )
			{
				is_negative = true;
				temp_time.remove ( CHR_HYPHEN );
			}
			hours = temp_time.leftRef ( idx ).toInt ();
			mins = temp_time.midRef ( idx + 1, 2 ).toInt ();
			
			idx = time.indexOf ( CHR_COLON, idx + 1 );
			if ( idx != -1 )
				secs = temp_time.midRef ( idx + 1, 2 ).toInt ();
			
			nbr_part[VM_IDX_HOUR] = !is_negative ? hours : 0 - hours;
			nbr_part[VM_IDX_MINUTE] = mins;
			nbr_part[VM_IDX_SECOND] = secs;
			
			hours = static_cast<int>( ::fabs ( static_cast<double>( hours ) ) );
			if ( hours == 24 && (( mins > 0 ) || ( secs > 0 )) )
				nbr_part[VM_IDX_STRFORMAT] = VTF_DAYS;
			if ( hours > 24 )
				nbr_part[VM_IDX_STRFORMAT] = VTF_DAYS;
			else
				nbr_part[VM_IDX_STRFORMAT] = VTF_24_HOUR;
			
			setType ( VMNT_TIME );
			setCached ( false );
			return *this;
		}
		//else
		//{
			//	getTimeFromFancyTimeString ( time, mQString, true );
		//}
	}
	clear ();
	return *this;
}

vmNumber& vmNumber::fromTrustedStrTime ( const QString& time, const VM_TIME_FORMAT format, const bool cache )
{
	clear ( false );
	if ( !time.isEmpty () )
	{
		switch ( format )
		{
			case VTF_24_HOUR:
				nbr_part[VM_IDX_HOUR] = time.leftRef ( 2 ).toInt ();
				if ( time.length () < 8 )
					nbr_part[VM_IDX_MINUTE] = time.rightRef ( 2 ).toInt ();
				else
				{
					nbr_part[VM_IDX_MINUTE] = time.midRef ( 3, 2 ).toInt ();
					nbr_part[VM_IDX_SECOND] = time.rightRef ( 2 ).toInt ();
				}
			break;
				
			case VTF_DAYS:
				nbr_part[VM_IDX_HOUR] = time.leftRef ( 4 ).toInt ();
				if ( time.length () < 10 )
					nbr_part[VM_IDX_MINUTE] = time.rightRef ( 2 ).toInt ();
				else
				{
					nbr_part[VM_IDX_MINUTE] = time.midRef ( 5, 2 ).toInt ();
					nbr_part[VM_IDX_SECOND] = time.rightRef ( 2 ).toInt ();
				}
			break;
				
			case VTF_FANCY:
				//getTimeFromFancyTimeString ( time, mQString );
			break;
		}
		nbr_part[VM_IDX_STRFORMAT] = format;
		setType ( VMNT_TIME );
		if ( cache )
		{
			setCached ( true );
			cached_str = time;
		}
	}
	return *this;
}

void vmNumber::fromQTime ( const QTime time )
{
	if ( !isTime () )
	{
		nbr_part[VM_IDX_HOUR] = nbr_part[VM_IDX_MINUTE] = nbr_part[VM_IDX_SECOND] = 0;
		setType ( VMNT_TIME );
	}
	nbr_part[VM_IDX_HOUR] = time.hour ();
	nbr_part[VM_IDX_MINUTE] = time.minute ();
	nbr_part[VM_IDX_SECOND] = time.second ();
	setCached ( false );
}

const QString& vmNumber::toTime ( const VM_TIME_FORMAT format ) const
{
	int hours ( 0 );
	int minutes ( 0 );
	switch ( type () )
	{
		case VMNT_PRICE: // will not even attempt to convert these
		case VMNT_DATE:
		case VMNT_PHONE:
			return emptyString;
	
		case VMNT_INT:
			numberToTime ( static_cast<unsigned int>(nbr_part[0]), hours, minutes );
		break;
		case VMNT_DOUBLE:
			numberToTime ( static_cast<unsigned int>(toDouble ()), hours, minutes );
		break;
		case VMNT_UNSET:
		case VMNT_TIME:
			m_type = VMNT_TIME;
			if ( !isCached () || format != nbr_part[VM_IDX_STRFORMAT] )
			{
				cached_str = formatTime ( this->hours (), this->minutes (), this->seconds (), format );
				setCached ( true );
				nbr_part[VM_IDX_STRFORMAT] = format;
			}
			return cached_str;
	}
	return formatTime ( hours, minutes, 0, format );
}

const QTime vmNumber::toQTime () const
{
	if ( isTime () )
		mQTime.setHMS ( hours (), minutes (), seconds () );
	else
		mQTime.setHMS ( 0, 0, 0 );
	return mQTime;
}

const QString& vmNumber::formatTime ( const int hour, const int min, const int sec, const VM_TIME_FORMAT format ) const
{
	auto abs_hour ( static_cast<unsigned int>( ::fabs ( static_cast<double>( hour ) ) ) );
	abs_hour += static_cast<unsigned int>( ( min / 60 ) );
	const int abs_min ( min - ( min / 60 ) * 60 );
	const int abs_sec ( sec - ( sec / 60 ) * 60 );
	QString str_hour, str_min, str_sec;
	str_hour.setNum ( abs_hour );
	str_min.setNum ( abs_min );
	str_sec.setNum ( abs_sec );
	
	if ( format != VTF_FANCY )
	{
		if ( abs_min < 10  )
			str_min.prepend ( CHR_ZERO );
		if ( abs_sec < 10 )
			str_sec.prepend ( CHR_ZERO );
	}

	switch ( format )
	{
		case VTF_24_HOUR:
			if ( abs_hour > 24 )
				str_hour = QStringLiteral ( "23" );
			else if ( abs_hour < 10 )
				str_hour.prepend ( CHR_ZERO );
			mQString = str_hour + CHR_COLON + str_min;
		break;
		case VTF_DAYS:
			if ( abs_hour < 10 )
				str_hour.prepend ( QStringLiteral ( "000" ) );
			else if ( abs_hour < 100 )
				str_hour.prepend ( QStringLiteral ( "00" ) );
			else if ( abs_hour < 1000 )
				str_hour.prepend ( CHR_ZERO );
			mQString = str_hour + CHR_COLON + str_min + CHR_COLON + str_sec;
		break;
		case VTF_FANCY:
			if ( abs_hour >= 24 )
			{
				const unsigned int abs_days ( abs_hour / 24 );
				abs_hour %= 24;
				str_hour.setNum ( abs_hour );
				mQString.setNum ( abs_days );
				mQString += QStringLiteral ( " days, ") + str_hour + QStringLiteral ( " hours, ")
									+ str_min + QStringLiteral ( " minutes, and " ) + str_sec + QStringLiteral ( " seconds" );
			}
			else
			{
				mQString = str_hour + QStringLiteral ( " hours, ") + str_min + QStringLiteral ( " minutes, and " )
									+ str_sec + QStringLiteral ( " seconds" );
			}
		break;
	}

	if ( hour < 0 )
	{
		if ( format != VTF_FANCY )
			mQString.prepend ( CHR_HYPHEN );
		else
			mQString.prepend ( QStringLiteral ( "Negative " ) );
	}
	return mQString;
}

const vmNumber& vmNumber::currentTime ()
{
	const time_t t ( time ( nullptr ) );   // get time now
	const struct tm* __restrict now ( localtime( &t ) );
	vmCurrentDateTime.nbr_part[VM_IDX_HOUR] = now->tm_hour;
	vmCurrentDateTime.nbr_part[VM_IDX_MINUTE] = now->tm_min;
	vmCurrentDateTime.nbr_part[VM_IDX_SECOND] = now->tm_sec;
	vmCurrentDateTime.mb_valid = true;
	vmCurrentDateTime.setType ( VMNT_TIME );
	return vmCurrentDateTime;
}
//-------------------------------------TIME------------------------------------------

//-------------------------------------OPERATORS------------------------------------------
vmNumber& vmNumber::operator= ( const QDate date )
{
	static_cast<void>(fromQDate ( date ));
	return *this;
}

vmNumber& vmNumber::operator= ( const int n )
{
	static_cast<void>(fromInt ( n ));
	return *this;
}

vmNumber& vmNumber::operator= ( const unsigned int n )
{
	static_cast<void>(fromUInt ( n ));
	return *this;
}

vmNumber& vmNumber::operator= ( const double n )
{
	static_cast<void>(fromDoubleNbr ( n ));
	return *this;
}

bool vmNumber::operator== ( const vmNumber& vmnumber ) const
{
	if ( m_type == vmnumber.m_type )
	{
		switch ( type () )
		{
			case VMNT_UNSET:
				return true;
			case VMNT_INT:
				return nbr_part[0] == vmnumber.nbr_part[0];
			case VMNT_DOUBLE:
			case VMNT_PRICE:
				return ( nbr_part[0] == vmnumber.nbr_part[0] && nbr_part[1] == vmnumber.nbr_part[1] );
			case VMNT_DATE:
			case VMNT_PHONE:
			case VMNT_TIME:
				return ( nbr_upart[0] == vmnumber.nbr_upart[0] && nbr_upart[1] == vmnumber.nbr_upart[1]
					 && nbr_upart[2] == vmnumber.nbr_upart[2] );
		}
	}
	return false;
}

bool vmNumber::operator== ( const int number ) const
{
	switch ( type () )
	{
		case VMNT_DATE:
		case VMNT_PHONE:
		case VMNT_UNSET:
			return false;
			
		case VMNT_INT:
			return nbr_part[0] == number;
			
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			return nbr_part[0] == number && nbr_part[1] == 0;
			
		case VMNT_TIME:
			return ( (hours () + minutes () + seconds ()) == number );
	}
	return false;
}

bool vmNumber::operator!= ( const vmNumber& vmnumber ) const
{
	return !operator== ( vmnumber );
}

bool vmNumber::operator>= ( const vmNumber& vmnumber ) const
{
	bool ret ( false );
	switch ( type () )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					ret = nbr_part[0] >= vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					ret = nbr_part[0] >= static_cast<int> ( vmnumber.nbr_upart[0] );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
					ret = nbr_part[0] >= vmnumber.nbr_part[0];
				break;
				case VMNT_PHONE:
				case VMNT_DATE:
					if ( nbr_part[0] == static_cast<int> ( vmnumber.nbr_upart[0] ) )
						ret = nbr_part[1] >= static_cast<int> ( vmnumber.nbr_upart[1] );
					else
						ret = nbr_part[0] > static_cast<int> ( vmnumber.nbr_upart[0] );
				break;
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					if ( nbr_part[0] >= vmnumber.nbr_part[0] )
					{
						if ( nbr_part[0] == vmnumber.nbr_part[0] )
							ret = nbr_part[1] >= vmnumber.nbr_part[1];
						else
							ret = true;
					}
				break;
				case VMNT_TIME:
					if ( hours () >= vmnumber.hours () )
					{
						if ( hours () == vmnumber.hours () )
						{
							if ( minutes () >= vmnumber.minutes () )
							{
								if ( minutes () == vmnumber.minutes () )
									ret = seconds () >= vmnumber.seconds ();
								else
									ret = true;
							}
						}
						else
							ret = true;
					}
				break;
				case VMNT_UNSET:
				break;
			}
		break;
		case VMNT_DATE:
		case VMNT_PHONE:
			if ( ( vmnumber.m_type == VMNT_DATE ) || ( vmnumber.m_type == VMNT_PHONE ) )
			{
				if ( nbr_upart[2] >= vmnumber.nbr_upart[2] )
				{
					if ( nbr_upart[2] == vmnumber.nbr_upart[2] )
					{
						if ( nbr_upart[1] >= vmnumber.nbr_upart[1] )
						{
							if ( nbr_upart[1] == vmnumber.nbr_upart[1] )
								ret = nbr_upart[0] >= vmnumber.nbr_upart[0];
							else // month is greater than
								ret = true;
						}
					}
					else // year is greater than
						ret = true;
				}
			}
		break;
		case VMNT_UNSET:
		break;
	}
	return ret;
}

bool vmNumber::operator<= ( const vmNumber& vmnumber ) const
{
	bool ret ( false );
	switch ( type () ) {
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					ret = nbr_part[0] <= vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					ret = nbr_part[0] <= static_cast<int> ( vmnumber.nbr_upart[0] );
				break;
				case VMNT_PHONE:
				case VMNT_UNSET:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
					ret = nbr_part[0] <= vmnumber.nbr_part[0];
				break;
				case VMNT_PHONE:
				case VMNT_DATE:
					if ( nbr_part[0] == static_cast<int>(vmnumber.nbr_upart[0] ))
						ret = nbr_part[1] <= static_cast<int>(vmnumber.nbr_upart[1]);
					else
						ret = nbr_part[0] < static_cast<int>(vmnumber.nbr_upart[0]);
				break;
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					if ( nbr_part[0] <= vmnumber.nbr_part[0] )
					{
						if ( nbr_part[0] == vmnumber.nbr_part[0] )
							ret = nbr_part[1] <= vmnumber.nbr_part[1];
						else
							ret = true;
					}
				break;
				case VMNT_TIME:
					if ( hours () <= vmnumber.hours () )
					{
						if ( hours () == vmnumber.hours () )
						{
							if ( minutes () <= vmnumber.minutes () )
							{
								if ( minutes () == vmnumber.minutes () )
									ret = seconds () <= vmnumber.seconds ();
								else
									ret = true;
							}
						}
						else
							ret = true;
					}
				break;
				case VMNT_UNSET:
				break;
			}
		break;
		case VMNT_DATE:
		case VMNT_PHONE:
			if ( ( vmnumber.m_type == VMNT_DATE ) || ( vmnumber.m_type == VMNT_PHONE ) )
			{
				if ( nbr_upart[2] <= vmnumber.nbr_upart[2] )
				{
					if ( nbr_upart[2] == vmnumber.nbr_upart[2] )
					{
						if ( nbr_upart[1] <= vmnumber.nbr_upart[1] )
						{
							if ( nbr_upart[1] == vmnumber.nbr_upart[1] )
								ret = nbr_upart[0] <= vmnumber.nbr_upart[0];
							else // month is less than
								ret = true;
						}
					}
					else // year is less than
						ret = true;
				}
			}
		break;
		case VMNT_UNSET:
		break;
	}
	return ret;
}

bool vmNumber::operator< ( const vmNumber& vmnumber ) const
{
	bool ret ( false );
	switch ( type () )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					ret = nbr_part[0] < vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					ret = nbr_part[0] < static_cast<int>(vmnumber.nbr_upart[0]);
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
					ret = nbr_part[0] < vmnumber.nbr_part[0];
				break;
				case VMNT_PHONE:
				case VMNT_DATE:
					if ( nbr_part[0] == static_cast<int>( vmnumber.nbr_upart[0] ) )
						ret = nbr_part[1] < static_cast<int>( vmnumber.nbr_upart[1] );
					else
						ret = nbr_part[0] < static_cast<int>( vmnumber.nbr_upart[0] );
				break;
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					if ( nbr_part[0] == vmnumber.nbr_part[0] )
						ret = nbr_part[1] < vmnumber.nbr_part[1];
					else
						ret = nbr_part[0] < vmnumber.nbr_part[0];
				break;
				case VMNT_TIME:
					if ( hours () <= vmnumber.hours () )
					{
						if ( hours () == vmnumber.hours () )
						{
							if ( minutes () <= vmnumber.minutes () )
							{
								if ( minutes () == vmnumber.minutes () )
									ret = seconds () < vmnumber.seconds ();
								else
									ret = true;
							}
						}
						else
							ret = true;
					}
				break;
				case VMNT_UNSET:
				break;
			}
		break;
		case VMNT_DATE:
		case VMNT_PHONE:
			if ( ( vmnumber.m_type == VMNT_DATE ) || ( vmnumber.m_type == VMNT_PHONE ) )
			{
				if ( nbr_upart[2] <= vmnumber.nbr_upart[2] )
				{
					if ( nbr_upart[2] == vmnumber.nbr_upart[2] )
					{
						if ( nbr_upart[1] <= vmnumber.nbr_upart[1] )
						{
							if ( nbr_upart[1] == vmnumber.nbr_upart[1] )
								ret = nbr_upart[0] < vmnumber.nbr_upart[0];
							else // month is less than
								ret = true;
						}
					}
					else // year is less than
						ret = true;
				}
			}
		break;
		case VMNT_UNSET:
		break;
	}
	return ret;
}

bool vmNumber::operator< ( const int number ) const
{
	switch ( type () )
	{
		case VMNT_PRICE:
		case VMNT_DOUBLE:
		case VMNT_INT:
		case VMNT_TIME:
			return nbr_part[0] < number;
	
		case VMNT_PHONE:
		case VMNT_DATE:
			return static_cast<int>(nbr_upart[0]) < number;
			
		case VMNT_UNSET:
			return false;
	}
	return false;
}

bool vmNumber::operator> ( const int number ) const
{
	switch ( type () )
	{
		case VMNT_PRICE:
		case VMNT_DOUBLE:
		case VMNT_INT:
			return nbr_part[0] > number;
	
		case VMNT_TIME:
			return ( (hours () + minutes () + seconds ()) > number );
		
		case VMNT_PHONE:
		case VMNT_DATE:
			return static_cast<int>(nbr_upart[0]) > number;
			
		case VMNT_UNSET:
			return false;
	}
	return false;
}

bool vmNumber::operator> ( const vmNumber& vmnumber ) const
{
	bool ret ( false );
	switch ( type () )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					ret = nbr_part[0] > vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					ret = nbr_part[0] > static_cast<int>(vmnumber.nbr_upart[0]);
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
					ret = nbr_part[0] > vmnumber.nbr_part[0];
				break;
				case VMNT_PHONE:
				case VMNT_DATE:
					if ( nbr_part[0] == static_cast<int>( vmnumber.nbr_upart[0] ) )
						ret = nbr_part[1] > static_cast<int>( vmnumber.nbr_upart[1] );
					else
						ret = nbr_part[0] > static_cast<int>( vmnumber.nbr_upart[0] );
				break;
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					if ( nbr_part[0] == vmnumber.nbr_part[0] )
						ret = nbr_part[1] > vmnumber.nbr_part[1];
					else
						ret = nbr_part[0] > vmnumber.nbr_part[0];
				break;
				case VMNT_TIME:
					if ( hours () >= vmnumber.hours () )
					{
						if ( hours () == vmnumber.hours () )
						{
							if ( minutes () >= vmnumber.minutes () )
							{
								if ( minutes () == vmnumber.minutes () )
									ret = seconds () > vmnumber.seconds ();
								else
									ret = true;
							}
						}
						else
							ret = true;
					}
				break;
				case VMNT_UNSET:
				break;
			}
		break;
		case VMNT_DATE:
		case VMNT_PHONE:
			if ( ( vmnumber.m_type == VMNT_DATE ) || ( vmnumber.m_type == VMNT_PHONE ) )
			{
				if ( nbr_upart[2] >= vmnumber.nbr_upart[2] )
				{
					if ( nbr_upart[2] == vmnumber.nbr_upart[2] )
					{
						if ( nbr_upart[1] >= vmnumber.nbr_upart[1] )
						{
							if ( nbr_upart[1] == vmnumber.nbr_upart[1] )
								ret = nbr_upart[0] > vmnumber.nbr_upart[0];
							else // month is less than
								ret = true;
						}
					}
					else // year is less than
						ret = true;
				}
			}
		break;
		case VMNT_UNSET:
		break;
	}
	return ret;
}

vmNumber& vmNumber::operator-= ( const vmNumber& vmnumber )
{
	if ( vmnumber.isNull () )
		return *this;

	mb_cached = false;
	cached_str.clear ();
	if ( m_type == VMNT_UNSET )
		m_type = vmnumber.m_type;

	switch ( type () )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					nbr_part[0] -= vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					nbr_part[0] -= static_cast<int>( vmnumber.nbr_upart[0] );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					nbr_part[0] -= vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					nbr_part[0] -= static_cast<int>( vmnumber.nbr_upart[0] );
				break;
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					nbr_part[0] -= vmnumber.nbr_part[0];
					nbr_part[1] -= vmnumber.nbr_part[1];
					if ( nbr_part[1] > 0 )
					{
						if ( nbr_part[0] < 0 )
							++nbr_part[0];
					}
					else if ( nbr_part[1] < 0 )
					{
						if ( nbr_part[0] > 0 )
							--nbr_part[0];
						nbr_part[1] = 0 - nbr_part[1]; // decimal part cannot be negative
					}
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_PRICE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					nbr_part[0] -= vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					nbr_part[0] -= static_cast<int>( vmnumber.nbr_upart[0] );
				break;
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					setPrice ( 0 - vmnumber.nbr_part[0], 0 - vmnumber.nbr_part[1], true );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					setTime ( 0, 0, (hours () + minutes () + seconds ()) - vmnumber.nbr_part[0], false, true );
				break;
				case VMNT_TIME:
					setTime ( 0 - vmnumber.hours (), 0 - vmnumber.minutes (), 0 - vmnumber.seconds (), true, false );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				case VMNT_DATE:
				break;
			}
		break;
		case VMNT_DATE:
			if ( vmnumber.m_type == VMNT_DATE )
				setDate ( static_cast<int>( 0 - vmnumber.nbr_upart[0] ), static_cast<int>( 0 - vmnumber.nbr_upart[1] ),
								static_cast<int>( 0 - vmnumber.nbr_upart[2] ), true );
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		break;
	}
	return *this;
}

vmNumber& vmNumber::operator+= ( const vmNumber& vmnumber )
{
	if ( vmnumber.isNull () )
		return *this;

	mb_cached = false;
	cached_str.clear ();
	if ( m_type == VMNT_UNSET )
		m_type = vmnumber.m_type;

	switch ( type () )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					nbr_part[0] += vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					nbr_part[0] += static_cast<int>( vmnumber.nbr_upart[0] );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					nbr_part[0] += vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					nbr_part[0] += static_cast<int>( vmnumber.nbr_upart[0] );
				break;
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					nbr_part[0] += vmnumber.nbr_part[0];
					nbr_part[1] += vmnumber.nbr_part[1];
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_PRICE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					nbr_part[0] += vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					nbr_part[0] += static_cast<int>( vmnumber.nbr_upart[0] );
				break;
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					setPrice ( vmnumber.nbr_part[VM_IDX_TENS], vmnumber.nbr_part[VM_IDX_CENTS], true );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					setTime ( 0, 0, (hours () + minutes () + seconds ()) + vmnumber.nbr_part[0], false, true );
				break;
				case VMNT_TIME:
				case VMNT_DATE:
					setTime ( vmnumber.hours (), vmnumber.minutes (), vmnumber.seconds (), true, false );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DATE:
			if ( vmnumber.m_type == VMNT_DATE )
				setDate ( static_cast<int>( vmnumber.nbr_upart[0] ), static_cast<int>( vmnumber.nbr_upart[1] ),
								static_cast<int>( vmnumber.nbr_upart[2] ), true );
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		break;
	}
	return *this;
}

vmNumber& vmNumber::operator+= ( const double number )
{
	if ( number == 0.0 )
		return *this;

	mb_cached = false;
	cached_str.clear ();

	switch ( type () )
	{
		case VMNT_INT:
			nbr_part[0] += static_cast<int>( number );
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
		{
			const auto tens ( static_cast<int>( number ) );
			const double cents ( ( number - static_cast<double>( number ) ) * 100.111 );
			setPrice ( tens, static_cast<int>( cents ), true );
		}
		break;
		case VMNT_TIME:
			setTime ( 0, 0, (hours () + minutes () + seconds ()) + static_cast<int>( number ), false, true );
		break;
		case VMNT_DATE:
			setDate ( static_cast<int>( number ), 0, 0, true );
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		break;
	}
	return *this;
}

vmNumber& vmNumber::operator/= ( const vmNumber& vmnumber )
{
	if ( vmnumber.isNull () )
		return *this;

	mb_cached = false;
	cached_str.clear ();

	switch ( type () )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					if ( vmnumber.nbr_part[0] != 0 )
					{
						setType ( vmnumber.m_type );
						nbr_part[0] /= vmnumber.nbr_part[0];
						nbr_part[1] = vmnumber.nbr_part[1];
						nbr_part[2] = vmnumber.nbr_part[2];
					}
				break;
				case VMNT_DATE: // full convertion
					if ( vmnumber.nbr_upart[0] != 0 )
					{
						setType ( vmnumber.m_type );
						nbr_upart[0] /= vmnumber.nbr_upart[0];
						nbr_upart[1] = vmnumber.nbr_upart[1];
						nbr_upart[2] = vmnumber.nbr_upart[2];
					}
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					if ( vmnumber.nbr_part[0] != 0 )
						nbr_part[0] /= vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					if ( ( vmnumber.nbr_upart[0] ) != 0 && ( vmnumber.nbr_upart[1] != 0 ) )
					{
						nbr_part[0] /= static_cast<int>( vmnumber.nbr_upart[0] );
						nbr_part[1] /= static_cast<int>( vmnumber.nbr_upart[1] );
					}
				break;
				case VMNT_DOUBLE:
					if ( ( vmnumber.nbr_part[0] ) != 0 && ( vmnumber.nbr_part[1] != 0 ) )
						fromTrustedStrDouble ( useCalc ( vmnumber, *this, QStringLiteral ( " / " ) ) );
				break;
				case VMNT_PRICE:
					if ( ( vmnumber.nbr_part[0] ) != 0 && ( vmnumber.nbr_part[1] != 0 ) )
						fromStrPrice ( useCalc ( vmnumber, *this, QStringLiteral ( " / " ) ) );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					setTime ( 0, 0, (hours () + minutes () + seconds ()) / vmnumber.nbr_part[0], false, true );
				break;
				case VMNT_TIME:
					setTime ( hours () / vmnumber.hours (), minutes () / vmnumber.minutes (), seconds () / vmnumber.seconds (), false, true );
				break;
				case VMNT_UNSET:
				case VMNT_DATE:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_UNSET:
		case VMNT_DATE:
		case VMNT_PHONE:
		break;
	}
	return *this;
}

vmNumber& vmNumber::operator/= ( const int number )
{
	if ( number == 0 )
		return *this;

	mb_cached = false;
	cached_str.clear ();

	switch ( type () )
	{
		case VMNT_INT:
			nbr_part[0] /= number;
		break;
		case VMNT_DOUBLE:
			fromTrustedStrDouble ( useCalc ( vmNumber ( number ), *this, QStringLiteral ( " / " ) ) );
		break;
		case VMNT_PRICE:
			fromStrPrice ( useCalc ( vmNumber ( number ), *this, QStringLiteral ( " / " ) ) );
		break;
		case VMNT_TIME:
			setTime ( 0, 0, (hours () + minutes () + seconds ()) / number, false, true );
		break;
		case VMNT_DATE:
			setDate ( static_cast<int>( nbr_part[VM_IDX_DAY] / number ),
				  static_cast<int>( nbr_part[VM_IDX_MONTH] / number ), static_cast<int>( nbr_part[VM_IDX_YEAR] / number ) );
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		break;
	}
	return *this;
}

vmNumber& vmNumber::operator*= ( const vmNumber& vmnumber )
{
	mb_cached = false;
	cached_str.clear ();
	if ( m_type == VMNT_UNSET )
		m_type = vmnumber.m_type;

	switch ( type () )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					setType ( vmnumber.m_type );
					nbr_part[0] *= vmnumber.nbr_part[0];
					nbr_part[1] = vmnumber.nbr_part[1];
					nbr_part[2] = vmnumber.nbr_part[2];
				break;
				case VMNT_DATE: // full convertion
					setType ( vmnumber.m_type );
					nbr_upart[0] *= vmnumber.nbr_upart[0];
					nbr_upart[1] = vmnumber.nbr_upart[1];
					nbr_upart[2] = vmnumber.nbr_upart[2];
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			switch ( vmnumber.type () )
			{
				case VMNT_INT:
				case VMNT_TIME:
					nbr_part[0] *= vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					nbr_part[0] *= static_cast<int> ( vmnumber.nbr_upart[0] );
					nbr_part[1] *= static_cast<int> ( vmnumber.nbr_upart[1] );
				break;
				case VMNT_PRICE:
					fromStrPrice ( useCalc ( vmnumber, *this, QStringLiteral ( " / " ) ) );
				break;
				case VMNT_DOUBLE:
					fromTrustedStrDouble ( useCalc ( vmnumber, *this, QStringLiteral ( " / " ) ) );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_TIME:
			switch ( vmnumber.type () )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					setTime ( 0, 0, (hours () + minutes () + seconds ()) * vmnumber.nbr_part[0], false, true );
				break;
				case VMNT_TIME:
					setTime ( hours () * vmnumber.hours (), minutes () * vmnumber.minutes (), seconds () * vmnumber.seconds (), false, true );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				case VMNT_DATE:
				break;
			}
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		case VMNT_DATE:
		break;
	}
	return *this;
}

vmNumber& vmNumber::operator*= ( const int number )
{
	mb_cached = false;
	cached_str.clear ();

	switch ( type () )
	{
		case VMNT_INT:
			nbr_part[0] *= number;
		break;
		case VMNT_PRICE:
			fromStrPrice ( useCalc ( vmNumber ( number ), *this, QStringLiteral ( " * " ) ) );
		break;
		case VMNT_DOUBLE:
			fromTrustedStrDouble ( useCalc ( vmNumber ( number ), *this, QStringLiteral ( " * " ) ) );
		break;
		case VMNT_DATE:
			setDate ( static_cast<int>(nbr_upart[VM_IDX_DAY]) * number,
				  static_cast<int>(nbr_upart[VM_IDX_MONTH]) * number, static_cast<int>(nbr_upart[VM_IDX_YEAR]) * number );
		break;
		case VMNT_TIME:
			setTime ( 0, 0, (hours () + seconds () + minutes ()) * number, false, true );
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		break;
	}
	return *this;
}

/*  In the following operator () functions, the type of the returned instance is set accordingly to
	an implicit rule of type precedence. Basic numeric types have lower precedence than complex types.
	The precedence order is thus:
	( int = unsigned int ) < double < the rest
*/

vmNumber vmNumber::operator- ( const vmNumber& vmnumber ) const
{
	if ( vmnumber.isNull () )
		return *this;

	vmNumber ret ( *this );
	ret.setCached ( false );

	switch ( ret.m_type )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					ret.m_type = vmnumber.m_type;
					ret.nbr_part[0] -= vmnumber.nbr_part[0];
					ret.nbr_part[1] = vmnumber.nbr_part[1];
					ret.nbr_part[2] = vmnumber.nbr_part[2];
				break;
				case VMNT_DATE: // full convertion
					ret.m_type = vmnumber.m_type;
					ret.nbr_upart[0] -= vmnumber.nbr_upart[0];
					ret.nbr_upart[1] = vmnumber.nbr_upart[1];
					ret.nbr_upart[2] = vmnumber.nbr_upart[2];
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					ret.nbr_part[0] = nbr_part[0] - vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					if ( ret.m_type == VMNT_DOUBLE )
					{
						ret.m_type = vmnumber.m_type;
						ret.nbr_upart[0] = static_cast<unsigned int>( ret.nbr_part[0] ) - vmnumber.nbr_upart[0];
						ret.nbr_upart[1] = static_cast<unsigned int>( ret.nbr_part[1] ) - vmnumber.nbr_upart[1];
						ret.nbr_upart[2] = vmnumber.nbr_upart[2];
					}
					else
					{
						ret.nbr_part[0] -= static_cast<int>( vmnumber.nbr_upart[0] );
						ret.nbr_part[1] -= static_cast<int>( vmnumber.nbr_upart[1] );
					}
				break;
				case VMNT_PRICE:
				case VMNT_DOUBLE:
					if ( vmnumber.m_type == VMNT_PRICE )
					{
						if ( ret.m_type == VMNT_DOUBLE )
							ret.m_type = VMNT_PRICE;	
					}
					ret.nbr_part[0] -= vmnumber.nbr_part[0];
					ret.nbr_part[1] -= vmnumber.nbr_part[1];
					if ( ret.nbr_part[1] > 0 )
					{
						if ( ret.nbr_part[0] < 0 )
						++ret.nbr_part[0];
					}
					else if ( ret.nbr_part[1] < 0 )
					{
						if ( ret.nbr_part[0] > 0 )
							--ret.nbr_part[0];
						ret.nbr_part[1] = 0 - ret.nbr_part[1]; // decimal part cannot be negative
					}
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					ret.setTime ( 0, 0, (ret.hours () + ret.minutes () + ret.seconds ()) - vmnumber.nbr_part[0], false, true );
				break;
				case VMNT_TIME:
					ret.setTime ( 0 - vmnumber.hours (), 0 - vmnumber.minutes (), 0 - vmnumber.seconds (), true, false );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				case VMNT_DATE:
				break;
			}
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		case VMNT_DATE:
		break;
	}
	return ret;
}

vmNumber vmNumber::operator+ ( const vmNumber& vmnumber ) const
{
	vmNumber ret ( *this );
	ret.setCached ( false );

	switch ( ret.m_type )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					ret.m_type = vmnumber.m_type;
					ret.nbr_part[0] += vmnumber.nbr_part[0];
					ret.nbr_part[1] = vmnumber.nbr_part[1];
					ret.nbr_part[2] = vmnumber.nbr_part[2];
				break;
				case VMNT_DATE: // full convertion
					ret.m_type = vmnumber.m_type;
					ret.nbr_upart[0] += vmnumber.nbr_upart[0];
					ret.nbr_upart[1] = vmnumber.nbr_upart[1];
					ret.nbr_upart[2] = vmnumber.nbr_upart[2];
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					ret.nbr_part[0] = nbr_part[0] + vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					if ( ret.m_type == VMNT_DOUBLE )
					{
						ret.m_type = vmnumber.m_type;
						ret.nbr_upart[0] = static_cast<unsigned int> ( ret.nbr_part[0] ) + vmnumber.nbr_upart[0];
						ret.nbr_upart[1] = static_cast<unsigned int> ( ret.nbr_part[1] ) + vmnumber.nbr_upart[1];
						ret.nbr_upart[2] = vmnumber.nbr_upart[2];
					}
					else
					{
						ret.nbr_part[0] += static_cast<int> ( vmnumber.nbr_upart[0] );
						ret.nbr_part[1] += static_cast<int> ( vmnumber.nbr_upart[1] );
					}
				break;
				case VMNT_PRICE:
					ret.fromStrPrice ( useCalc ( vmnumber, ret, QStringLiteral ( " + " ) ) );
				break;
				case VMNT_DOUBLE:
					ret.fromTrustedStrDouble ( useCalc ( vmnumber, ret, QStringLiteral ( " + " ) ) );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					ret.setTime ( 0, 0, (ret.hours () + ret.minutes () + ret.seconds ()) + vmnumber.nbr_part[0], false, true );
				break;
				case VMNT_TIME:
					ret.setTime ( vmnumber.hours (), vmnumber.minutes (), vmnumber.seconds (), true, false );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				case VMNT_DATE:
				break;
			}
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		case VMNT_DATE:
		break;
	}
	return ret;
}

vmNumber vmNumber::operator/ ( const vmNumber& vmnumber ) const
{
	vmNumber ret ( *this );
	ret.setCached ( false );

	switch ( ret.m_type )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					if ( vmnumber.nbr_part[0] != 0 )
					{
						ret.m_type = vmnumber.m_type;
						ret.nbr_part[0] /= vmnumber.nbr_part[0];
						ret.nbr_part[1] = vmnumber.nbr_part[1];
						ret.nbr_part[2] = vmnumber.nbr_part[2];
					}
				break;
				case VMNT_DATE: // full convertion
					if ( vmnumber.nbr_upart[0] != 0 )
					{
						ret.m_type = vmnumber.m_type;
						ret.nbr_upart[0] /= vmnumber.nbr_upart[0];
						ret.nbr_upart[1] = vmnumber.nbr_upart[1];
						ret.nbr_upart[2] = vmnumber.nbr_upart[2];
					}
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					if ( vmnumber.nbr_part[0] != 0 )
						ret.nbr_part[0] = nbr_part[0] / vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					if ( ( vmnumber.nbr_upart[0] ) != 0 && ( vmnumber.nbr_upart[1] != 0 ) )
					{
						if ( ret.m_type == VMNT_DOUBLE )
						{
							ret.m_type = vmnumber.m_type;
							ret.nbr_upart[0] = static_cast<unsigned int> ( ret.nbr_part[0] ) / vmnumber.nbr_upart[0];
							ret.nbr_upart[1] = static_cast<unsigned int> ( ret.nbr_part[1] ) / vmnumber.nbr_upart[1];
							ret.nbr_upart[2] = vmnumber.nbr_upart[2];
						}
						else
						{
							ret.nbr_part[0] /= static_cast<int> ( vmnumber.nbr_upart[0] );
							ret.nbr_part[1] /= static_cast<int> ( vmnumber.nbr_upart[1] );
						}
					}
				break;
				case VMNT_PRICE:
					ret.fromStrPrice ( useCalc ( vmnumber, ret, QStringLiteral ( " / " ) ) );
				break;
				case VMNT_DOUBLE:
					ret.fromTrustedStrDouble ( useCalc ( vmnumber, ret, QStringLiteral ( " / " ) ) );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					ret.setTime ( 0, 0, (ret.hours() + ret.minutes () + ret.seconds ()) / vmnumber.nbr_part[0], false, true );
				break;
				case VMNT_TIME:
					ret.setTime ( ret.hours () / vmnumber.hours (), ret.minutes () / vmnumber.minutes (), ret.seconds () / vmnumber.seconds (), false, true );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				case VMNT_DATE:
				break;
			}
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		case VMNT_DATE:
		break;
	}
	return ret;
}

vmNumber vmNumber::operator/ ( const int number ) const
{
	vmNumber ret ( *this );
	if ( number == 0 )
		return ret;
	ret.setCached ( false );

	switch ( ret.m_type )
	{
		case VMNT_INT:
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			ret.nbr_part[0] /= number;
		break;
		case VMNT_PHONE:
		case VMNT_DATE:
		break;
		case VMNT_TIME:
			ret.setTime ( 0, 0, (ret.hours () + ret.minutes () + ret.seconds()) / number, false, true );
		break;
		case VMNT_UNSET:
		break;
	}
	return ret;
}

vmNumber vmNumber::operator/ ( const unsigned int number ) const
{
	if ( number > INT_MAX )
	{
		return emptyNumber;
	}
	return operator/ ( static_cast<int>(number) );
}

vmNumber vmNumber::operator* ( const vmNumber& vmnumber ) const
{
	vmNumber ret ( *this );
	ret.setCached ( false );

	switch ( ret.m_type )
	{
		case VMNT_INT:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
				case VMNT_TIME:
					ret.m_type = vmnumber.m_type;
					ret.nbr_part[0] *= vmnumber.nbr_part[0];
					ret.nbr_part[1] = vmnumber.nbr_part[1];
					ret.nbr_part[2] = vmnumber.nbr_part[2];
				break;
				case VMNT_DATE: // full convertion
					ret.m_type = vmnumber.m_type;
					ret.nbr_upart[0] *= vmnumber.nbr_upart[0];
					ret.nbr_upart[1] = vmnumber.nbr_upart[1];
					ret.nbr_upart[2] = vmnumber.nbr_upart[2];
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_DOUBLE:
		case VMNT_PRICE:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_TIME:
					ret.nbr_part[0] = nbr_part[0] * vmnumber.nbr_part[0];
				break;
				case VMNT_DATE:
					if ( ret.m_type == VMNT_DOUBLE )
					{
						ret.m_type = vmnumber.m_type;
						ret.nbr_upart[0] = static_cast<unsigned int> ( ret.nbr_part[0] ) * vmnumber.nbr_upart[0];
						ret.nbr_upart[1] = static_cast<unsigned int> ( ret.nbr_part[1] ) * vmnumber.nbr_upart[1];
						ret.nbr_upart[2] = vmnumber.nbr_upart[2];
					}
					else
					{
						ret.nbr_part[0] *= static_cast<int> ( vmnumber.nbr_upart[0] );
						ret.nbr_part[1] *= static_cast<int> ( vmnumber.nbr_upart[1] );
					}
				break;
				case VMNT_PRICE:
					ret.fromStrPrice ( useCalc ( vmnumber, ret, QStringLiteral ( " * " ) ) );
				break;
				case VMNT_DOUBLE:
					ret.fromTrustedStrDouble ( useCalc ( vmnumber, ret, QStringLiteral ( " * " ) ) );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				break;
			}
		break;
		case VMNT_TIME:
			switch ( vmnumber.m_type )
			{
				case VMNT_INT:
				case VMNT_DOUBLE:
				case VMNT_PRICE:
					ret.setTime ( 0, 0, (ret.hours () + ret.minutes () + ret.seconds ()) * vmnumber.nbr_part[0], false, true );
				break;
				case VMNT_TIME:
					ret.setTime ( ret.hours () * vmnumber.hours (), ret.minutes () * vmnumber.minutes (), ret.seconds () * vmnumber.seconds (), false, true );
				break;
				case VMNT_UNSET:
				case VMNT_PHONE:
				case VMNT_DATE:
				break;
			}
		break;
		case VMNT_UNSET:
		case VMNT_PHONE:
		case VMNT_DATE:
		break;
	}
	return ret;
}
//-------------------------------------OPERATORS------------------------------------------
