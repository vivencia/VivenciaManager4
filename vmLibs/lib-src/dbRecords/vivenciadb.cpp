#include "vivenciadb.h"
#include "vmlibs.h"
#include "vmlist.h"
#include "heapmanager.h"

#include "client.h"
#include "job.h"
#include "payment.h"
#include "purchases.h"
#include "quickproject.h"
#include "supplierrecord.h"
#include "userrecord.h"
#include "generaltable.h"
#include "companypurchases.h"
#include "inventory.h"
#include "dbsupplies.h"
#include "completerrecord.h"
#include "machinesrecord.h"
#include "dbcalendar.h"

#include <vmUtils/vmtextfile.h>
#include <vmUtils/vmcompress.h>
#include <vmUtils/fileops.h>
#include <vmUtils/configops.h>
#include <vmNotify/vmnotify.h>
#include <vmStringRecord/stringrecord.h>
#include <vmWidgets/vmtablewidget.h>

#include <QtSql/QSqlError>
#include <QtSql/QSqlResult>
#include <QtCore/QStringMatcher>
#include <QtCore/QTextStream>
#include <QtCore/QScopedPointer>

#ifdef USING_QT6
#include <QtCore/QStringView>
#include <QtCore/QRegularExpression>
#else
#include <QtCore/QRegExp>
#endif

podList<uint> VivenciaDB::currentHighestID_list ( 0, TABLES_IN_DB + 1 );

static const QLatin1String chrDelim2 ( "\'," );

const TABLE_INFO* const VivenciaDB::table_info[TABLES_IN_DB] = {
	&generalTable::t_info,
	&userRecord::t_info,
	&Client::t_info,
	&Job::t_info,
	&Payment::t_info,
	&Buy::t_info,
	&quickProject::t_info,
	&dbSupplies::t_info,
	&supplierRecord::t_info,
	&companyPurchases::t_info,
	&Inventory::t_info,
	&completerRecord::t_info,
	&dbCalendar::t_info,
	&machinesRecord::t_info
};

static const uint PORT ( 3306 );
static const QString HOST ( QStringLiteral ( "localhost" ) );
static const QString CONNECTION_NAME ( QStringLiteral ( "vivenciadb" ) );
#ifdef USING_QT6
static const QString DB_DRIVER_NAME ( QStringLiteral ( "QMYSQL" ) );
#else
static const QString DB_DRIVER_NAME ( QStringLiteral ( "QMYSQL3" ) );
#endif
static const QString ROOT_CONNECTION ( QStringLiteral ( "root_connection" ) );

//-----------------------------------------STATIC---------------------------------------------
QString VivenciaDB::getTableColumnName ( const TABLE_INFO* __restrict t_info, uint column )
{
	int sep_1 ( 0 ) , sep_2 ( -1 );

	QString col_name;
	while ( ( sep_2 = t_info->field_names.indexOf ( CHR_PIPE, sep_1 ) ) != -1 )
	{
		if ( column-- == 0 )
		{
			col_name = t_info->field_names.mid ( sep_1, sep_2-sep_1 );
			break;
		}
		sep_1 = ( sep_2 ) + 1;
	}
	return col_name;
}

QString VivenciaDB::getTableColumnFlags ( const TABLE_INFO* __restrict t_info, uint column )
{
	int sep_1 ( 0 ), sep_2 ( -1 );

	QString col_flags;
	while ( ( sep_2 = t_info->field_flags.indexOf ( CHR_PIPE, sep_1 ) ) != -1 )
	{
		if ( column-- == 0 )
		{
			col_flags = t_info->field_flags.mid ( sep_1, sep_2-sep_1 );
			col_flags.chop ( 2 );
			break;
		}
		sep_1 = ( sep_2 ) + 1;
	}
	return col_flags;
}

QString VivenciaDB::getTableColumnLabel ( const TABLE_INFO* __restrict t_info, uint column )
{
	int sep_1 ( 0 ) , sep_2 ( -1 );

	QString col_label;
	while ( ( sep_2 = t_info->field_labels.indexOf ( CHR_PIPE, sep_1 ) ) != -1 )
	{
		if ( column-- == 0 )
		{
			col_label = t_info->field_labels.mid ( sep_1, sep_2 - sep_1 );
			break;
		}
		sep_1 = ( sep_2 ) + 1;
	}
	return col_label;
}

int VivenciaDB::getTableColumnIndex ( const TABLE_INFO* t_info, const QString& column_name )
{
	for ( uint i ( 0 ); i < t_info->field_count; ++i )
	{
		if ( getTableColumnName ( t_info, i ) == column_name )
			return static_cast<int>( i );
	}
	return -1;
}

uint VivenciaDB::tableIDFromTableName ( const QString& strTableName )
{
	for ( uint i ( 0 ); i < TABLES_IN_DB; ++i )
	{
		if ( table_info[i]->table_name == strTableName )
			return table_info[i]->table_id;
	}
	return 0;
}

QString VivenciaDB::tableName ( const TABLE_ORDER table )
{	
	QString ret;
	switch ( table )
	{
		case TABLE_GENERAL_ORDER:
			ret = generalTable::t_info.table_name;
		break;
		case TABLE_USERS_ORDER:
			ret = userRecord::t_info.table_name;
		break;
		case TABLE_CLIENT_ORDER:
			ret = Client::t_info.table_name;
		break;
		case TABLE_JOB_ORDER:
			ret = Job::t_info.table_name;
		break;
		case TABLE_PAY_ORDER:
			ret = Payment::t_info.table_name;
		break;
		case TABLE_BUY_ORDER:
			ret = Buy::t_info.table_name;
		break;
		case TABLE_QP_ORDER:
			ret = quickProject::t_info.table_name;
		break;
		case TABLE_SUPPLIES_ORDER:
			ret = dbSupplies::t_info.table_name;
		break;
		case TABLE_SUPPLIER_ORDER:
			ret = supplierRecord::t_info.table_name;
		break;
		case TABLE_CP_ORDER:
			ret = companyPurchases::t_info.table_name;
		break;
		case TABLE_INVENTORY_ORDER:
			ret = Inventory::t_info.table_name;
		break;
		case TABLE_COMPLETER_RECORDS_ORDER:
			ret = completerRecord::t_info.table_name;
		break;
		case TABLE_CALENDAR_ORDER:
			ret = dbCalendar::t_info.table_name;
		break;
		case TABLE_MACHINES_ORDER:
			ret = machinesRecord::t_info.table_name;
		break;
	}
	return ret;
}

const QString VivenciaDB::mysqlInitScript ()
{
	static QString script;
	if ( script.isEmpty() )
	{
		if ( fileOps::exists ( QStringLiteral ( "/etc/init.d/mariadb" ) ).isOn () )
			script = QStringLiteral ( "/etc/init.d/mariadb" );
		else if ( fileOps::exists ( QStringLiteral ( "/etc/init.d/mysql" ) ).isOn () )
			script = QStringLiteral ( "/etc/init.d/mysql" );
	}
	return script;
}

//-----------------------------------------STATIC---------------------------------------------

VivenciaDB::VivenciaDB ()
	: m_db ( nullptr ), m_ok ( false ), mNewDB ( false ), mBackupSynced ( true ),
      m_progressMaxSteps_func ( nullptr ), m_errorHandling_func ( nullptr )
{
	//Avoid constant ifs for checking whether this callback was set or not
	m_progressStepUp_func = [&] () { MSG_OUT("VivenciaDB working...") };
}

VivenciaDB::~VivenciaDB ()
{
	if ( DBRecord::databaseManager () == this )
		DBRecord::setDatabaseManager ( nullptr );
	database ()->close ();
	delete m_db;
}

//-----------------------------------------MYSQL-------------------------------------------
bool VivenciaDB::isMySQLRunning ( const bool b_firsttimerun )
{
	QStringList args;

	if ( b_firsttimerun )
	{
		args <<
				QStringLiteral ( "status" );

#ifdef USING_QT6
		const QStringView outStr ( fileOps::executeAndCaptureOutput ( args, mysqlInitScript (), true ) );
		return ( static_cast<bool>( outStr.indexOf ( QRegularExpression ( QStringLiteral ( "OK|active (running)|start/running|Server version" ) ) ) != -1 ) );
#else
		const QString outStr ( fileOps::executeAndCaptureOutput ( args, mysqlInitScript (), true ) );
		return ( static_cast<bool>( outStr.indexOf ( QRegExp ( QStringLiteral ( "OK|active (running)|start/running|Server version" ) ) ) != -1 ) );
#endif
	}
	else //New code as of 2023-10-26 - MX Linux 23
	{

		args <<
				QStringLiteral ( "--user=") + USER_NAME <<
				QStringLiteral ( "--password=" ) + PASSWORD <<
				QStringLiteral ( "status" );
#ifdef USING_QT6
		const QStringView outStr ( fileOps::executeAndCaptureOutput ( args, adminApp () ) );
#else
		const QString outStr ( fileOps::executeAndCaptureOutput ( args, adminApp () ) );
#endif
		return ( static_cast<bool>( outStr.indexOf ( QStringLiteral ( "Uptime:" ) ) != -1 ) );
	}
}

/*bool VivenciaDB::isMySQLRunning ( const bool b_firsttimerun )
{
	QStringList args ( QStringLiteral ( "status" ) );
#ifdef USING_QT6
	QStringView
#else
	QString
#endif
			//The first time this probram is run we must exec the script using root.
			//Even though we might have added the user to the mysl group (which owns access to the mysql init script)
			//We will not have the permission effectively in place until the next login
			 outStr ( fileOps::executeAndCaptureOutput ( args, mysqlInitScript (), b_firsttimerun ) );

	//if ( fileOps::exists ( QStringLiteral ( "/etc/lsb-release" ) ).isOn () ) // *buntu derivatives
	//{

	//The regular expression looks for several possible outputs of the status script. Theses options were what I found along the years, with different distributions
#ifdef USING_QT6
		return ( static_cast<bool>( outStr.indexOf ( QRegularExpression ( QStringLiteral ( "OK|active (running)|start/running|Server version" ) ) ) != -1 ) );
#else
		return ( static_cast<bool>( outStr.indexOf ( QRegExp ( QStringLiteral ( "OK|active (running)|start/running|Server version" ) ) ) != -1 ) );
#endif
		//if ( configOps::initSystem ( SYSTEMD ) )
		//	return ( static_cast<bool>( outStr.indexOf ( QStringLiteral ( "active (running)" ) ) != -1 ) );
		//return ( static_cast<bool>( outStr.indexOf ( QStringLiteral ( "start/running, process" ) ) != -1 ) );
	//}
	//return ( static_cast<bool>( outStr.indexOf ( QStringLiteral ( "OK" ) ) != -1 ) );
}*/

DB_ERROR_CODES VivenciaDB::commandMySQLServer ( const QString& command, const bool b_firsttimerun )
{
	QStringList args ( command );
	//In the first run all around must be root to use mysql. After that, the regular user will have access to the mysql tools and database
	if ( !fileOps::executeWait ( args, mysqlInitScript (), b_firsttimerun ) )
		return ERR_COMMAND_MYSQL;
	return NO_ERR;
}

DB_ERROR_CODES VivenciaDB::checkDatabase ( VivenciaDB* vdb, const bool b_firsttimerun )
{
	bool ok ( false );
	ok = isMySQLRunning ( b_firsttimerun );
	if ( !ok )
	{
		ok = VivenciaDB::commandMySQLServer ( QStringLiteral ( "start" ), b_firsttimerun ) == NO_ERR;
	}

	if ( ok )
	{
		if ( vdb == nullptr )
		{
			QScopedPointer<VivenciaDB> temp_vdb ( new VivenciaDB );
			vdb = temp_vdb.data ();
		}

		if ( !vdb->openDataBase () )
			return ERR_NO_DB;
		if ( vdb->databaseIsEmpty ( vdb ) )
			return ERR_DB_EMPTY;
	}
	else
	{
		static_cast<void>( vmNotify::notifyMessage ( nullptr, APP_TR_FUNC ( "Could not connect to mysql server" ),
										 APP_TR_FUNC (	"Make sure the packages mysql-client/server are installed. "
														"The application will now exit. Try to manually start the mysql daemon"
														" or troubleshoot for the problem if it persists." ), 5000, true ) );
		return ERR_DATABASE_PROBLEM;
	}
	return NO_ERR;
}
//-----------------------------------------MYSQL-------------------------------------------

//-----------------------------------------READ-OPEN-LOAD-------------------------------------------
bool VivenciaDB::openDataBase ( const QString& connecion_name )
{
	if ( database () )
	{
		if ( database ()->isOpen () )
			return true;
	}
	else
	{
		m_db = new QSqlDatabase ( QSqlDatabase::addDatabase ( DB_DRIVER_NAME, connecion_name.isEmpty () ? QSqlDatabase::defaultConnection : connecion_name ) );
		database ()->setHostName ( HOST );
		database ()->setPort ( PORT );
		database ()->setUserName ( USER_NAME );
		database ()->setPassword ( PASSWORD );
		database ()->setDatabaseName ( DB_NAME );
	}
	return ( m_ok = database ()->open () );
}

bool VivenciaDB::restart ()
{
	if ( database () )
	{
		if ( database ()->isOpen () )
		{
			database ()->close ();
			return openDataBase ();
		}
	}
	return false;
}

void VivenciaDB::doPreliminaryWork ()
{
	if ( !mNewDB )
	{
		generalTable gen_rec;
		bool b_version_mismatch ( false );
		unsigned char current_version ( 'A' );
		for ( uint i ( 1 ); i <= TABLES_IN_DB; ++i )
		{
			if ( gen_rec.readRecord ( i ) )
			{
				const QString& version ( recStrValue ( &gen_rec, FLD_GENERAL_TABLEVERSION ) );
				if ( !version.isEmpty () )
				{
					current_version = static_cast<unsigned char>( version.at ( 0 ).toLatin1 () );
					b_version_mismatch = table_info[i-1]->version != current_version;
				}
				else
					b_version_mismatch = true;
			}
			else
				b_version_mismatch = true;

			// regardless of a new version, if there is an update function, call it. Some maintanance work must sometimes be made
			if ( table_info[i-1]->update_func )
			{
				if ( (*table_info[i-1]->update_func) ( current_version ) )
				{
					if ( b_version_mismatch ) // update the GENERAL_TABLE with the current table version
						gen_rec.insertOrUpdate ( table_info[i-1] );
				}
			}
			else // no update function, only fix GENERAL_TABLE
			{
				if ( b_version_mismatch ) // update the GENERAL_TABLE with the current table version
					gen_rec.insertOrUpdate ( table_info[i-1] );
			}
		}
	}
}
//-----------------------------------------READ-OPEN-LOAD-------------------------------------------

//-----------------------------------------CREATE-DESTROY-MODIFY-------------------------------------
bool VivenciaDB::createDatabase ( const bool bCreateTables )
{
	if ( !openDataBase () )
	{
		QString workingRootPswd;
		if ( checkRootPasswordForMySQL ( workingRootPswd ) )
		{
            QSqlDatabase dbRoot ( QSqlDatabase::addDatabase ( DB_DRIVER_NAME, ROOT_CONNECTION ) );
            dbRoot.setDatabaseName ( STR_MYSQL );
            dbRoot.setHostName ( HOST );
			dbRoot.setUserName ( DB_ROOT );
			dbRoot.setPassword ( workingRootPswd );
            if ( dbRoot.open () )
            {
                const QString str_query ( QLatin1String ( "CREATE DATABASE " ) + DB_NAME +
											QLatin1String ( " DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci" ) );
                QSqlQuery rootQuery ( dbRoot );
                m_ok = rootQuery.exec ( str_query );
                rootQuery.finish ();
                dbRoot.commit ();
                dbRoot.close ();
                rootQuery.clear ();
            }
            QSqlDatabase::removeDatabase ( ROOT_CONNECTION );
			if ( createUser ( workingRootPswd ) )
			{
				if ( bCreateTables )
					return createAllTables ();
				return true;
			}
		}
		vmNotify::notifyMessage ( nullptr, APP_TR_FUNC ( "Database error" ), APP_TR_FUNC ( "MYSQL could create the database. Exiting..." ), 5000, true );
		return false;
	}
	return true;
}

//The root password is not stored by the application
bool VivenciaDB::checkRootPasswordForMySQL ( QString& password )
{
	if ( vmNotify::passwordBox ( password, nullptr, APP_TR_FUNC ( "Root password" ),
								 APP_TR_FUNC ( "Please enter the current mysql root password. Or, if it isn't set yet, provide a new root password" ) ) )
	{

		QStringList mysqladmin_args ( QStringList () <<
									QStringLiteral ( "status" ) <<
									QStringLiteral ( "-u" ) <<
									DB_ROOT <<
									QStringLiteral ( "-p'" ) + password + CHR_CHRMARK );

		const QString mysqladmin_output ( fileOps::executeAndCaptureOutput ( mysqladmin_args, adminApp () ) );
		//The entered password is either new or incorrect
		if ( mysqladmin_output.contains ( QStringLiteral ( "Access denied" ) ) ) //"error: 'Access denied for user 'root'@'localhost'"
		{
			//Maybe root password is not set yet. Assume the root password given is new
			if ( !changeRootPassword ( emptyString, password ) )
			{
				vmNotify::notifyMessage ( nullptr, APP_TR_FUNC ( "Incorrect root password. Try again!" ),
										  APP_TR_FUNC ( "Please provide the correct password for the current mysql setup" ), 6000, true );
				return checkRootPasswordForMySQL ( password );
			}
		}
		return true;
	}
	return false;
}

bool VivenciaDB::changeRootPassword ( const QString& oldpasswd, const QString& newpasswd )
{
	QStringList mysqladmin_args ( QStringList ()  <<
								QStringLiteral ( "-u" ) <<
								DB_ROOT <<
								QStringLiteral ( "-p'" ) + oldpasswd + CHR_CHRMARK <<
								QStringLiteral ( "password" ) <<
								newpasswd );
	int e_code;

	const QString mysqladmin_output ( fileOps::executeAndCaptureOutput ( mysqladmin_args, adminApp (), true, &e_code ) );
	MSG_OUT(mysqladmin_output)
	return e_code == 0;
}

bool VivenciaDB::createUser ( const QString& root_passwd )
{
	m_ok = false;
    QSqlDatabase dbRoot ( QSqlDatabase::addDatabase ( DB_DRIVER_NAME, ROOT_CONNECTION ) );
    dbRoot.setDatabaseName ( STR_MYSQL );
    dbRoot.setHostName ( HOST );
	dbRoot.setUserName ( DB_ROOT );
	dbRoot.setPassword ( root_passwd );

    if ( dbRoot.open () )
    {
        QString str_query ( QString ( QStringLiteral ( "CREATE USER %1@%2 IDENTIFIED BY '%3'" ) ).arg ( USER_NAME, HOST, PASSWORD ) );
        QSqlQuery rootQuery ( dbRoot );
        rootQuery.exec ( str_query );
        str_query = QString ( QStringLiteral ( "GRANT ALL ON %1.* TO '%2'@'%3';" ) ).arg ( DB_NAME, USER_NAME, HOST );
        m_ok = rootQuery.exec ( str_query );
        rootQuery.finish ();
        dbRoot.commit ();
        dbRoot.close ();
        rootQuery.clear ();
    }
    QSqlDatabase::removeDatabase ( ROOT_CONNECTION );

	if ( !m_ok )
		vmNotify::notifyMessage ( nullptr, APP_TR_FUNC ( "Database error" ),
								APP_TR_FUNC ( "MYSQL could create the user for the application's database." ), 5000, true );
	return m_ok;
}

bool VivenciaDB::createAllTables ()
{
	database ()->close ();
	if ( openDataBase () )
	{
		uint i ( 0 );
		do
		{
			createTable ( tableInfo ( i ) );
		} while ( ++i < TABLES_IN_DB );
		mNewDB = true;
		return true;
	}
	return false;
}

bool VivenciaDB::createTable ( const TABLE_INFO* t_info )
{
	if ( tableExists ( t_info, this ) )
		return true;

	QString cmd ( QLatin1String ( "CREATE TABLE " ) + t_info->table_name + CHR_L_PARENTHESIS );
	int sep_1[2] = { 0 };
	int sep_2[2] = { 0 };

	QString values;
	while ( ( sep_2[0] = t_info->field_names.indexOf ( CHR_PIPE, sep_1[0] ) ) != -1 )
    {
#ifdef USING_QT6
        values.append ( t_info->field_names.mid ( sep_1[0], sep_2[0] - sep_1[0] ) );
#else
        values.append ( t_info->field_names.midRef ( sep_1[0], sep_2[0] - sep_1[0] ) );
#endif
		sep_1[0] = sep_2[0] + 1;

		sep_2[1] = t_info->field_flags.indexOf ( CHR_PIPE, sep_1[1] );
#ifdef USING_QT6
        values.append ( t_info->field_flags.mid ( sep_1[1], sep_2[1]-sep_1[1] ) );
#else
        values.append ( t_info->field_flags.midRef ( sep_1[1], sep_2[1]-sep_1[1] ) );
#endif
		sep_1[1] = sep_2[1] + 1;
	}
	values.append ( t_info->primary_key );
	cmd += values + CHR_R_PARENTHESIS + CHR_SPACE + t_info->table_flags;

	database ()->exec ( cmd );
	return ( database ()->lastError ().type () == QSqlError::NoError );
}

bool VivenciaDB::insertColumn ( const uint column, const TABLE_INFO* __restrict t_info )
{
	const QString cmd ( QLatin1String ( "ALTER TABLE " ) + t_info->table_name +
						QLatin1String ( " ADD COLUMN " ) + getTableColumnName ( t_info, column ) +
						getTableColumnFlags ( t_info, column ) + ( column > 0 ?
								QString ( QLatin1String ( " AFTER " ) + getTableColumnName ( t_info, column - 1 ) ) :
								QLatin1String ( " FIRST" ) )
					  );
	QSqlQuery query ( cmd, *database () );

	query.exec ();
	if ( !database ()->lastError ().isValid () )
	{
		mBackupSynced = false;
		return true;
	}
	MSG_OUT(database ()->lastError ().text ())
	return false;
}

bool VivenciaDB::removeColumn ( const QString& column_name, const TABLE_INFO* t_info )
{
	const QString cmd ( QLatin1String ( "ALTER TABLE " ) + t_info->table_name +
						QLatin1String ( " DROP COLUMN " ) + column_name );
	QSqlQuery query ( cmd, *database () );

	query.exec ();
	if ( !database ()->lastError ().isValid () )
	{
		mBackupSynced = false;
		return true;
	}
	return false;
}

bool VivenciaDB::renameColumn ( const QString& old_column_name, const uint col_idx, const TABLE_INFO* __restrict t_info )
{
	const QString cmd ( QLatin1String ( "ALTER TABLE " ) + t_info->table_name +
						QLatin1String ( " CHANGE " ) + old_column_name +
						getTableColumnName ( t_info, col_idx ) + CHR_SPACE + getTableColumnFlags ( t_info, col_idx ) );
	QSqlQuery query ( cmd, *database () );

	query.exec ();
	if ( !database ()->lastError ().isValid () )
	{
		mBackupSynced = false;
		return true;
	}
	return false;
}

bool VivenciaDB::changeColumnProperties ( const uint column, const TABLE_INFO* __restrict t_info )
{
	const QString cmd ( QLatin1String ( "ALTER TABLE " ) + t_info->table_name +
						QLatin1String ( " MODIFY COLUMN " ) + getTableColumnName ( t_info, column ) +
						getTableColumnFlags ( t_info, column ) );
	QSqlQuery query ( cmd, *database () );

	query.exec ();
	if ( !database ()->lastError ().isValid () )
	{
		mBackupSynced = false;
		return true;
	}
	return false;
}

void VivenciaDB::populateTableWidget ( const TABLE_INFO* t_info, vmTableWidget* table ) const
{
	const QString cmd ( QStringLiteral ( "SELECT * FROM " ) + t_info->table_name );

	QSqlQuery query ( *database () );
	query.setForwardOnly ( true );
	query.exec ( cmd );
	if ( !query.first () ) return;

	uint row ( 0 );
	do
	{
		for ( uint col ( 0 ); col < t_info->field_count; col++)
			table->sheetItem ( row, col )->setText ( query.value ( static_cast<int>( col ) ).toString (), false, false );
		if ( static_cast<int>( ++row ) >= table->totalsRow () )
			table->appendRow ();
	} while ( query.next () );
}

void VivenciaDB::updateTableWidget ( const TABLE_INFO* t_info, vmTableWidget* table, const QString& cmd, uint startrow )
{
	QSqlQuery query ( *database () );
	query.setForwardOnly ( true );
	query.exec ( cmd );
	if ( !query.first () ) return;

	if ( static_cast<int>( startrow ) >= table->totalsRow () )
		table->appendRow ();

	do
	{
		for ( uint col ( 0 ); col < t_info->field_count; col++)
			table->sheetItem ( startrow, col )->setText ( query.value ( static_cast<int>( col ) ).toString (), false, false );
		if ( static_cast<int>( ++startrow ) >= table->totalsRow () )
			table->appendRow ();
	} while ( query.next () );
}

void VivenciaDB::updateTableWidget ( const TABLE_INFO* t_info, vmTableWidget* table, const bool b_only_new )
{
	QString cmd;
	if ( b_only_new )
		cmd = QStringLiteral ( "SELECT * FROM " ) + t_info->table_name +
				QStringLiteral ( "WHERE ID>" ) + table->sheetItem ( static_cast<uint>( table->lastUsedRow () ), 0 )->text ();
	else
		cmd = QStringLiteral ( "SELECT * FROM " ) + t_info->table_name;
	updateTableWidget ( t_info, table, cmd, b_only_new ? static_cast<uint>( table->lastUsedRow () ) + 1 : 0 );
}

void VivenciaDB::updateTableWidget ( const TABLE_INFO* t_info, vmTableWidget* table, const uint id )
{
	updateTableWidget ( t_info, table,
				  QStringLiteral ( "SELECT * FROM " ) + t_info->table_name + QStringLiteral ( "WHERE ID=" ) + QString::number ( id ),
				  /*Tables should be in order. A casual out of order record is OK, but more must be investigated the reason. The -10 is extrapollating*/
				  static_cast<uint>( qMax ( static_cast<int>( id - 10 ), static_cast<int>( 0 ) ) ) );
}

void VivenciaDB::updateTableWidget ( const TABLE_INFO* t_info, vmTableWidget* table, const podList<uint>& ids )
{
	if ( !ids.isEmpty () )
	{
		QString id_string ( QString::number ( ids.first () ) );
		uint id ( 0 );
		do {
			id = ids.next ();
			if ( id > 0 )
				id_string += CHR_COMMA + QString::number ( id );
			else
				break;
		} while ( true );
		updateTableWidget ( t_info, table,
					  QStringLiteral ( "SELECT * FROM " ) + t_info->table_name + QStringLiteral ( "WHERE ID=" ) + id_string );
	}
}

bool VivenciaDB::deleteDB ( const QString& dbname, const VivenciaDB* vdb )
{
	QSqlQuery query ( vdb != nullptr ? *(vdb->database ()) : QSqlDatabase () );
	return runModifyingQuery ( QLatin1String ( "DROP DATABASE " ) + ( dbname.isEmpty () ? DB_NAME : dbname ), query );
}

bool VivenciaDB::optimizeTable ( const TABLE_INFO* __restrict t_info, const VivenciaDB* vdb )
{
	QSqlQuery query ( QLatin1String ( "OPTIMIZE LOCAL TABLE " ) + t_info->table_name, *(vdb->database ()) );
	return query.exec ();
}

bool VivenciaDB::lockTable ( const TABLE_INFO* __restrict t_info, const VivenciaDB* vdb )
{
	QSqlQuery query ( QLatin1String ( "LOCK TABLE " ) + t_info->table_name + QLatin1String ( "READ" ), *(vdb->database ()) );
	return query.exec ();
}

bool VivenciaDB::unlockTable ( const TABLE_INFO* __restrict t_info, const VivenciaDB* vdb )
{
	QSqlQuery query ( QLatin1String ( "UNLOCK TABLE " ) + t_info->table_name, *(vdb->database ()) );
	return query.exec ();
}

bool VivenciaDB::unlockAllTables ( const VivenciaDB* vdb )
{
	QSqlQuery query ( QStringLiteral ( "UNLOCK TABLES" ), *(vdb->database ()) );
	return query.exec ();
}

// After a call to this function, must needs call unlockAllTables
bool VivenciaDB::flushAllTables ( const VivenciaDB* vdb )
{
	QSqlQuery query ( QStringLiteral ( "FLUSH TABLES WITH READ LOCK" ), *(vdb->database ()) );
	return query.exec ();
}

bool VivenciaDB::deleteTable ( const QString& table_name, const VivenciaDB* vdb )
{
	QSqlQuery query ( QStringLiteral ( "DROP TABLE " ) + table_name, *(vdb->database ()) );
	return query.exec ();
}

bool VivenciaDB::clearTable ( const QString& table_name, const VivenciaDB* vdb )
{
	QSqlQuery query ( QStringLiteral ( "TRUNCATE TABLE " ) + table_name, *(vdb->database ()) );
	return query.exec ();
}

bool VivenciaDB::tableExists ( const TABLE_INFO* __restrict t_info, const VivenciaDB* vdb )
{
	QSqlQuery query ( QStringLiteral ( "SHOW TABLES LIKE '" ) + t_info->table_name + CHR_CHRMARK, *(vdb->database ()) );
	if ( query.exec () && query.first () )
		return query.value ( 0 ).toString () == t_info->table_name;
	return false;
}

uint VivenciaDB::recordCount ( const TABLE_INFO* __restrict t_info, const VivenciaDB* vdb )
{
	QSqlQuery query ( QStringLiteral ( "SELECT COUNT(*) FROM " ) + t_info->table_name, *(vdb->database ()) );
	if ( query.exec () && query.first () )
		return query.value ( 0 ).toUInt ();
	return 0;
}

bool VivenciaDB::databaseIsEmpty ( const VivenciaDB* vdb )
{
	QSqlQuery query ( QStringLiteral ( "SHOW TABLES FROM " ) + DB_NAME, *(vdb->database ()) );
	if ( query.exec () )
		return ( !query.isActive () || !query.next () );
	return true;
}

uint VivenciaDB::getHighestID ( const uint table, const VivenciaDB* vdb )
{
	if ( VivenciaDB::currentHighestID_list.at ( table ) == 0 )
	{
		QSqlQuery query ( vdb != nullptr ? *(vdb->database ()) : QSqlDatabase () );
		if ( runSelectLikeQuery ( QStringLiteral ( "SELECT MAX(ID) FROM " ) + tableInfo ( table )->table_name, query ) )
		{
			if ( recordCount ( tableInfo ( table ), vdb ) > 0 )
				VivenciaDB::currentHighestID_list[table] = query.value ( 0 ).toUInt ();
			else
				return 0;
		}
		else
			return 0;
	}
	return VivenciaDB::currentHighestID_list.at ( table );
}

uint VivenciaDB::getLowestID ( const uint table, const VivenciaDB* vdb )
{
	QSqlQuery query ( vdb != nullptr ? *(vdb->database ()) : QSqlDatabase () );
	if ( runSelectLikeQuery ( QStringLiteral ( "SELECT MIN(ID) FROM " ) + tableInfo ( table )->table_name, query ) )
	{
		if ( recordCount ( tableInfo ( table ), vdb ) > 0 )
			return query.value ( 0 ).toUInt ();
	}
	return 0;
}

uint VivenciaDB::getNextID ( const uint table, const VivenciaDB* vdb )
{
	if ( VivenciaDB::currentHighestID_list.at ( table ) == 0 )
		VivenciaDB::currentHighestID_list[table] = getHighestID ( table, vdb );
	return ++VivenciaDB::currentHighestID_list[table];
}

bool VivenciaDB::recordExists ( const QString& table_name, const int id, const VivenciaDB* vdb )
{
	if ( id >= 0 )
	{
		QSqlQuery query ( vdb != nullptr ? *(vdb->database ()) : QSqlDatabase () );
		return ( runSelectLikeQuery ( QStringLiteral ( "SELECT `ID` FROM %1 WHERE `ID`=%2" ).arg (
				 table_name, QString::number ( id ) ), query ) );
	}
	return false;
}
//-----------------------------------------CREATE-DESTROY-MODIFY-------------------------------------

//-----------------------------------------COMMOM-DBRECORD-------------------------------------------
bool VivenciaDB::insertRecord ( const DBRecord* db_rec ) const
{
	QString str_query ( QStringLiteral ( "INSERT INTO " ) + db_rec->t_info->table_name + QStringLiteral ( " VALUES ( " ) );
	QString values;

	const uint field_count ( db_rec->t_info->field_count );
	for ( uint i ( 0 ); i < field_count; ++i )
		values += CHR_CHRMARK + recStrValue ( db_rec, i ) + chrDelim2;
	values.chop ( 1 );
	
	str_query += values + QStringLiteral ( " )" );
	database ()->exec ( str_query );
	if ( database ()->lastError ().type () == QSqlError::NoError )
	{
		mBackupSynced = false;
		return true;
	}

	MSG_OUT(QStringLiteral ( "VivenciaDB::insertRecord - " ) << database ()->lastError ().text ())
	MSG_OUT(QStringLiteral ( "--------" ))
	MSG_OUT(QStringLiteral ( "Query command:" ))
	MSG_OUT(str_query)
	return false;
}

bool VivenciaDB::updateRecord ( const DBRecord* db_rec ) const
{
	if ( !db_rec || !db_rec->isModified () )
		return true;

	QString str_query ( QStringLiteral ( "UPDATE " ) +
						db_rec->t_info->table_name + QStringLiteral ( " SET " ) );

	int sep_1 ( ( db_rec->t_info->field_names.indexOf ( CHR_PIPE, 1 ) ) + 1 ); // skip ID field
	int sep_2 ( 0 );

	const uint field_count ( db_rec->t_info->field_count );
	const QString chrDelim ( QStringLiteral ( "=\'" ) );
	const QChar CHR_PIPE_PTR ( CHR_PIPE );
	const QStringMatcher sep_matcher ( &CHR_PIPE_PTR, 1 );

	for ( uint i ( 1 ); i < field_count; ++i )
	{
		sep_2 = sep_matcher.indexIn ( db_rec->t_info->field_names, sep_1 );
		if ( db_rec->isModified ( i ) )
		{
#ifdef USING_QT6
            str_query += db_rec->t_info->field_names.mid ( sep_1, sep_2 - sep_1 ) + chrDelim + recStrValue ( db_rec, i ) + chrDelim2;
#else
            str_query += db_rec->t_info->field_names.midRef ( sep_1, sep_2 - sep_1 ) + chrDelim + recStrValue ( db_rec, i ) + chrDelim2;
#endif
		}
		sep_1 = sep_2 + 1;
	}
	str_query.chop ( 1 );
	str_query += ( QStringLiteral ( " WHERE ID='" ) + db_rec->actualRecordStr ( 0 ) + CHR_CHRMARK ); // id ()

	database ()->exec ( str_query );

	if ( database ()->lastError ().type () == QSqlError::NoError )
	{
		mBackupSynced = false;
		return true;
	}
	MSG_OUT(QStringLiteral ( "VivenciaDB::updateRecord - " ) << database ()->lastError ().text ())
	return false;
}

bool VivenciaDB::removeRecord ( const DBRecord* db_rec ) const
{
	if ( db_rec->actualRecordInt ( 0 ) >= 0 )
	{
		const QString str_query ( QStringLiteral ( "DELETE FROM " ) + db_rec->t_info->table_name +
								  QStringLiteral ( " WHERE ID='" ) + db_rec->actualRecordStr ( 0 ) + CHR_CHRMARK );
		database ()->exec ( str_query );

		if ( database ()->lastError ().type () == QSqlError::NoError )
		{
			const auto id ( static_cast<uint>( db_rec->actualRecordInt ( 0 ) ) );
			const uint table ( db_rec->t_info->table_order );
			if ( id >= getHighestID ( table, this ) )
				VivenciaDB::currentHighestID_list[table] = id - 1;

			mBackupSynced = false;
			return true;
		}
		MSG_OUT(QStringLiteral ( "VivenciaDB::removeRecord - " ) << database ()->lastError ().text ())
	}
	return false;
}

void VivenciaDB::loadDBRecord ( DBRecord* db_rec, const QSqlQuery* const query )
{
	for ( uint i ( 0 ); i < db_rec->t_info->field_count; ++i )
	{
		setRecValue ( db_rec, i, query->value ( static_cast<int>( i ) ).toString () );
		if ( db_rec->fieldType ( i ) == DBTYPE_ID )
			db_rec->setIntValue ( i, query->value ( static_cast<int>( i ) ).toInt () );
	}
}

bool VivenciaDB::insertDBRecord ( DBRecord* db_rec )
{
	const uint table_order ( db_rec->t_info->table_order );
	const bool bOutOfOrder ( db_rec->recordInt ( 0 ) >= 1 );
	const uint new_id ( bOutOfOrder ? static_cast<uint>( db_rec->recordInt ( 0 ) ) : getNextID ( table_order, this ) );

	db_rec->setIntValue ( 0, static_cast<int>( new_id ) );
	db_rec->setValue ( 0, QString::number ( new_id ) );
	const bool ret ( insertRecord ( db_rec ) );
	if ( ret )
		VivenciaDB::currentHighestID_list[table_order] = new_id;
	return ret;
}

bool VivenciaDB::getDBRecord ( DBRecord* db_rec, const uint field, const bool load_data )
{
	/* I've met with a few instances when the program would crash without a seemingly good reason and when
	 * the mysql server was being updated or stopped for some reason. When those happend, the program's execution pointer
	 * was here, execing the query.
	 */
	try
	{
		const QString cmd ( QStringLiteral ( "SELECT * FROM " ) +
						db_rec->t_info->table_name + QStringLiteral ( " WHERE " ) +
						getTableColumnName ( db_rec->t_info, field ) + CHR_EQUAL +
						db_rec->backupRecordStr ( field )
					  );

		QSqlQuery query ( *database () );
		query.setForwardOnly ( true );
		query.exec ( cmd );
		if ( query.first () )
		{
			if ( load_data )
				loadDBRecord ( db_rec, &query );
			else
			{
				/* For the SELECT statement have some use, at least save the read id for later use.
				 * Save all the initial id fields. Fast table lookup and fast item lookup, that's
				 * the goal of this part of the code
				*/

				uint i ( 0 ); // save first id fields. Useful for subsequent queries or finding related vmListItems
				do
				{
					if ( db_rec->fieldType ( i ) == DBTYPE_ID )
						db_rec->setIntValue ( i, query.value ( static_cast<int>( i ) ).toInt () );
					else
						break;
				} while ( ++i < db_rec->fieldCount () );
				setRecValue ( db_rec, 0, query.value ( 0 ).toString () );
			}
			return true;
		}
	}
	catch ( const std::runtime_error& e )
	{
		MSG_OUT("A runtime exception was caught with message: \"" << e.what () << "\"")
		if ( m_errorHandling_func )
			m_errorHandling_func ( e.what () );
	}
	catch ( const std::exception& e )
	{
		MSG_OUT("A standard exception was caught with message: \"" << e.what () << "\"")
		if ( m_errorHandling_func )
			m_errorHandling_func ( e.what () );
	}
	
	return false;
}

bool VivenciaDB::getDBRecord ( DBRecord* db_rec, DBRecord::st_Query& stquery, const bool load_data )
{
	/* I've met with a few instances when the program would crash without a seemingly good reason and when
	 * the mysql server was being updated or stopped for some reason. When those happend, the program's execution pointer
	 * was here, execing the query.
	 */
	try
	{
		bool ret ( false );
		stquery.query = new QSqlQuery ( *database () );
		if ( !stquery.customquery )
		{
			const QString cmd ( QStringLiteral ( "SELECT * FROM " ) + db_rec->t_info->table_name +
							( QStringLiteral ( " WHERE " ) +
							  getTableColumnName ( db_rec->t_info, static_cast<uint>(stquery.field) ) + CHR_EQUAL +
							  CHR_QUOTES + stquery.search + CHR_QUOTES )
						  );
			ret = stquery.query->exec ( cmd );
		}
		else
		{
			ret = stquery.query->exec ( stquery.str_query );
		}
		if ( ret )
			ret = stquery.query->first ();

		if ( ret )
		{
			if ( load_data )
				loadDBRecord ( db_rec, stquery.query );
			else
			{
				uint i ( 0 ); // save first id fields. Useful for subsequent queries or finding related vmListItems
				do
				{
					if ( db_rec->fieldType ( i ) == DBTYPE_ID )
						db_rec->setIntValue ( i, stquery.query->value ( static_cast<int>( i ) ).toInt () );
					else
						break;
				} while ( ++i < db_rec->fieldCount () );
				setRecValue ( db_rec, 0, stquery.query->value ( 0 ).toString () );
			}
			return true;
		}
	}
	catch ( const std::runtime_error& e )
	{
		MSG_OUT("A runtime exception was caught with message: \"" << e.what () << "\"")
		if ( m_errorHandling_func )
			m_errorHandling_func ( e.what () );
	}
	catch ( const std::exception& e )
	{
		MSG_OUT("A standard exception was caught with message: \"" << e.what () << "\"")
		if ( m_errorHandling_func )
			m_errorHandling_func ( e.what () );
	}
	
	return false;
}

bool VivenciaDB::runSelectLikeQuery ( const QString& query_cmd, QSqlQuery& queryRes )
{
	if ( !query_cmd.isEmpty () )
	{
		try {
			queryRes.setForwardOnly ( true );
			if ( queryRes.exec ( query_cmd ) )
			{
				//DBG_OUT ( "Query executed successfully:", query_cmd, false, true )
				return queryRes.first ();
			}
			//DBG_OUT ( "Query execution failed:", query_cmd, false, true )
		}
		catch ( const std::runtime_error& e )
		{
			MSG_OUT("A runtime exception was caught with message: \"" << e.what () << "\"")
		}
		catch ( const std::exception& e )
		{
			MSG_OUT("A standard exception was caught with message: \"" << e.what () << "\"")
		}
	}
	return false;	
}

bool VivenciaDB::runModifyingQuery ( const QString& query_cmd, QSqlQuery& queryRes )
{
	if ( !query_cmd.isEmpty () )
	{
		try {
			queryRes.setForwardOnly ( true );
			if ( queryRes.exec ( query_cmd ) )
				return !queryRes.lastError ().isValid (); // other types of query do not yield browseable results
		}
		catch ( const std::runtime_error& e )
		{
			MSG_OUT("A runtime exception was caught with message: \"" << e.what () << "\"")
		}
		catch ( const std::exception& e )
		{
			MSG_OUT("A standard exception was caught with message: \"" << e.what () << "\"")
		}
	}
	return false;	
}
//-----------------------------------------COMMOM-DBRECORD-------------------------------------------

//-----------------------------------------IMPORT-EXPORT--------------------------------------------
bool VivenciaDB::doBackup ( const QString& filepath, const QString& tables )
{
	flushAllTables ( this );
	if ( m_progressMaxSteps_func )
		m_progressMaxSteps_func ( 3 );
	m_progressStepUp_func ();
	QStringList mysqldump_args;
	mysqldump_args <<
				QStringLiteral ( "--user=") + USER_NAME <<
				QStringLiteral ( "--password=" ) + PASSWORD <<
				QStringLiteral ( "--opt" ) <<
				QStringLiteral ( "--skip-set-charset" ) <<
				QStringLiteral ( "--default-character-set=utf8mb4" ) <<
				QStringLiteral ( "--skip-extended-insert" ) <<
				QStringLiteral ( "--single-transaction" ) <<
				QStringLiteral ( "--databases" ) <<
				DB_NAME;

	mysqldump_args.append ( tables.split ( CHR_SPACE ) );
	mBackupSynced = fileOps::executeWithFeedFile ( mysqldump_args, backupApp (), filepath, false );
	unlockAllTables ( this );
	m_progressStepUp_func ();
	return mBackupSynced;
}

/* Returns true if dumpfile is of the same version the program supports
   Returns false if there is any mismatch
   In the future, I might do a per table analysis and use mysqlimport
 */
bool VivenciaDB::checkDumpFile ( const QString& dumpfile )
{
	bool ok ( false );
	if ( m_ok )
	{
		QFile file ( dumpfile );
		if ( file.open ( QIODevice::ReadOnly|QIODevice::Text ) )
		{
			auto* __restrict line_buf ( new char [8*1024] );// OPT-6
			qint64 n_chars ( -1 );
			QString line, searchedExpr;
			const QString expr ( QStringLiteral ( "(%1,%2,%3," ) );

			do
			{
				n_chars = file.readLine ( line_buf, 8*1024 );
				if ( n_chars >= 500 )
				{ // The first time ths function was created, the searched line had 509 characters
					line = QString::fromUtf8 ( line_buf, static_cast<int>(strlen ( line_buf ) - 1) );
					if ( line.contains ( QStringLiteral ( "INSERT INTO `GENERAL`" ) ) )
					{
						ok = true; // so far, it is
						for ( const TABLE_INFO* ti : table_info )
						//for ( uint i ( 0 ); i < TABLES_IN_DB; ++i )
						{
							searchedExpr = expr.arg ( ti->table_order ).arg ( ti->table_name ).arg ( ti->version );
							if ( !line.contains ( searchedExpr ) ) // one single mismatch the dump file cannot be used by mysqlimport
							{
								ok = false;
								break;
							}
						}
					}
				}
			} while ( n_chars != -1 );
			delete [] line_buf;
			file.close ();
		}
	}
	return ok;
}

bool VivenciaDB::doRestore ( const QString& filepath )
{
	QString uncompressed_file ( QStringLiteral ( "/tmp/vmdb_tempfile" ) );
	const bool bSourceCompressed ( VMCompress::isCompressed ( filepath ) );

	if ( bSourceCompressed )
	{
		fileOps::removeFile ( uncompressed_file );
		VMCompress::decompress ( filepath, uncompressed_file );
	}
	else
		uncompressed_file = filepath;

	if ( m_progressMaxSteps_func )
		m_progressMaxSteps_func ( 3 );
	m_progressStepUp_func ();

	QStringList restoreapp_args;
	restoreapp_args <<
					   QStringLiteral ( "--user=") + USER_NAME <<
					   QStringLiteral ( "--password=" ) + PASSWORD <<
					   QStringLiteral ( "--default_character_set=utf8mb4" ) <<
					   QStringLiteral ( "--database" ) << DB_NAME;
	bool ret ( true );

	//hopefully, by now, Data will have intermediated well the situation, having all the pieces of the program
	// aware of the changes that might occur.
	database ()->close ();

	// use a previous dump from mysqldump to create a new database.
	// Note that DB_NAME must refer to an unexisting database or this will fail
	// Also I think the dump must be of a complete database, with all tables, to avoid inconsistencies
	if ( !checkDumpFile ( uncompressed_file ) )
	{
		// some table version mismatch
		ret = fileOps::executeWithFeedFile ( restoreapp_args, importApp (), uncompressed_file, true, true );
	}
	// use a previous dump from mysqldump to update or replace one or more tables in an existing database
	// Tables will be overwritten or created. Note that DB_NAME must refer to an existing database
	else
	{
		restoreapp_args.append ( uncompressed_file );
		ret = fileOps::executeWait ( restoreapp_args, restoreApp (), true );
	}
	// Repair the tables for any problems that we have faced or would face

	QStringList repairapp_args;
	repairapp_args <<
					QStringLiteral ( "--user=") + USER_NAME <<
					QStringLiteral ( "--password=" ) + PASSWORD <<
					QStringLiteral ( "--auto-repair" ) <<
					QStringLiteral ( "--optimize" ) <<
					QStringLiteral ( "--databases" ) << DB_NAME;
	fileOps::executeWait ( repairapp_args, QStringLiteral ( "mysqlcheck" ) );

	m_progressStepUp_func ();

	if ( bSourceCompressed ) // do not leave trash behind
		fileOps::removeFile ( uncompressed_file );

	m_progressStepUp_func ();
	if ( ret )
	{
		mBackupSynced = true;
		return openDataBase ();
	}
	return false;
}

bool VivenciaDB::importFromCSV ( const QString& filename )
{
	auto tdb ( new dataFile ( filename ) );
	if ( !tdb->load ().isOn () )
		return false;
	tdb->setRecordSeparationChar ( CHR_NEWLINE );

	if ( m_progressMaxSteps_func )
		m_progressMaxSteps_func ( 3 );
	m_progressStepUp_func ();

	stringRecord rec;
	rec.setFieldSeparationChar ( CHR_SEMICOLON );
	if ( !tdb->getRecord ( rec, 0 ) )
		return false;

	const int table_id  ( rec.fieldValue ( 1 ).toInt () );
	DBRecord* db_rec ( nullptr );
	switch ( table_id )
	{
		case GENERAL_TABLE:
			db_rec = new generalTable;
		break;
		case USERS_TABLE:
			db_rec = new userRecord;
		break;
		case CLIENT_TABLE:
			db_rec = new Client;
		break;
		case JOB_TABLE:
			db_rec = new Job;
		break;
		case PAYMENT_TABLE:
			db_rec = new Payment;
		break;
		case PURCHASE_TABLE:
			db_rec = new Buy;
		break;
		case QUICK_PROJECT_TABLE:
			db_rec = new quickProject;
		break;
		case INVENTORY_TABLE:
			db_rec = new Inventory;
		break;
		case SUPPLIER_TABLE:
			db_rec = new supplierRecord;
		break;
		case COMPANY_PURCHASES_TABLE:
			db_rec = new companyPurchases;
		break;
		case SUPPLIES_TABLE:
			db_rec = new dbSupplies;
		break;
		case COMPLETER_RECORDS_TABLE:
			db_rec = new completerRecord;
		break;
	}

	m_progressStepUp_func ();
	bool bSuccess ( false );
	//we don't import if there is a version mismatch
	if ( rec.fieldValue ( 0 ).isEmpty () )
         return false;
    if ( rec.fieldValue ( 0 ).at ( 0 ).toLatin1 () == db_rec->t_info->version )
	{
		uint fld ( 1 );
		int idx ( 1 );
		do
		{
			if ( !tdb->getRecord ( rec, idx ) )
				break;
			if ( rec.first () )
			{
				setRecValue ( db_rec, 0, rec.curValue () );
				do
				{
					if ( rec.next () )
						setRecValue ( db_rec, fld++, rec.curValue () );
					else
						break;
				} while ( true );
			}
			db_rec->saveRecord ();
			++idx;
		} while ( true );
		if ( idx >= 2 )
			bSuccess = true;
		delete db_rec;
	}

	m_progressStepUp_func ();
	delete tdb;
	return bSuccess;
}

bool VivenciaDB::exportToCSV ( const uint table, const QString& filename )
{
	static_cast<void>(fileOps::removeFile ( filename ));
	auto* __restrict tdb ( new dataFile ( filename ) );
	if ( tdb->load ().isOff () )
		return false;

	const TABLE_INFO* t_info ( nullptr );
	switch ( table )
	{
		case GENERAL_TABLE:
			t_info = &generalTable::t_info;
		break;
		case USERS_TABLE:
			t_info = &userRecord::t_info;
		break;
		case CLIENT_TABLE:
			t_info = &Client::t_info;
		break;
		case JOB_TABLE:
			t_info = &Job::t_info;
		break;
		case PAYMENT_TABLE:
			t_info = &Payment::t_info;
		break;
		case PURCHASE_TABLE:
			t_info = &Buy::t_info;
		break;
		case QUICK_PROJECT_TABLE:
			t_info = &quickProject::t_info;
		break;
		case INVENTORY_TABLE:
			t_info = &Inventory::t_info;
		break;
		case SUPPLIER_TABLE:
			t_info = &supplierRecord::t_info;
		break;
		case COMPANY_PURCHASES_TABLE:
			t_info = &companyPurchases::t_info;
		break;
		case SUPPLIES_TABLE:
			t_info = &dbSupplies::t_info;
		break;
		case COMPLETER_RECORDS_TABLE:
			t_info = &completerRecord::t_info;
		break;
	}

	if ( m_progressMaxSteps_func )
		m_progressMaxSteps_func ( 4 );

	tdb->setRecordSeparationChar ( CHR_NEWLINE, CHR_PIPE );
	m_progressStepUp_func ();

	stringRecord rec;
	rec.setFieldSeparationChar ( CHR_PIPE );
	rec.fastAppendValue ( QString::number ( t_info->table_id ) ); //Record table_id information
	rec.fastAppendValue ( QString::number ( t_info->version, 'f', 1 ) ); //Record version information
	tdb->insertRecord ( 0, rec );
	m_progressStepUp_func ();

	const QString cmd ( QStringLiteral ( "SELECT * FROM " ) + t_info->table_name );

	QSqlQuery query ( *database () );
	query.setForwardOnly ( true );
	query.exec ( cmd );

	if ( query.first () )
	{
		uint pos ( 1 ), i ( 0 );
		do
		{
			rec.clear ();
			for ( i = 0; i < t_info->field_count; ++i )
				rec.fastAppendValue ( query.value ( static_cast<int>(i) ).toString () );
			tdb->insertRecord ( static_cast<int>(pos), rec );
			++pos;
		} while ( query.next () );
	}

	m_progressStepUp_func ();

	const uint n ( tdb->recCount () );
	tdb->commit ();
	delete tdb;

	m_progressStepUp_func ();
	return ( n > 0 );
}
//-----------------------------------------IMPORT-EXPORT--------------------------------------------

