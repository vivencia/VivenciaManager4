#include "completerrecord.h"

#include "vivenciadb.h"
#include "client.h"
#include "supplierrecord.h"
#include "dbsupplies.h"
#include "inventory.h"
#include "payment.h"
#include "purchases.h"
#include "generaltable.h"

#ifdef COMPLETER_TABLE_UPDATE_AVAILABLE
#include "vmnotify.h"
#include "job.h"
#endif

const unsigned int TABLE_VERSION ( 'B' );

constexpr DB_FIELD_TYPE CR_FIELDS_TYPE[CR_FIELD_COUNT] =
{
	DBTYPE_ID, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT,
	DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT,
	DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT
};

#ifdef COMPLETER_TABLE_UPDATE_AVAILABLE
bool updateCompleterRecordTable ()
{
	//VDB ()->optimizeTable ( &completerRecord::t_info );
	//return true;
	vmNotify* pBox ( nullptr );
	uint step ( 0 );
	const uint max_steps ( 9 );
	pBox = vmNotify::progressBox ( nullptr, nullptr, max_steps, step++,
			QStringLiteral ( "Updating the Completers database. This might take a while..." ),
			QStringLiteral ( "Starting...." ) );
	
	VDB ()->deleteTable ( &completerRecord::t_info );
	VDB ()->createTable ( &completerRecord::t_info );

	completerRecord cr;
	QStringList results, results_2, results_3;
	int i ( 0 );
	stringRecord rec;
	stringTable payinfo;
	QString str_temp;

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for brands" ) );
	cr.runQuery ( results, &Inventory::t_info, FLD_INVENTORY_BRAND, true );
	cr.runQuery ( results, &dbSupplies::t_info, FLD_SUPPLIES_BRAND, true );
	cr.batchUpdateTable ( BRAND, results );

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for inventory type" ) );
	cr.runQuery ( results, &Inventory::t_info, FLD_INVENTORY_TYPE, true );
	cr.batchUpdateTable ( STOCK_TYPE, results );

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for inventory and supplies place" ) );
	cr.runQuery ( results, &Inventory::t_info, FLD_INVENTORY_PLACE, true );
	cr.runQuery ( results, &dbSupplies::t_info, FLD_SUPPLIES_PLACE, true );
	cr.batchUpdateTable ( STOCK_PLACE, results );

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for addresses" ) );
	cr.runQuery ( results, &Client::t_info, FLD_CLIENT_STREET, true );
	cr.runQuery ( results, &Client::t_info, FLD_CLIENT_CITY, true );
	cr.runQuery ( results, &Client::t_info, FLD_CLIENT_DISTRICT, true );
	cr.runQuery ( results, &supplierRecord::t_info, FLD_SUPPLIER_STREET, true );
	cr.runQuery ( results, &supplierRecord::t_info, FLD_SUPPLIER_CITY, true );
	cr.runQuery ( results, &supplierRecord::t_info, FLD_SUPPLIER_DISTRICT, true );
	cr.batchUpdateTable ( ADDRESS, results );

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for delivery method" ) );
	cr.runQuery ( results, &Buy::t_info, FLD_BUY_DELIVERMETHOD, true );
	cr.batchUpdateTable ( DELIVERY_METHOD, results );

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for payment methods" ) );
	cr.runQuery ( results, &Buy::t_info, FLD_BUY_PAYINFO, true );
	if ( !results.isEmpty () ) {
		for ( i = 0; i < results.count (); ++i )
		{
			payinfo.fromString ( results.at ( i ) );
			if ( payinfo.isOK () )
			{
				rec = payinfo.first ();
				if ( !rec.isNull () )
				{
					str_temp = rec.fieldValue ( PHR_METHOD );
					if ( !str_temp.isEmpty () && !results_2.contains ( str_temp ) )
						results_2.append ( str_temp );
				}
			}
		}
		results.clear ();
	}

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for accounts" ) );
	cr.runQuery ( results, &Payment::t_info, FLD_PAY_INFO, true );
	if ( !results.isEmpty () )
	{
		for ( i = 0; i < results.count (); ++i )
		{
			payinfo.fromString ( results.at ( i ) );
			if ( payinfo.isOK () )
			{
				rec = payinfo.first ();
				if ( !rec.isNull () )
				{
					str_temp = rec.fieldValue ( PHR_METHOD );
					if ( !str_temp.isEmpty () && !results_2.contains ( str_temp ) )
						results_2.append ( str_temp );
					str_temp = rec.fieldValue ( PHR_ACCOUNT );
					if ( !str_temp.isEmpty () && !results_3.contains ( str_temp ) )
						results_3.append ( str_temp );
				}
			}
		}
		results.clear ();
	}
	cr.batchUpdateTable ( PAYMENT_METHOD, results_2 );
	cr.batchUpdateTable ( ACCOUNT, results_3 );

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for job types" ) );
	cr.runQuery ( results, &Job::t_info, FLD_JOB_TYPE, true );
	cr.batchUpdateTable ( JOB_TYPE, results );

	pBox = vmNotify::progressBox ( pBox, nullptr, max_steps, step++, QString::null, QStringLiteral( "Creating completer records for supply item strings" ) );
	dbSupplies db_sup;
	if ( db_sup.readFirstRecord () )
	{
		stringRecord info;
		QString compositItemName;
		results_2.clear ();
		results_3.clear ();
		do {
			compositItemName = db_sup.isrValue ( ISR_NAME ) + QLatin1String ( " (" ) +
							   db_sup.isrValue ( ISR_UNIT ) + QLatin1String ( ") " ) +
							   db_sup.isrValue ( ISR_BRAND );
			results_2.append ( compositItemName );
			for ( uint fld ( 0 ); fld <= 8; ++fld )
				info.fastAppendValue ( db_sup.isrValue ( static_cast<ITEMS_AND_SERVICE_RECORD>( fld ) ) );
			results_3.append ( info.toString () );
			info.clear ();
		} while ( db_sup.readNextRecord () );
		cr.batchUpdateTable ( ALL_CATEGORIES, results_2 );
		cr.batchUpdateTable ( PRODUCT_OR_SERVICE, results_3 );
	}
	VDB ()->optimizeTable( &completerRecord::t_info );
	return true;
}
#endif //COMPLETER_TABLE_UPDATE_AVAILABLE

const TABLE_INFO completerRecord::t_info =
{
	COMPLETER_RECORDS_TABLE,
	QStringLiteral ( "COMPLETER_RECORDS" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ), UNIQUE KEY `id` ( `ID` )" ),
	QStringLiteral ( " `ID`|`PRODUCT_OR_SERVICE_1`|`PRODUCT_OR_SERVICE_2`|`BRAND`|`STOCK_TYPE`|`STOCK_PLACE`|`PAYMENT_METHOD`|`ADDRESS`|`DELIVERY_METHOD`|`ACCOUNT`|`JOB_TYPE`|`MACHINE_NAME`|`MACHINE_EVENT`|" ),
	QStringLiteral ( " int ( 9 ) NOT NULL, | varchar ( 100 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 200 ) COLLATE utf8_unicode_ci DEFAULT NULL, | "
	"varchar ( 50 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 50 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 30 ) DEFAULT NULL, | varchar ( 50 ) DEFAULT NULL, | "
	"varchar ( 100 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 50 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 50 ) COLLATE utf8_unicode_ci DEFAULT NULL, | "
	"varchar ( 50 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 100 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 100 ) COLLATE utf8_unicode_ci DEFAULT NULL, |" ),
	QStringLiteral ( "ID|Product or service|Brand|Type|Place|Payment method|Address|Delivery method|Account|Job type|Machine Name|Machine Event" ),
	CR_FIELDS_TYPE, TABLE_VERSION, CR_FIELD_COUNT, TABLE_COMPLETER_RECORDS_ORDER, 
	#ifdef COMPLETER_TABLE_UPDATE_AVAILABLE
	&updateCompleterRecordTable
	#else
	nullptr
	#endif //COMPLETER_TABLE_UPDATE_AVAILABLE
	#ifdef TRANSITION_PERIOD
	, true
	#endif
};

completerRecord::completerRecord ()
	: DBRecord ( CR_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = &( completerRecord::t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = CR_FIELDS_TYPE;
	DBRecord::helperFunction = this->helperFunction;
}

completerRecord::~completerRecord () {}

static inline uint translateCategory ( const COMPLETER_CATEGORIES category )
{
	uint field ( 0 );
	switch ( category )
	{
		case CC_NONE:														break;
		case CC_ALL_CATEGORIES:		field = FLD_CR_PRODUCT_OR_SERVICE_1;	break;
		case CC_PRODUCT_OR_SERVICE:	field = FLD_CR_PRODUCT_OR_SERVICE_2;	break;
		case CC_SUPPLIER:													break;
		case CC_BRAND:				field = FLD_CR_BRAND;					break;
		case CC_STOCK_TYPE:			field = FLD_CR_STOCK_TYPE;				break;
		case CC_STOCK_PLACE:		field = FLD_CR_STOCK_PLACE;				break;
		case CC_PAYMENT_METHOD:		field = FLD_CR_PAYMENT_METHOD;			break;
		case CC_ADDRESS:			field = FLD_CR_ADDRESS;					break;
		case CC_ITEM_NAMES:													break;
		case CC_CLIENT_NAME:												break;
		case CC_DELIVERY_METHOD:	field = FLD_CR_DELIVERY_METHOD;			break;
		case CC_ACCOUNT:			field = FLD_CR_ACCOUNT;					break;
		case CC_JOB_TYPE:			field = FLD_CR_JOB_TYPE;				break;
		case CC_MACHINE_NAME:		field = FLD_CR_MACHINE_NAME;			break;
		case CC_MACHINE_EVENT:		field = FLD_CR_MACHINE_EVENT;			break;
	}
	return field;
}

// This method does not check the value of str. It assumes it is new because vmCompleters did not find it in their lists
// Because of that, only vmCompleters can safely call it here
void completerRecord::updateTable ( const COMPLETER_CATEGORIES category, const QString& str, const bool b_reset_search )
{
	const uint field ( translateCategory ( category ) );
	if ( field  == 0 )
		return;
	
	bool b_needadd ( true );
	clearAll ( !b_reset_search );
	
	// When we are instructed to continue from the last used record ( !b_reset_search ) the first moment must be a reading from the first
	// record, and only the subsequent readings will continue from the current index
	if ( b_reset_search ? readFirstRecord () : readNextRecord () )
	{
		do
		{
			if ( recStrValue ( this, field ).isEmpty () )
			{
				b_needadd = false;
				break;
			}
		} while ( readNextRecord () );
	}
	setAction ( b_needadd ? ACTION_ADD : ACTION_EDIT );
	setRecValue ( this, field, str );
	saveRecord ();
}

void completerRecord::batchUpdateTable ( const COMPLETER_CATEGORIES category, QStringList& str_list )
{
	QStringList::const_iterator itr ( str_list.constBegin () );
	const QStringList::const_iterator& itr_end ( str_list.constEnd () );
	if ( itr != itr_end )
	{
		updateTable ( category, *itr++ ); // the first reading, reset the index
		for ( ; itr != itr_end; ++itr )
			updateTable ( category, *itr, false ); // subsequent writes, continue from the last
	}
	str_list.clear ();
}

void completerRecord::runQuery ( QStringList& results, const TABLE_INFO* t_info, const uint field, const bool b_check_duplicates )
{
	QSqlQuery query;
	if ( VivenciaDB::runSelectLikeQuery (
				QLatin1String ( "SELECT " ) + VivenciaDB::getTableColumnName ( t_info, field ) +
				QLatin1String ( " FROM " ) + t_info->table_name, query ) )
	{
		QString value;
		do
		{
			value = query.value ( 0 ).toString ();
			if ( !value.isEmpty () )
			{
				if ( b_check_duplicates )
				{
					if ( results.contains ( value, Qt::CaseInsensitive ) )
						continue;
				}
				results.append ( value );
			}
		} while ( query.next () );
	}
}

void completerRecord::loadCompleterStrings ( QStringList& completer_strings, const COMPLETER_CATEGORIES category )
{
	switch ( category )
	{
		case CC_CLIENT_NAME:
			runQuery ( completer_strings, &Client::t_info, FLD_CLIENT_NAME );
		break;
		case CC_SUPPLIER:
			runQuery ( completer_strings, &supplierRecord::t_info, FLD_SUPPLIER_NAME  );
		break;
		case CC_ITEM_NAMES:
			runQuery ( completer_strings, &dbSupplies::t_info, FLD_SUPPLIES_ITEM );
			runQuery ( completer_strings, &Inventory::t_info, FLD_INVENTORY_ITEM );
		break;
		default:
			runQuery ( completer_strings, &completerRecord::t_info, translateCategory ( category ) );
		break;
	}
}

void completerRecord::loadCompleterStringsForProductsAndServices ( QStringList& completer_strings, QStringList& completer_strings_2 )
{
	runQuery ( completer_strings, &completerRecord::t_info, FLD_CR_PRODUCT_OR_SERVICE_1 );
	runQuery ( completer_strings_2, &completerRecord::t_info, FLD_CR_PRODUCT_OR_SERVICE_2 );
}
