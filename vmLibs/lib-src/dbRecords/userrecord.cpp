#include "userrecord.h"
#include "vivenciadb.h"

static const unsigned char USERS_FIELD_TABLE_VERSION ( 'A' );

constexpr DB_FIELD_TYPE USERS_FIELDS_TYPE[USERS_FIELD_COUNT] =
{
	DBTYPE_ID, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT, DBTYPE_SHORTTEXT
};

#ifdef USER_TABLE_UPDATE_AVAILABLE
bool updateUsersTable ()
{
	//VDB ()->deleteTable ( &userRecord::t_info );
	//VDB ()->createTable ( &userRecord::t_info );
	VDB ()->optimizeTable ( &userRecord::t_info );
	return true;
}
#endif //USER_TABLE_UPDATE_AVAILABLE

const TABLE_INFO userRecord::t_info =
{
	USERS_TABLE,
	QStringLiteral ( "USERS" ),
	QStringLiteral ( " ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci" ),
	QStringLiteral ( " PRIMARY KEY ( `ID` ) , UNIQUE KEY `id` ( `ID` ) " ),
	QStringLiteral ( "`ID`|`USR_INFO`|`PRIVILEDGES`|`LOCATION`|`LOGGED_USERS`|" ),
	QStringLiteral ( " int ( 4 ) NOT NULL, | varchar ( 200 ) NOT NULL, |"
	" int ( 32 ) UNSIGNED NOT NULL, | varchar ( 50 ) DEFAULT NULL, | longtext COLLATE utf8_unicode_ci DEFAULT NULL, |" ),
	QStringLiteral ( "ID|User name|Password|Privileges|Location|Logged Users|" ),
	USERS_FIELDS_TYPE, USERS_FIELD_TABLE_VERSION, USERS_FIELD_COUNT, TABLE_USERS_ORDER,
	#ifdef USER_TABLE_UPDATE_AVAILABLE
	&updateUsersTable
	#else
	nullptr
	#endif //USER_TABLE_UPDATE_AVAILABLE
	#ifdef TRANSITION_PERIOD
	, false
	#endif
};

userRecord::userRecord ()
	: DBRecord ( USERS_FIELD_COUNT )
{
	::memset ( this->helperFunction, 0, sizeof ( this->helperFunction ) );
	DBRecord::t_info = &( userRecord::t_info );
	DBRecord::m_RECFIELDS = this->m_RECFIELDS;
	DBRecord::helperFunction = this->helperFunction;
	DBRecord::mFieldsTypes = USERS_FIELDS_TYPE;
}

userRecord::~userRecord () {}
