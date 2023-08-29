#include "generaltable.h"
#include "vivenciadb.h"

static const unsigned char GENERAL_FIELD_TABLE_VERSION ( 'A' );

constexpr DB_FIELD_TYPE GENERAL_FIELDS_TYPE[GENERAL_FIELD_COUNT] =
{
	DBTYPE_ID, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_NUMBER, DBTYPE_FILE, DBTYPE_SHORTTEXT
};

bool updateGeneralTable ( const unsigned char /*current_table_version*/	)
{
	return false;
//#ifdef TABLE_UPDATE_AVAILABLE
//	VDB ()->deleteTable ( generalTable::t_info.table_name );
//	VDB ()->createTable ( &generalTable::t_info );
//	VDB ()->optimizeTable ( &generalTable::t_info );
//	return true;
//#else
//	VivenciaDB::optimizeTable ( &generalTable::t_info );
//	return false;
//#endif
}

const TABLE_INFO generalTable::t_info =
{
	GENERAL_TABLE,
	QStringLiteral ( "GENERAL" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ) , UNIQUE KEY `id` ( `ID` ) " ),
	QStringLiteral ( "`ID`|`TABLE_NAME`|`TABLE_VERSION`|`TABLE_ID`|`CONFIG_FILE`|`EXTRA`|" ),
	QStringLiteral ( " int ( 4 ) NOT NULL, | varchar ( 20 ) NOT NULL, | varchar ( 10 ) NOT NULL, | varchar ( 20 ) NOT NULL, |"
	" varchar ( 255 ) COLLATE utf8_unicode_ci DEFAULT NULL, | varchar ( 50 ) DEFAULT NULL, |"),
	QStringLiteral ( "ID|Table Name|Table Version|Table ID|Oil Change|Config File|Extra|" ),
	GENERAL_FIELDS_TYPE, GENERAL_FIELD_TABLE_VERSION,
	GENERAL_FIELD_COUNT, TABLE_GENERAL_ORDER, &updateGeneralTable
	#ifdef TABLE_UPDATE_AVAILABLE
	//, false
	#endif
};

generalTable::generalTable ()
	: DBRecord ( GENERAL_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = &( generalTable::t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::mFieldsTypes = GENERAL_FIELDS_TYPE;
	DBRecord::helperFunction = this->helperFunction;
}

generalTable::~generalTable () {}

void generalTable::insertOrUpdate ( const TABLE_INFO* const t_info )
{
	const QString genIDStr ( QString::number ( t_info->table_order + 1 ) );

	if ( readRecord ( FLD_GENERAL_ID, genIDStr ) )
	{
		setAction ( ACTION_EDIT );
		setRecValue ( this, FLD_GENERAL_TABLENAME, t_info->table_name );
		setRecValue ( this, FLD_GENERAL_TABLEVERSION, QString ( t_info->version ) );
	}
	else
	{
		setAction ( ACTION_ADD );
		setRecIntValue ( this, FLD_GENERAL_ID, static_cast<int>( t_info->table_order + 1 ) );
		setRecValue ( this, FLD_GENERAL_ID, genIDStr );
		setRecValue ( this, FLD_GENERAL_TABLENAME, t_info->table_name );
		setRecValue ( this, FLD_GENERAL_TABLEVERSION, QString ( t_info->version ) );
		setRecValue ( this, FLD_GENERAL_TABLEID, QString::number ( t_info->table_id ) );
		setRecValue ( this, FLD_GENERAL_CONFIG_FILE, QString () );
		setRecValue ( this, FLD_GENERAL_EXTRA, QString () );
	}
	static_cast<void>( saveRecord () );
}
