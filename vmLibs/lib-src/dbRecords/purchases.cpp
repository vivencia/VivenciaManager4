#include "purchases.h"
#include "vmlibs.h"
#include "dblistitem.h"
#include "supplierrecord.h"
#include "vivenciadb.h"

#include <vmStringRecord/stringrecord.h>

static const unsigned int TABLE_VERSION ( 'B' );

constexpr DB_FIELD_TYPE BUYS_FIELDS_TYPE[BUY_FIELD_COUNT] = {
	DBTYPE_ID, DBTYPE_ID, DBTYPE_ID, DBTYPE_DATE, DBTYPE_DATE, DBTYPE_PRICE,
	DBTYPE_PRICE, DBTYPE_NUMBER, DBTYPE_LIST, DBTYPE_LIST, DBTYPE_SHORTTEXT,
	DBTYPE_SUBRECORD, DBTYPE_SUBRECORD, DBTYPE_YESNO
};

bool updatePurchaseTable ( const unsigned char /*current_table_version*/ )
{
	/*if ( current_table_version == 'A')
	{
		if ( DBRecord::databaseManager ()->insertColumn ( FLD_BUY_LAST_VIEWED, &Buy::t_info ) )
		{
			VivenciaDB::optimizeTable ( &Buy::t_info );
			return true;
		}
	}
	return false;*/
	return true;
}

const TABLE_INFO Buy::t_info =
{
	PURCHASE_TABLE,
	QStringLiteral ( "PURCHASES" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ), UNIQUE KEY `id` ( `ID` ) " ),
	QStringLiteral ( "`ID`|`CLIENTID`|`JOBID`|`DATE`|`DELIVERDATE`|`PRICE`|`TOTAL_PAID`|`TOTALPAYS`|`DELIVERMETHOD`|`SUPPLIER`|`NOTES`|`REPORT`|`PAYINFO`|`LAST_VIEWED`|" ),
	QStringLiteral ( " int ( 9 ) NOT NULL, | int ( 9 ) NOT NULL, | int ( 9 ) DEFAULT NULL, | varchar ( 30 ) DEFAULT NULL, |"
	" varchar ( 30 ) DEFAULT NULL, | varchar ( 30 ) DEFAULT NULL, | varchar ( 30 ) DEFAULT NULL, | varchar ( 10 ) DEFAULT NULL, |"
	" varchar ( 40 ) DEFAULT NULL, | varchar ( 100 ) DEFAULT NULL, | varchar ( 200 ) DEFAULT NULL, |"
	" longtext COLLATE utf8_unicode_ci, | longtext COLLATE utf8_unicode_ci, | int ( 2 ) DEFAULT 0, |" ),
	QStringLiteral ( "ID|Client ID|Job ID|Purchase date|Delivery date|Price|Price paid|Number of payments|Delivery method|Supplier|Notes|Report|Pay info|Last Viewed|" ),
	BUYS_FIELDS_TYPE,
	TABLE_VERSION, BUY_FIELD_COUNT, TABLE_BUY_ORDER,
	&updatePurchaseTable
	#ifdef TRANSITION_PERIOD
	, true
	#endif
};

void updateSupplierTable_purchases ( const DBRecord* db_rec )
{
	supplierRecord::insertIfSupplierInexistent ( recStrValue ( db_rec, FLD_BUY_SUPPLIER ) );
}

void updateBuyDeliverCompleter ( const DBRecord* db_rec )
{
	DBRecord::completerManager ()->updateCompleter ( db_rec, FLD_BUY_DELIVERMETHOD, DELIVERY_METHOD );
}

Buy::Buy ( const bool connect_helper_funcs )
	: DBRecord ( BUY_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = & ( this->t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = BUYS_FIELDS_TYPE;

	if ( connect_helper_funcs )
	{
		DBRecord::helperFunction = this->helperFunction;
		setHelperFunction ( FLD_BUY_SUPPLIER, &updateSupplierTable_purchases );
		setHelperFunction ( FLD_BUY_DELIVERMETHOD, &updateBuyDeliverCompleter );
	}
}

Buy::~Buy () {}

uint Buy::isrRecordField ( const ITEMS_AND_SERVICE_RECORD isr_field ) const
{
	uint rec_field ( 0 );
	switch ( isr_field )
	{
		case ISR_SUPPLIER:				rec_field = FLD_BUY_SUPPLIER;	break;
		case ISR_ID:					rec_field = FLD_BUY_ID;			break;
		case ISR_OWNER:					rec_field = FLD_BUY_JOBID;		break;
		case ISR_DATE:					rec_field = FLD_BUY_DATE;		break;
		default:						rec_field = FLD_BUY_REPORT;		break;
	}
	return rec_field;
}

QString Buy::isrValue ( const ITEMS_AND_SERVICE_RECORD isr_field, const int sub_record ) const
{
	const uint rec_field ( isrRecordField ( isr_field ) );
	if ( rec_field == FLD_BUY_REPORT )
	{
		if ( sub_record >= 0 )
		{
			const stringTable& table ( recStrValue ( this, FLD_BUY_REPORT ) );
			const stringRecord* rec ( &table.readRecord ( static_cast<uint>( sub_record ) ) );
			return rec->fieldValue ( isr_field );
		}
		return QString ();
	}
	return recStrValue ( this, rec_field );
}

int Buy::searchCategoryTranslate ( const SEARCH_CATEGORIES sc ) const
{
	switch ( sc )
	{
		case SC_ID:			return FLD_BUY_ID;
		case SC_REPORT_1:	return FLD_BUY_REPORT;
		case SC_REPORT_2:	return FLD_BUY_NOTES;
		case SC_ADDRESS_1:	return FLD_BUY_SUPPLIER;
		case SC_PRICE_1:	return FLD_BUY_PRICE;
		case SC_DATE_1:		return FLD_BUY_DATE;
		case SC_DATE_3:		return FLD_BUY_DELIVERDATE;
		case SC_EXTRA_1:	return FLD_BUY_CLIENTID;
		case SC_EXTRA_2:	return FLD_BUY_JOBID;
		default:			return -1;
	}
}

void Buy::copySubRecord ( const uint subrec_field, const stringRecord& subrec )
{
	if ( subrec_field <= FLD_BUY_REPORT )
	{
		const uint nFields ( 6 ); // PAY_INFO_FIELD_COUNT == 6 and ITEMS_AND_SERVICE_RECORD used fields by Buy == 6
		if ( subrec.curIndex () == -1 )
			subrec.first ();
		stringRecord rec;
		for ( uint i ( 0 ); i < nFields; ++i )
		{
			rec.fastAppendValue ( subrec.curValue () );
			if ( !subrec.next () ) break;
		}
		setRecValue ( this, subrec_field, rec.toString () );
	}
}

void Buy::setListItem ( buyListItem* buy_item )
{
	DBRecord::mListItem = static_cast<dbListItem*>( buy_item );
}

buyListItem* Buy::buyItem () const
{
	return dynamic_cast<buyListItem*>( DBRecord::mListItem );
}
