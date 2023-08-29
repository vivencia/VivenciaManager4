#include "supplierrecord.h"
#include "vmlibs.h"
#include "vivenciadb.h"

static const unsigned int TABLE_VERSION ( 'A' );

constexpr DB_FIELD_TYPE SUPPLIER_FIELDS_TYPE[SUPPLIER_FIELD_COUNT] =
{
	DBTYPE_ID, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_NUMBER, DBTYPE_SHORTTEXT,
	DBTYPE_SHORTTEXT, DBTYPE_PHONE, DBTYPE_SHORTTEXT
};

#ifdef SUPPLIER_TABLE_UPDATE_AVAILABLE
bool updateSupplierTable ()
{
	VDB ()->optimizeTable ( &supplierRecord::t_info );
	return true;
}
#endif //SUPPLIER_TABLE_UPDATE_AVAILABLE

const TABLE_INFO supplierRecord::t_info =
{
	SUPPLIER_TABLE,
	QStringLiteral ( "SUPPLIERS" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ) , UNIQUE KEY `id` ( `ID` ) " ),
	QStringLiteral ( "`ID`|`NAME`|`STREET`|`NUMBER`|`DISTRICT`|`CITY`|`PHONES`|`EMAIL`|" ),
	QStringLiteral ( " int ( 9 ) NOT NULL, | varchar ( 100 ) COLLATE utf8_unicode_ci DEFAULT NULL, |"
	" varchar ( 100 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 10 ) DEFAULT NULL, |"
	" varchar ( 50 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 50 ) COLLATE utf8_unicode_ci DEFAULT NULL, |"
	" varchar ( 100 ) COLLATE utf8_unicode_ci DEFAULT NULL, |varchar ( 200 ) COLLATE utf8_unicode_ci DEFAULT NULL, |" ),
	QStringLiteral ( "ID|Name|Street|Number|District|City|Phones|E-mail|" ),
	SUPPLIER_FIELDS_TYPE, TABLE_VERSION, SUPPLIER_FIELD_COUNT, TABLE_SUPPLIER_ORDER,
	#ifdef SUPPLIER_TABLE_UPDATE_AVAILABLE
	&updateSupplierTable
	#else
	nullptr
	#endif //SUPPLIER_TABLE_UPDATE_AVAILABLE
	#ifdef TRANSITION_PERIOD
	, false
	#endif
};

static void updateSupplierNameCompleter ( const DBRecord* db_rec )
{
	DBRecord::completerManager ()->updateCompleter ( db_rec, FLD_SUPPLIER_NAME, SUPPLIER );
}

static void updateSupplierStreetCompleter ( const DBRecord* db_rec )
{
	DBRecord::completerManager ()->updateCompleter ( db_rec, FLD_SUPPLIER_STREET, ADDRESS );
}

static void updateSupplierDistrictCompleter ( const DBRecord* db_rec )
{
	DBRecord::completerManager ()->updateCompleter ( db_rec, FLD_SUPPLIER_DISTRICT, ADDRESS );
}

static void updateSupplierCityCompleter ( const DBRecord* db_rec )
{
	DBRecord::completerManager ()->updateCompleter ( db_rec, FLD_SUPPLIER_CITY, ADDRESS );
}

supplierRecord::supplierRecord ( const bool connect_helper_funcs )
	: DBRecord ( SUPPLIER_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = & ( this->t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = SUPPLIER_FIELDS_TYPE;

	if ( connect_helper_funcs )
	{
		DBRecord::helperFunction = this->helperFunction;
		setHelperFunction ( FLD_SUPPLIER_NAME, &updateSupplierNameCompleter );
		setHelperFunction ( FLD_SUPPLIER_STREET, &updateSupplierStreetCompleter );
		setHelperFunction ( FLD_SUPPLIER_DISTRICT, &updateSupplierDistrictCompleter );
		setHelperFunction ( FLD_SUPPLIER_CITY, &updateSupplierCityCompleter );
	}
}

supplierRecord::~supplierRecord () {}

void supplierRecord::insertIfSupplierInexistent ( const QString& supplier )
{
	QSqlQuery query;
	if ( !VivenciaDB::runSelectLikeQuery ( QLatin1String ( "SELECT ID FROM SUPPLIERS WHERE NAME=\'" ) + supplier + QLatin1Char ( '\'' ), query ) )
	{
		supplierRecord sup_rec ( true );
		sup_rec.setAction ( ACTION_ADD );
		sup_rec.setValue ( FLD_SUPPLIER_NAME, supplier );
		sup_rec.saveRecord ();
	}
}
