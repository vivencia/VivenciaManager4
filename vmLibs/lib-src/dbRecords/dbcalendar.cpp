#include "dbcalendar.h"
#include "vmlibs.h"

#include "job.h"
#include "payment.h"
#include "purchases.h"

#include <vmStringRecord/stringrecord.h>

#include <QtCore/QCoreApplication>

const unsigned int TABLE_VERSION ( 'A' );

constexpr DB_FIELD_TYPE CALENDAR_FIELDS_TYPE[CALENDAR_FIELD_COUNT] = {
	DBTYPE_ID, DBTYPE_DATE, DBTYPE_NUMBER, DBTYPE_NUMBER, DBTYPE_SHORTTEXT,
	DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT
};

inline uint monthToDBMonth ( const vmNumber& date )
{
	return ( date.year () - 2009 ) * 12 + date.month ();
}

inline uint DBMonthToMonth ( const uint dbmonth )
{
	const uint year ( dbmonth / 12 );
	return ( dbmonth - ( year * 12 ) );
}

inline void DBMonthToDate ( const uint dbmonth, vmNumber& date )
{
	const uint year ( dbmonth / 12 );
	date.setDate ( 1, static_cast<int>(DBMonthToMonth ( dbmonth )), static_cast<int>(year + 2009) );
}

inline uint weekNumberToDBWeekNumber ( const vmNumber& date )
{
	return ( date.year () - 2009 ) * 100 + date.weekNumber ();
}

inline uint DBWeekNumberToWeekNumber ( const uint dbweek )
{
	const uint year ( dbweek / 100 );
	return ( dbweek - ( year * 100 ) );
}

inline void DBWeekNumberToDate ( const uint dbweek, vmNumber& date )
{
	const uint year ( dbweek / 100 );
	/* This will retrieve the week number back, but the day will not be the same (unless by coincidence)
	 * and the month might not be the same, as a week may have two months (by comprising the end of one month
	 * and the beginning of the other)
	 */
	date.setDate ( ( static_cast<int>(DBWeekNumberToWeekNumber ( dbweek ) * 7) ) - 2, 1, static_cast<int>(year + 2009) );
}

#define CALENDAR_TABLE_UPDATE_AVAILABLE
#ifdef CALENDAR_TABLE_UPDATE_AVAILABLE
#include "vivenciadb.h"
#include <vmNumbers/vmnumberformats.h>
#include <vmNotify/vmnotify.h>
#endif //CALENDAR_TABLE_UPDATE_AVAILABLE

bool updateCalendarTable ( const unsigned char /*current_table_version*/ )
{
	/*if ( current_table_version != 'A' )
		return false;

	auto vdb ( DBRecord::databaseManager () );
	vdb->database ()->exec ( QStringLiteral ( "RENAME TABLE `CALENDAR` TO `OLD_CALENDAR`" ) );
	vdb->createTable ( &dbCalendar::t_info );

	const uint max_pbar_value ( vdb->getHighestID ( TABLE_JOB_ORDER ) + vdb->getHighestID ( TABLE_PAY_ORDER ) +
								vdb->getHighestID ( TABLE_BUY_ORDER ) - 10 );
	vmNotify* pBox ( nullptr );
	pBox = vmNotify::progressBox ( nullptr, nullptr, max_pbar_value, 0,
			QStringLiteral ( "Updating the Calendar database. This might take a while..." ),
			QStringLiteral ( "Starting...." ) );

	
	vdb->clearTable ( &dbCalendar::t_info );
	dbCalendar calendar;

	Job job;
	uint i ( 1 );
	if ( job.readFirstRecord () )
	{
		do
		{
			pBox = vmNotify::progressBox ( pBox, nullptr, max_pbar_value, i++, QString (), QStringLiteral( "Scanning the Jobs table" ) );
			calendar.updateCalendarWithJobInfo ( &job );
		} while ( job.readNextRecord () );
	}

	Payment pay;
	if ( pay.readFirstRecord () )
	{
		do
		{
			pBox = vmNotify::progressBox ( pBox, nullptr, max_pbar_value, i++, QString (), QStringLiteral( "Scanning the Payments table" ) );
			calendar.updateCalendarWithPayInfo ( &pay );
		} while ( pay.readNextRecord () );
	}

	Buy buy;
	if ( buy.readFirstRecord () )
	{
		do
		{
			pBox = vmNotify::progressBox ( pBox, nullptr, max_pbar_value, i++, QString (), QStringLiteral( "Scanning the Purchases table" ) );
			calendar.updateCalendarWithBuyInfo ( &buy );
		} while ( buy.readNextRecord () );
	}
	pBox = vmNotify::progressBox ( pBox, nullptr, max_pbar_value, i++, QString (), QStringLiteral( "Done. Calendar table updated" ) );
	vdb->optimizeTable ( &dbCalendar::t_info );
	vdb->deleteTable ( "OLD_CALENDAR" );
	return true;*/
	return false;
}

const TABLE_INFO dbCalendar::t_info = {
	CALENDAR_TABLE,
	QStringLiteral ( "CALENDAR" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ) , UNIQUE KEY `id` ( `ID` ) " ),
	QStringLiteral ( "`ID`|`DAY_DATE`|`WEEK_NUMBER`|`MONTH`|`JOBS`|`PAYS`|`PAYS_USE`|"
	"`BUYS`|`BUYS_PAY`|`TOTAL_JOBPRICE_SCHEDULED`|`TOTAL_PAY_RECEIVED`|"
	"`TOTAL_BUY_BOUGHT`|`TOTAL_BUY_PAID`|" ),
	QStringLiteral ( " int ( 9 ) NOT NULL, | varchar ( 30 ) NOT NULL, | int ( 4 ) DEFAULT NULL, |"
	" int ( 4 ) DEFAULT NULL, | varchar ( 100 ) DEFAULT NULL, | varchar ( 100 ) DEFAULT NULL, |"
	" varchar ( 100 ) DEFAULT NULL, | varchar ( 100 ) DEFAULT NULL, | varchar ( 100 ) DEFAULT NULL, |"
	" varchar ( 50 ) DEFAULT NULL, | varchar ( 50 ) DEFAULT NULL, | varchar ( 50 ) DEFAULT NULL, |"
	" varchar ( 50 ) DEFAULT NULL, |" ),
	QCoreApplication::tr ( "ID|Date|Week Number|Month|Jobs|Payments|Deposit date|Purchases|"
	"Purchase pay date|Scheduled amount to receive|Amount actually receive|"
	"Purchases made|Purchases paid|" ),
	CALENDAR_FIELDS_TYPE, TABLE_VERSION, CALENDAR_FIELD_COUNT, TABLE_CALENDAR_ORDER,
	#ifdef CALENDAR_TABLE_UPDATE_AVAILABLE
	&updateCalendarTable
	#else
	nullptr
	#endif //CALENDAR_TABLE_UPDATE_AVAILABLE
	#ifdef TRANSITION_PERIOD
	, true
	#endif
};

dbCalendar::dbCalendar ()
	: DBRecord ( CALENDAR_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = &( dbCalendar::t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = CALENDAR_FIELDS_TYPE;
	DBRecord::helperFunction = this->helperFunction;
}

void dbCalendar::updateCalendarWithJobInfo ( const Job* const job )
{
	const stringTable& jobReport ( recStrValue ( job, FLD_JOB_REPORT ) );
	const uint n_days ( jobReport.countRecords () );
	vmNumber date;
	const vmNumber& pricePerDay ( job->price ( FLD_JOB_PRICE ) / n_days );
	const stringRecord* dayRecord ( nullptr );
	pointersList<CALENDAR_EXCHANGE*> ce_list ( 5 );

	switch ( job->action () )
	{
		case ACTION_ADD:
		case ACTION_READ: // this is when updating the table. All read jobs will contain new info as though they were new
		{					// The prevAction for those DBRecords that have just been created/read is ACTION_NONE
			for ( uint i ( 0 ); i < n_days ; ++i )
			{
				dayRecord = &jobReport.readRecord ( i );
				date.fromTrustedStrDate ( dayRecord->fieldValue ( Job::JRF_DATE ), vmNumber::VDF_DB_DATE );
				addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE1, date, vmNumber::emptyNumber, static_cast<int>( i + 1 ) );
				if ( !pricePerDay.isNull () )
					addCalendarExchangeRule ( ce_list, CEAO_ADD_PRICE_DATE1, date, pricePerDay );
			}
		}
		break;
		case ACTION_DEL:
		{
			for ( uint i ( 0 ); i < n_days ; ++i )
			{
				dayRecord = &jobReport.readRecord ( i );
				date.fromTrustedStrDate ( dayRecord->fieldValue ( Job::JRF_DATE ), vmNumber::VDF_DB_DATE );
				addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE1, date, vmNumber::emptyNumber, static_cast<int>( i + 1 ) );
				if ( !pricePerDay.isNull () )
					addCalendarExchangeRule ( ce_list, CEAO_DEL_PRICE_DATE1, date, pricePerDay );
			}
		}
		break;
		case ACTION_EDIT:
		{
			const stringTable& oldJobReport ( recStrValueAlternate ( job, FLD_JOB_REPORT ) );
			const uint old_n_days ( oldJobReport.countRecords () );
			const vmNumber& oldPricePerDay ( vmNumber ( recStrValueAlternate ( job, FLD_JOB_PRICE ), VMNT_PRICE, 1 ) / old_n_days );
			
			if ( job->wasModified ( FLD_JOB_REPORT ) )
			{
				for ( uint i ( 0 ); i < old_n_days ; ++i )
				{
					dayRecord = &oldJobReport.readRecord ( i );
					date.fromTrustedStrDate ( dayRecord->fieldValue ( Job::JRF_DATE ), vmNumber::VDF_DB_DATE );
					addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE1, date, vmNumber::emptyNumber, static_cast<int>( i + 1 ) );
					if ( !oldPricePerDay.isNull () )
						addCalendarExchangeRule ( ce_list, CEAO_DEL_PRICE_DATE1, date, pricePerDay );
				}
				
				for ( uint i ( 0 ); i < n_days ; ++i )
				{
					dayRecord = &jobReport.readRecord ( i );
					date.fromTrustedStrDate ( dayRecord->fieldValue ( Job::JRF_DATE ), vmNumber::VDF_DB_DATE );
					addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE1, date, vmNumber::emptyNumber, static_cast<int>( i + 1 ) );
					if ( !pricePerDay.isNull () )
						addCalendarExchangeRule ( ce_list, CEAO_ADD_PRICE_DATE1, date, pricePerDay );
				}
			} 
			else // report not modified
			{
				if ( job->wasModified ( FLD_JOB_PRICE ) )
				{
					for ( uint i ( 0 ); i < n_days ; ++i )
					{
						dayRecord = &jobReport.readRecord ( i );
						date.fromTrustedStrDate ( dayRecord->fieldValue ( Job::JRF_DATE ), vmNumber::VDF_DB_DATE );
						addCalendarExchangeRule ( ce_list, CEAO_EDIT_PRICE_DATE1, date, pricePerDay );
						ce_list[ce_list.currentIndex()]->price2 = oldPricePerDay;
					}
				}
			}
		}
		break;
		case ACTION_NONE:
		case ACTION_REVERT:
			break;
	}
	updateCalendarDB ( job, ce_list );
}

void dbCalendar::updateCalendarWithPayInfo ( const Payment* const pay )
{	
	switch ( pay->action () )
	{
		case ACTION_ADD: // An adding pay is an empty pay. 
		case ACTION_REVERT:
		break;
		default:
		{
			vmNumber date, price;
			const stringTable& payInfo ( recStrValue ( pay, FLD_PAY_INFO ) );
			const uint n_pays ( payInfo.countRecords () );
			const stringRecord* payRecord ( nullptr );
			pointersList<CALENDAR_EXCHANGE*> ce_list ( 5 );
			
			switch ( pay->action () )
			{
				case ACTION_READ: // see updateCalendarWithJobInfo
					for ( uint i ( 0 ); i < n_pays ; ++i )
					{
						payRecord = &payInfo.readRecord ( i );
						price.fromTrustedStrPrice ( payRecord->fieldValue ( PHR_VALUE ) );
						date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
						addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE1, date, vmNumber::emptyNumber, static_cast<int>( i + 1 ) );
						if ( payRecord->fieldValue ( PHR_PAID ) == CHR_ONE )
						{
							date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_USE_DATE ), vmNumber::VDF_DB_DATE );
							addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE2, date, vmNumber::emptyNumber, static_cast<int>( i + 1 ) );
							addCalendarExchangeRule ( ce_list, CEAO_ADD_PRICE_DATE2, date, price );
						}
					}
				break;
				case ACTION_DEL:
					for ( uint i ( 0 ); i < n_pays ; ++i )
					{
						payRecord = &payInfo.readRecord ( i );
						price.fromTrustedStrPrice ( payRecord->fieldValue ( PHR_VALUE ) );
						date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
						addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE1, date, vmNumber::emptyNumber );
						if ( payRecord->fieldValue ( PHR_PAID ) == CHR_ONE )
						{
							date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_USE_DATE ), vmNumber::VDF_DB_DATE );
							addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE2, date, vmNumber::emptyNumber );
							addCalendarExchangeRule ( ce_list, CEAO_DEL_PRICE_DATE2, date, price );
						}
					}
				break;
				case ACTION_EDIT:
				{
					if ( pay->wasModified ( FLD_PAY_INFO ) )
					{
						const stringTable& oldPayInfo ( recStrValueAlternate ( pay, FLD_PAY_INFO ) );
						const uint old_n_pays ( oldPayInfo.countRecords () );
						for ( uint i ( 0 ); i < old_n_pays ; ++i )
						{
							payRecord = &oldPayInfo.readRecord ( i );
							price.fromTrustedStrPrice ( payRecord->fieldValue ( PHR_VALUE ) );
							date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
							addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE1, date, vmNumber::emptyNumber );
							if ( payRecord->fieldValue ( PHR_PAID ) == CHR_ONE )
							{
								date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_USE_DATE ), vmNumber::VDF_DB_DATE );
								addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE2, date, vmNumber::emptyNumber );
								addCalendarExchangeRule ( ce_list, CEAO_DEL_PRICE_DATE2, date, price );
							}
						}
						
						for ( uint i ( 0 ); i < n_pays ; ++i )
						{
							payRecord = &payInfo.readRecord ( i );
							price.fromTrustedStrPrice ( payRecord->fieldValue ( PHR_VALUE ) );
							date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
							addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE1, date, vmNumber::emptyNumber, static_cast<int>(i + 1) );
							if ( payRecord->fieldValue ( PHR_PAID ) == CHR_ONE )
							{
								date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_USE_DATE ), vmNumber::VDF_DB_DATE );
								addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE2, date, vmNumber::emptyNumber, static_cast<int>(i + 1) );
								addCalendarExchangeRule ( ce_list, CEAO_ADD_PRICE_DATE2, date, price );
							}
						}
					}
				}
				break; // action == ACTION_EDIT
				default: break;
			}
			updateCalendarDB ( pay, ce_list );
		}
		break;  // default action
	}
}

void dbCalendar::updateCalendarWithBuyInfo ( const Buy* const buy )
{
	const vmNumber& date ( buy->date ( FLD_BUY_DATE ) );
	const vmNumber& price ( buy->price ( FLD_BUY_PRICE ) );
	pointersList<CALENDAR_EXCHANGE*> ce_list ( 5 );

	switch ( buy->action () )
	{
		case ACTION_ADD:
		case ACTION_READ: // see updateCalendarWithJobInfo
		{
			if ( !price.isNull () )
				addCalendarExchangeRule ( ce_list, CEAO_ADD_PRICE_DATE1, date, price );
			addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE1, date );
		}
		break;
		
		case ACTION_DEL:
		{
			addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE1, date );
			addCalendarExchangeRule ( ce_list, CEAO_DEL_PRICE_DATE1, date, price );
		}
		break;
		
		case ACTION_EDIT:
		{
			if ( buy->wasModified ( FLD_BUY_PRICE ) || buy->wasModified ( FLD_BUY_DATE ) )
			{
				const vmNumber origDate ( recStrValueAlternate ( buy, FLD_BUY_DATE ), VMNT_DATE, vmNumber::VDF_DB_DATE );

				/* If either date or price were changed, we must exclude the
				 * the original price (if) stored in calendar, and (possibly) add the new price
				 */
				const vmNumber origPrice ( recStrValueAlternate ( buy, FLD_BUY_PRICE ), VMNT_PRICE, 1 );
				if ( !origPrice.isNull () )
				{
					if ( origPrice != price && !price.isNull () )
					{
						addCalendarExchangeRule ( ce_list, CEAO_DEL_PRICE_DATE1, origDate, origPrice );
						addCalendarExchangeRule ( ce_list, CEAO_ADD_PRICE_DATE1, date, price );
					}
				}
				// Now, we alter the date in calendar only if date was changed
				if ( origDate != date )
				{
					addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE1, origDate );
					addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE1, date );
				}
			}
		}
		break;
		
		case ACTION_NONE:
		case ACTION_REVERT:
		return;
	}
	updateCalendarWithBuyPayInfo ( buy, ce_list );
	updateCalendarDB ( buy, ce_list );
}

void dbCalendar::updateCalendarWithBuyPayInfo ( const Buy* const buy, pointersList<CALENDAR_EXCHANGE*>& ce_list )
{
	const stringRecord* rec ( nullptr );
	vmNumber date, price;
	const stringTable& newTable ( recStrValue ( buy, FLD_BUY_PAYINFO ) );

	switch ( buy->action () )
	{
		case ACTION_ADD:
		case ACTION_READ:
		{
			rec = &newTable.first ();
			while ( rec->isOK () )
			{
				date.fromTrustedStrDate ( rec->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
				addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE2, date, vmNumber::emptyNumber, newTable.currentIndex () + 1 );
				if ( rec->fieldValue ( PHR_PAID ) == CHR_ONE )
				{
					price.fromTrustedStrPrice ( rec->fieldValue ( PHR_VALUE ) );
					addCalendarExchangeRule ( ce_list, CEAO_ADD_PRICE_DATE2, date, price );
				}
				rec = &newTable.next ();
			}
		}
		break;
		
		case ACTION_DEL:
		{
			rec = &newTable.first ();
			while ( rec->isOK () )
			{
				date.fromTrustedStrDate ( rec->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
				addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE2, date, vmNumber::emptyNumber, newTable.currentIndex () + 1 );
				if ( rec->fieldValue ( PHR_PAID ) == CHR_ONE )
				{
					price.fromTrustedStrPrice ( rec->fieldValue ( PHR_VALUE ) );
					addCalendarExchangeRule ( ce_list, CEAO_DEL_PRICE_DATE2, date, price );
				}
				rec = &newTable.next ();
			}
		}
		break;
		
		case ACTION_EDIT:
		{
			const stringTable& oldTable ( recStrValueAlternate ( buy, FLD_BUY_PAYINFO ) );
			rec = &oldTable.first ();
			while ( rec->isOK () )
			{
				date.fromTrustedStrDate ( rec->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
				addCalendarExchangeRule ( ce_list, CEAO_DEL_DATE2, date, vmNumber::emptyNumber, newTable.currentIndex () + 1 );
				if ( rec->fieldValue ( PHR_PAID ) == CHR_ONE )
				{
					price.fromTrustedStrPrice ( rec->fieldValue ( PHR_VALUE ) );
					addCalendarExchangeRule ( ce_list, CEAO_DEL_PRICE_DATE2, date, price );
				}
				rec = &oldTable.next ();
			}
			
			rec = &newTable.first ();
			while ( rec->isOK () )
			{
				date.fromTrustedStrDate ( rec->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
				addCalendarExchangeRule ( ce_list, CEAO_ADD_DATE2, date, vmNumber::emptyNumber, newTable.currentIndex () + 1 );
				if ( rec->fieldValue ( PHR_PAID ) == CHR_ONE )
				{
					price.fromTrustedStrPrice ( rec->fieldValue ( PHR_VALUE ) );
					addCalendarExchangeRule ( ce_list, CEAO_ADD_PRICE_DATE2, date, price );
				}
				rec = &newTable.next ();
			}
		} //case ACTION_EDIT
		break;
		case ACTION_NONE:
		case ACTION_REVERT:
		break;
	}
}

void dbCalendar::updateCalendarDB ( const DBRecord* dbrec, pointersList<CALENDAR_EXCHANGE*>& ce_list )
{
	if ( ce_list.isEmpty () )
		return;

	stringRecord calendarIdTrio;
	calendarIdTrio.fastAppendValue ( dbrec->actualRecordStr ( 0 ) );
	calendarIdTrio.fastAppendValue ( dbrec->actualRecordStr ( 1 ) );
	
	uint calendarField[4] = { 0 };
	switch ( dbrec->typeID () )
	{
		case JOB_TABLE:
			calendarField[0] = FLD_CALENDAR_JOBS;
			calendarField[2] = FLD_CALENDAR_TOTAL_JOBPRICE_SCHEDULED;
		break;
		case PAYMENT_TABLE:
			calendarField[0] = FLD_CALENDAR_PAYS;
			calendarField[1] = FLD_CALENDAR_PAYS_USE;
			calendarField[2] = FLD_CALENDAR_TOTAL_PAY_RECEIVED;
			calendarField[3] = FLD_CALENDAR_TOTAL_PAY_RECEIVED;
		break;
		case PURCHASE_TABLE:
			calendarField[0] = FLD_CALENDAR_BUYS;
			calendarField[1] = FLD_CALENDAR_BUYS_PAY;
			calendarField[2] = FLD_CALENDAR_TOTAL_BUY_BOUGHT;
			calendarField[3] = FLD_CALENDAR_TOTAL_BUY_PAID;
		break;
	}

	CALENDAR_EXCHANGE* ce ( nullptr );
	for ( uint i ( 0 ); i < ce_list.count (); ++i )
	{
		ce = ce_list.at ( i );
		if ( ce->extra_info > 0 )
			calendarIdTrio.changeValue ( 2, QString::number ( ce->extra_info ) );
		switch ( ce->action )
		{
			case CEAO_NOTHING:
				continue;
			case CEAO_ADD_DATE1:
				addDate ( ce->date, calendarField[0], calendarIdTrio );
			break;
			case CEAO_DEL_DATE1:
				delDate ( ce->date, calendarField[0], calendarIdTrio );
			break;
			case CEAO_ADD_DATE2:
				addDate ( ce->date, calendarField[1], calendarIdTrio );
			break;
			case CEAO_DEL_DATE2:
				delDate ( ce->date, calendarField[1], calendarIdTrio );
			break;
			case CEAO_ADD_PRICE_DATE1:
				addPrice ( ce->date, ce->price, calendarField[2] );
			break;
			case CEAO_DEL_PRICE_DATE1:
				delPrice ( ce->date, ce->price, calendarField[2] );
			break;
			case CEAO_EDIT_PRICE_DATE1:
				editPrice ( ce->date, ce->price, ce->price2, calendarField[2] );
			break;
			case CEAO_ADD_PRICE_DATE2:
				addPrice ( ce->date, ce->price, calendarField[3] );
			break;
			case CEAO_DEL_PRICE_DATE2:
				delPrice ( ce->date, ce->price, calendarField[3] );
			break;
			case CEAO_EDIT_PRICE_DATE2:
				editPrice ( ce->date, ce->price, ce->price2, calendarField[3] );
			break;
		}
	}
	ce_list.clear ( true );
}

const stringTable& dbCalendar::dateLog ( const vmNumber& date, const uint search_field, const uint requested_field,
		QString& price, const uint requested_field_price, const bool bIncludeDates )
{
	mStrTable.clear ();
	if ( requested_field < FLD_CALENDAR_JOBS || requested_field > FLD_CALENDAR_BUYS_PAY )
		return mStrTable;

	switch ( search_field )
	{
		case FLD_CALENDAR_DAY_DATE:
			if ( readRecord ( FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) ) )
			{
				price = recStrValue ( this, requested_field_price );
				mStrTable.fromString ( recStrValue ( this, requested_field ) );
			}
			else
				clearAll ();
		break;
		case FLD_CALENDAR_WEEK_NUMBER:
		case FLD_CALENDAR_MONTH:
			if ( readFirstRecord ( static_cast<int>( search_field ), QString::number ( search_field == FLD_CALENDAR_MONTH ?
									monthToDBMonth ( date ) : weekNumberToDBWeekNumber ( date ) ) ) )
			{
				uint i_row ( 0 );
				vmNumber total_price;
				do
				{
					mStrTable.appendTable ( recStrValue ( this, requested_field ) );
					if ( bIncludeDates )
					{
						for ( ; i_row < mStrTable.countRecords (); ++i_row )
							mStrTable.changeRecord ( i_row, 3, recStrValue ( this, FLD_CALENDAR_DAY_DATE ) );
					}
					total_price += vmNumber ( recStrValue ( this, requested_field_price ), VMNT_PRICE, 1 );
				} while ( readNextRecord ( true ) );
				price = total_price.toPrice ();
			}
			else
				clearAll ();
		break;
		default:
		break;
	}
	return mStrTable;
}

void dbCalendar::addDate ( const vmNumber& date, const uint field, const stringRecord& id_trio )
{
	stringTable ids;
	
	clearAll ();
	if ( readRecord ( FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) ) )
	{
		ids.fromString ( recStrValue ( this, field ) );
		if ( ids.matchRecord ( id_trio ) >= 0 )
			return;
		setAction ( ACTION_EDIT );
	}
	else
	{
		setAction ( ACTION_ADD );
		setRecValue ( this, FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) );
		setRecValue ( this, FLD_CALENDAR_WEEK_NUMBER, QString::number ( weekNumberToDBWeekNumber ( date ) ) );
		setRecValue ( this, FLD_CALENDAR_MONTH, QString::number ( monthToDBMonth ( date ) ) );
		setRecValue ( this, FLD_CALENDAR_TOTAL_JOBPRICE_SCHEDULED, vmNumber::zeroedPrice.toPrice () );
		setRecValue ( this, FLD_CALENDAR_TOTAL_PAY_RECEIVED, vmNumber::zeroedPrice.toPrice () );
		setRecValue ( this, FLD_CALENDAR_TOTAL_BUY_PAID, vmNumber::zeroedPrice.toPrice () );
		setRecValue ( this, FLD_CALENDAR_TOTAL_BUY_BOUGHT, vmNumber::zeroedPrice.toPrice () );
	}
	ids.fastAppendRecord ( id_trio );
	setRecValue ( this, field, ids.toString () );
	saveRecord ();
}

void dbCalendar::delDate ( const vmNumber& date, const uint field, const stringRecord& id_trio )
{
	clearAll ();
	if ( readRecord ( FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) ) )
	{		
		setAction ( ACTION_EDIT );
		stringTable ids;
		ids.fromString ( recStrValue ( this, field ) );
		if ( ids.removeRecordByValue ( id_trio ) )
		{
			setRecValue ( this, field, ids.toString () );
			saveRecord ();
		}
	}
}

void dbCalendar::addPrice ( const vmNumber& date, const vmNumber& price, const uint field )
{
	vmNumber new_price ( price );
	clearAll ();
	if ( readRecord ( FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) ) )
	{
		const vmNumber old_price ( recStrValue ( this, field ), VMNT_PRICE, 1 );
		new_price += old_price;
		setAction ( ACTION_EDIT );
	}
	else
	{
		setAction ( ACTION_ADD );
		setRecValue ( this, FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) );
		setRecValue ( this, FLD_CALENDAR_WEEK_NUMBER, QString::number ( weekNumberToDBWeekNumber ( date ) ) );
		setRecValue ( this, FLD_CALENDAR_MONTH, QString::number ( monthToDBMonth ( date ) ) );
	}
	setRecValue ( this, field, new_price.toPrice () );
	saveRecord ();
}

void dbCalendar::editPrice ( const vmNumber& date, const vmNumber& new_price, const vmNumber& old_price, const uint field )
{
	vmNumber price;
	clearAll ();
	if ( readRecord ( FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) ) )
	{
		setAction ( ACTION_EDIT );
		price.fromTrustedStrPrice ( recStrValue ( this, field ) );
		price -= old_price;
		price += new_price;
	}
	else
	{
		setAction ( ACTION_ADD );
		setRecValue ( this, FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) );
		setRecValue ( this, FLD_CALENDAR_WEEK_NUMBER, QString::number ( weekNumberToDBWeekNumber ( date ) ) );
		setRecValue ( this, FLD_CALENDAR_MONTH, QString::number ( monthToDBMonth ( date ) ) );
		price = new_price;
	}
	setRecValue ( this, field, price.toPrice () );
	saveRecord ();
}

void dbCalendar::delPrice ( const vmNumber& date, const vmNumber& price, const uint field )
{
	if ( !price.isNull () )
	{
		clearAll ();
		if ( readRecord ( FLD_CALENDAR_DAY_DATE, date.toDate ( vmNumber::VDF_DB_DATE ) ) )
		{
			setAction ( ACTION_EDIT );
			const vmNumber old_price ( recStrValue ( this, field ), VMNT_PRICE, 1 );
			const vmNumber new_price ( old_price - price );
			setRecValue ( this, field, new_price.toPrice () );
			saveRecord ();
		}
	}
}

void dbCalendar::addCalendarExchangeRule ( pointersList<CALENDAR_EXCHANGE*>& ce_list,
		const CALENDAR_EXCHANGE_ACTION_ORDER action,
		const vmNumber& date, const vmNumber& price, const int extra_info )
{
	if ( action == CEAO_NOTHING )
		return;
	auto ce ( new CALENDAR_EXCHANGE );
	ce->action = action;
	ce->date = date;
	ce->price = price;
	ce->extra_info = extra_info;
	ce_list.append ( ce );
}
