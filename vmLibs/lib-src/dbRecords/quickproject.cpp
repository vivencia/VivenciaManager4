#include "quickproject.h"
#include "vivenciadb.h"
#include "vmlibs.h"

static const unsigned int TABLE_VERSION ( 'B' );

constexpr DB_FIELD_TYPE QP_FIELDS_TYPE[QUICK_PROJECT_FIELD_COUNT] =
{
	DBTYPE_ID, DBTYPE_SHORTTEXT, DBTYPE_LIST, DBTYPE_NUMBER, DBTYPE_PRICE,
	DBTYPE_PRICE, DBTYPE_NUMBER, DBTYPE_PRICE, DBTYPE_PRICE, DBTYPE_PRICE
};

//#define QP_TABLE_UPDATE_AVAILABLE
#ifdef QP_TABLE_UPDATE_AVAILABLE

#include <vmStringRecord/stringrecord.h>
bool updateQuickProjectTable ( const unsigned char current_table_version )
{
	if ( current_table_version == 'A' )
	{
		quickProject qp;
		QString new_value;
		if ( qp.readFirstRecord () )
		{
			do
			{
				for ( uint i_col ( FLD_QP_ITEM ); i_col < FLD_QP_RESULT; ++i_col )
				{
					new_value = recStrValue ( &qp, i_col );
					if ( new_value.startsWith ( record_separator ) || new_value.startsWith ( table_separator ) )
					{
						qp.setAction ( ACTION_EDIT );
						new_value.remove ( 0, 1 );
						setRecValue ( &qp, i_col, new_value );
						qp.saveRecord ();
					}
				}
			} while ( qp.readNextRecord () );
			VivenciaDB::optimizeTable ( &quickProject::t_info );
			return true;
		}
	}
	return false;
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
