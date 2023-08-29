#include "companypurchases.h"
#include "vmlibs.h"
#include "supplierrecord.h"
#include "vivenciadb.h"
#include "inventory.h"

#include <QtCore/QCoreApplication>

const unsigned int TABLE_VERSION ( 'A' );

constexpr DB_FIELD_TYPE CP_FIELDS_TYPE[COMPANY_PURCHASES_FIELD_COUNT] =
{
	DBTYPE_ID, DBTYPE_DATE, DBTYPE_DATE, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_PRICE,
	DBTYPE_PRICE, DBTYPE_LIST, DBTYPE_LIST
};

bool updateCPTable ( const unsigned char /*current_table_version*/ )
{
	return false;
}

const TABLE_INFO companyPurchases::t_info =
{
	COMPANY_PURCHASES_TABLE,
	QStringLiteral ( "COMPANY_PURCHASES" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ), UNIQUE KEY `id` ( `ID` )" ),
	QStringLiteral ( "`ID`|`DATE`|`DELIVERY_DATE`|`DELIVERY_METHOD`|`NOTES`|`SUPPLIER`|"
	"`TOTAL_PRICE`|`PAY_VALUE`|`ITEM_REPORT`|`PAY_REPORT`|" ),
	QStringLiteral ( " int ( 9 ) NOT NULL, | varchar ( 20 ) DEFAULT NULL, | varchar ( 20 ) DEFAULT NULL, |"
	" varchar ( 30 ) DEFAULT NULL, | varchar ( 200 ) DEFAULT NULL, | varchar ( 100 ) DEFAULT NULL, |"
	" varchar ( 30 ) DEFAULT NULL, | varchar ( 30 ) DEFAULT NULL, | longtext COLLATE utf8_unicode_ci DEFAULT NULL, |"
	" longtext COLLATE utf8_unicode_ci DEFAULT NULL, |" ),
	QCoreApplication::tr ( "ID|Purchase date|Delivery date|Delivery method|Notes|Supplier|Total price|Payment value|Items bought|Payment history|" ),
	CP_FIELDS_TYPE, TABLE_VERSION, COMPANY_PURCHASES_FIELD_COUNT, TABLE_CP_ORDER, &updateCPTable
	#ifdef TRANSITION_PERIOD
	, true
	#endif
};

static void updateCPPayReport ( const DBRecord* db_rec )
{
	//TODO
	DBRecord::completerManager ()->updateCompleter ( db_rec, FLD_CP_PAY_REPORT, STOCK_TYPE );
}

static void updateSupplierTable ( const DBRecord* db_rec )
{
	supplierRecord::insertIfSupplierInexistent ( recStrValue ( db_rec, FLD_CP_SUPPLIER ) );
}

static void updateCPDeliverMethodCompleter ( const DBRecord* db_rec )
{
	DBRecord::completerManager ()->updateCompleter ( db_rec, FLD_CP_DELIVERY_METHOD, DELIVERY_METHOD );
}

companyPurchases::companyPurchases ( const bool connect_helper_funcs )
	: DBRecord ( COMPANY_PURCHASES_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = &( this->t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = CP_FIELDS_TYPE;
	
	if ( connect_helper_funcs )
	{
		DBRecord::helperFunction = this->helperFunction;
		setHelperFunction ( FLD_CP_PAY_REPORT, &updateCPPayReport );
		setHelperFunction ( FLD_CP_SUPPLIER, &updateSupplierTable );
		setHelperFunction ( FLD_CP_DELIVERY_METHOD, &updateCPDeliverMethodCompleter );
	}
}

companyPurchases::~companyPurchases () {}

void companyPurchases::exportToInventory ()
{
	Inventory inv_rec;
	const stringTable& cp_itemreport ( recStrValue ( this, FLD_CP_ITEMS_REPORT ) );
	const stringRecord* item_report ( nullptr );
	if ( ( item_report = &cp_itemreport.first () ) )
	{
		do {
			inv_rec.setAction ( ACTION_ADD );
			setRecValue ( &inv_rec, FLD_INVENTORY_ITEM, item_report->fieldValue ( ISR_NAME ) );
			setRecValue ( &inv_rec, FLD_INVENTORY_BRAND, item_report->fieldValue ( ISR_BRAND ) );
			setRecValue ( &inv_rec, FLD_INVENTORY_QUANTITY, item_report->fieldValue ( ISR_QUANTITY ) );
			setRecValue ( &inv_rec, FLD_INVENTORY_UNIT, item_report->fieldValue ( ISR_UNIT ) );
			setRecValue ( &inv_rec, FLD_INVENTORY_TYPE, APP_TR_FUNC ( "Company purchased item" ) );
			setRecValue ( &inv_rec, FLD_INVENTORY_DATE_IN, recStrValue ( this, FLD_CP_DATE ) );
			setRecValue ( &inv_rec, FLD_INVENTORY_PRICE, item_report->fieldValue ( ISR_UNIT_PRICE ) );
			setRecValue ( &inv_rec, FLD_INVENTORY_SUPPLIER, recStrValue ( this, FLD_CP_SUPPLIER ) );
			setRecValue ( &inv_rec, FLD_INVENTORY_PLACE, APP_TR_FUNC ( "Company purchases storage room" ) );
			item_report = &cp_itemreport.next ();
			inv_rec.saveRecord ();
			inv_rec.clearAll ();
		} while ( item_report->isOK () );
	}
}
