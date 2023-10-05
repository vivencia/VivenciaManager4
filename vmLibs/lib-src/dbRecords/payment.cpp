#include "payment.h"
#include "vmlibs.h"

#include "dblistitem.h"
#include "vivenciadb.h"

#include <vmStringRecord/stringrecord.h>

static const unsigned int TABLE_VERSION ( 'B' );

constexpr DB_FIELD_TYPE PAYS_FIELDS_TYPE[PAY_FIELD_COUNT] = {
	DBTYPE_ID, DBTYPE_ID, DBTYPE_ID, DBTYPE_PRICE, DBTYPE_NUMBER, DBTYPE_PRICE,
	DBTYPE_SHORTTEXT, DBTYPE_SUBRECORD, DBTYPE_YESNO, DBTYPE_PRICE, DBTYPE_NUMBER
};

bool updatePaymentTable ( const unsigned char /*current_table_version*/ )
{
	/*if ( current_table_version == 'A')
	{
		DBRecord::databaseManager ()->insertColumn ( FLD_PAY_SEARCH_STATUS, &Payment::t_info );
		VivenciaDB::optimizeTable ( &Payment::t_info );
		return true;
	}*/
	return false;
}

const TABLE_INFO Payment::t_info =
{
	PAYMENT_TABLE,
	QStringLiteral ( "PAYMENTS" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ) , UNIQUE KEY `id` ( `ID` ) " ),
	QStringLiteral ( "`ID`|`CLIENTID`|`JOBID`|`PRICE`|`TOTALPAYS`|`TOTALPAID`|`OBS`|`INFO`|`OVERDUE`|`OVERDUE_VALUE`|`SEARCH_STATUS`|" ),
	QStringLiteral ( " int ( 9 ) NOT NULL, | int ( 9 ) NOT NULL, | int ( 9 ) DEFAULT NULL, | varchar ( 30 ) DEFAULT NULL, |"
	" varchar ( 30 ) DEFAULT NULL, | varchar ( 30 ) DEFAULT NULL, | varchar ( 100 ) COLLATE utf8_unicode_ci DEFAULT NULL, |"
	" longtext COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 4 ) DEFAULT NULL, | varchar ( 30 ) DEFAULT NULL, | int ( 9 ) DEFAULT 0, |" ),
	QStringLiteral ( "ID|Client ID|Job ID|Price|Number of payments|Total paid|Observations|Info|Overdue|Overdue value|Search Status|" ),
	PAYS_FIELDS_TYPE, TABLE_VERSION, PAY_FIELD_COUNT, TABLE_PAY_ORDER, 
	&updatePaymentTable
	#ifdef TRANSITION_PERIOD
	, true
	#endif
};

Payment::Payment ( const bool )
	: DBRecord ( PAY_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = &( this->t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = PAYS_FIELDS_TYPE;
}

Payment::~Payment () {}

int Payment::searchCategoryTranslate ( const SEARCH_CATEGORIES sc ) const
{
	switch ( sc )
	{
		case SC_ID:				return FLD_PAY_ID;
		case SC_REPORT_2:		return FLD_PAY_OBS;
		case SC_PRICE_1:		return FLD_PAY_PRICE;
		default:				return -1;
	}
}

void Payment::copySubRecord ( const uint subrec_field, const stringRecord& subrec )
{
	if ( subrec_field == FLD_PAY_INFO )
	{
		if ( subrec.curIndex () == -1 )
			subrec.first ();
		stringRecord pay_info;
		for ( uint i ( 0 ); i < PAY_INFO_FIELD_COUNT; ++i )
		{
			pay_info.fastAppendValue ( subrec.curValue () );
			if ( !subrec.next () ) break;
		}
		setRecValue ( this, FLD_PAY_INFO, pay_info.toString () );
	}
}

void Payment::setListItem ( payListItem* pay_item )
{
	DBRecord::mListItem = static_cast<dbListItem*>( pay_item );
}

payListItem* Payment::payItem () const
{
	return dynamic_cast<payListItem*>( DBRecord::mListItem );
}
