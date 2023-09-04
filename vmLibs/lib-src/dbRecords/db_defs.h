#ifndef GLOBAL_DB_H
#define GLOBAL_DB_H

#include <QtCore/QString>

static const QString STR_MYSQL ( QStringLiteral ( "mysql" ) );
static const QString SYSTEMD ( QStringLiteral ( "systemd" ) );
static const QString SYSTEM_ROOT_SERVICE ( QStringLiteral ( "SYSTEM_ROOT_SERVICE" ) );
static const QString SYSTEM_ROOT_PASSWORD_ID ( QStringLiteral ( "SYSTEM_ROOT_PASSWORD_ID" ) );

static const QString DB_NAME ( QStringLiteral ( "VivenciaDatabase4" ) );
static const QString USER_NAME ( QStringLiteral ( "vivenciamngr4" ) );
static const QString PASSWORD ( QStringLiteral ( "fenixfenix4" ) ) ;
static const QString DB_ROOT ( QStringLiteral ( "root" ) );

struct RECORD_FIELD
{

public:

	enum { IDX_ACTUAL = 0, IDX_TEMP = 1 };

	int i_field[2];
	bool modified;
	bool was_modified;
	QString str_field[2];

	RECORD_FIELD () : modified ( false ), was_modified ( false )
	{
		i_field[0] = i_field[1] = -1;
	}
};

enum DB_FIELD_TYPE
{
	DBTYPE_ID = 0, DBTYPE_YESNO = 1, DBTYPE_LIST, DBTYPE_FILE,
	DBTYPE_SHORTTEXT, DBTYPE_LONGTEXT, DBTYPE_SUBRECORD,
	DBTYPE_DATE, DBTYPE_TIME, DBTYPE_PHONE, DBTYPE_PRICE, DBTYPE_NUMBER
};

struct TABLE_INFO //compile-time set
{
	const unsigned int table_id;
	const QString table_name;
	const QString table_flags;
	const QString primary_key;
	const QString field_names;
	const QString field_flags;
	const QString field_labels;
	const DB_FIELD_TYPE* field_types;
	const unsigned char version;
	uint field_count;
	const unsigned int table_order;
	bool ( *update_func ) ( const unsigned char current_table_version );// New field: From version 2.0, upates are conducted within the originating module
#ifdef TRANSITION_PERIOD
	/* certain routines in the transition period from the 2 to the 3 series (re)create tables. These
	 * tables are created orderly and do not need, for example, to have their IDs rearranged. Other update
	 * routines might check this flag, although none does so far
	 */

	bool new_table;
#endif
};

static const unsigned int TABLES_IN_DB ( 14 );

enum TABLE_ORDER
{
	TABLE_GENERAL_ORDER = 0, TABLE_USERS_ORDER = 1, TABLE_CLIENT_ORDER = 2, TABLE_JOB_ORDER = 3, TABLE_PAY_ORDER = 4,
	TABLE_BUY_ORDER = 5, TABLE_QP_ORDER = 6, TABLE_SUPPLIES_ORDER = 7, TABLE_SUPPLIER_ORDER = 8,
	TABLE_CP_ORDER = 9, TABLE_INVENTORY_ORDER = 10, TABLE_COMPLETER_RECORDS_ORDER = 11,
	TABLE_CALENDAR_ORDER = 12, TABLE_MACHINES_ORDER = 13
};

enum TABLE_IDS
{
	GENERAL_TABLE = 2<<0, USERS_TABLE = 2<<1, CLIENT_TABLE = 2<<2, JOB_TABLE = 2<<3,
	PAYMENT_TABLE = 2<<4, PURCHASE_TABLE = 2<<5, QUICK_PROJECT_TABLE = 2<<6,
	SUPPLIES_TABLE = 2<<7, SUPPLIER_TABLE = 2<<8,
	COMPANY_PURCHASES_TABLE = 2<<9, INVENTORY_TABLE = 2<<10,
	COMPLETER_RECORDS_TABLE = 2<<11, CALENDAR_TABLE = 2<<12, MACHINES_TABLE = 2<<13
};

enum RECORD_ACTION
{
	ACTION_NONE = -2, ACTION_REVERT = -1, ACTION_READ = 0, ACTION_DEL = 1, ACTION_ADD = 2, ACTION_EDIT = 3
};

enum SEARCH_CATEGORIES
{
	SC_ID = 0, SC_REPORT_1, SC_REPORT_2, SC_ADDRESS_1, SC_ADDRESS_2, SC_ADDRESS_3,
	SC_ADDRESS_4, SC_ADDRESS_5, SC_PHONE_1, SC_PHONE_2, SC_PHONE_3,
	SC_PRICE_1, SC_PRICE_2, SC_DATE_1, SC_DATE_2, SC_DATE_3, SC_TIME_1,
	SC_TIME_2, SC_TYPE, SC_EXTRA_1, SC_EXTRA_2
};

enum ITEMS_AND_SERVICE_RECORD
{
	ISR_NAME = 0, ISR_UNIT = 1, ISR_BRAND = 2, ISR_QUANTITY = 3, ISR_UNIT_PRICE = 4,
	ISR_TOTAL_PRICE = 5, ISR_SUPPLIER = 6, ISR_ID = 7, ISR_OWNER = 8, ISR_DATE = 9
};

enum PAY_HISTORY_RECORD
{
	PHR_DATE = 0, PHR_VALUE, PHR_PAID, PHR_METHOD, PHR_ACCOUNT, PHR_USE_DATE
};

#endif // GLOBAL_DB_H
