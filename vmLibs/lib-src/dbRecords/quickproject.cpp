#include "quickproject.h"
#include "vivenciadb.h"
#include "vmlibs.h"

static const unsigned int TABLE_VERSION ( 'A' );

constexpr DB_FIELD_TYPE QP_FIELDS_TYPE[QUICK_PROJECT_FIELD_COUNT] =
{
	DBTYPE_ID, DBTYPE_SHORTTEXT, DBTYPE_LIST, DBTYPE_NUMBER, DBTYPE_PRICE,
	DBTYPE_PRICE, DBTYPE_NUMBER, DBTYPE_PRICE, DBTYPE_PRICE, DBTYPE_PRICE
};

#ifdef QP_TABLE_UPDATE_AVAILABLE
bool updateQuickProjectTable ()
{
	VDB ()->optimizeTable ( &quickProject::t_info );
	return true;
}
#endif //QP_TABLE_UPDATE_AVAILABLE

const TABLE_INFO quickProject::t_info =
{
	QUICK_PROJECT_TABLE,
	QStringLiteral ( "QUICK_PROJECT" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ), UNIQUE KEY `id` ( `ID` )" ),
	QStringLiteral ( "`ID`|`PROJECT_ID`|`ITEM`|`SELL_QUANTITY`|`SELL_UNIT_PRICE`|`SELL_TOTAL_PRICE`|"
	"`PURCHASE_QUANTITY`|`PURCHASE_UNIT_PRICE`|`PURCHASE_TOTAL_PRICE`|`RESULT`|" ),
	QStringLiteral ( " int ( 9 ) NOT NULL, | int ( 9 ) NOT NULL, | longtext COLLATE utf8_unicode_ci, | longtext COLLATE utf8_unicode_ci, |"
	" longtext COLLATE utf8_unicode_ci, | longtext COLLATE utf8_unicode_ci, | longtext COLLATE utf8_unicode_ci, | longtext COLLATE utf8_unicode_ci, |"
	" longtext COLLATE utf8_unicode_ci, | longtext COLLATE utf8_unicode_ci, |" ),
	QStringLiteral ( "ID|Project ID|Item|Quantity|Selling price (un)|Selling price (total)|Purchase quantity|Purchase price (un)|Purchase price (total)|Result|" ),
	QP_FIELDS_TYPE, TABLE_VERSION, QUICK_PROJECT_FIELD_COUNT, TABLE_QP_ORDER,
	#ifdef QP_TABLE_UPDATE_AVAILABLE
	&updateQuickProjectTable
	#else
	nullptr
	#endif //QP_TABLE_UPDATE_AVAILABLE
	#ifdef TRANSITION_PERIOD
	, false
	#endif
};

quickProject::quickProject ()
	: DBRecord ( QUICK_PROJECT_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = & ( quickProject::t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = QP_FIELDS_TYPE;
	DBRecord::helperFunction = this->helperFunction;
}

quickProject::~quickProject () {}
