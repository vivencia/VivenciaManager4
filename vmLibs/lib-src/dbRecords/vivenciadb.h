#ifndef VIVENCIADB_H
#define VIVENCIADB_H

#include "db_defs.h"
#include "vmlibs.h"
#include "dbrecord.h"
#include "vmlist.h"

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include <functional>

enum DB_ERROR_CODES
{
	NO_ERR = 0,
	ERR_DATABASE_PROBLEM = 1,
	ERR_MYSQL_NOT_FOUND = 2,
	ERR_USER_NOT_ADDED_TO_MYSQL_GROUP = 3,
	ERR_SETUP_FILES_MISSING = 4,
	ERR_COMMAND_MYSQL = 5,
	ERR_NO_DB = 6,
	ERR_DB_EMPTY = 7
};

static const QString MYSQL_INIT_SCRIPT ( QStringLiteral ( "/etc/init.d/mysql" ) );

class vmTableWidget;

class VivenciaDB
{

friend class DBRecord;
friend class fixDatabase;

public:
	explicit VivenciaDB ( const bool b_set_dbrec_mngr = false );
	~VivenciaDB ();

	//-----------------------------------------STATIC---------------------------------------------
	static QString getTableColumnName ( const TABLE_INFO* t_info, uint column );
	static QString getTableColumnFlags ( const TABLE_INFO* t_info, uint column );
	static QString getTableColumnLabel ( const TABLE_INFO* t_info, uint column );
	static int getTableColumnIndex ( const TABLE_INFO* t_info, const QString& column_name );
	static uint tableIDFromTableName ( const QString& strTableName );
	static QString tableName ( const TABLE_ORDER table );

	static inline const TABLE_INFO* tableInfo ( const uint to ) //to = table order
	{
		return table_info[to];
	}
	
	static inline QString backupApp () { return QStringLiteral ( "mysqldump" ); }
	static inline QString importApp () { return STR_MYSQL; }
	static inline QString restoreApp () { return QStringLiteral ( "mysqlimport" ); }
	static inline QString adminApp () { return QStringLiteral ( "mysqladmin" ); }
	//-----------------------------------------STATIC---------------------------------------------
	
	inline QSqlDatabase* database () const
	{
		return const_cast<VivenciaDB*>( this )->m_db;
	}

	inline bool db_oK ()  const { return m_ok; }
	inline bool backUpSynced () const { return mBackupSynced; }

	//-----------------------------------------MYSQL-------------------------------------------
	static bool isMySQLRunning ();
	static DB_ERROR_CODES commandMySQLServer ( const QString& command, const bool b_do_not_exec = false );
	static DB_ERROR_CODES checkDatabase ( VivenciaDB* vdb );
	//-----------------------------------------MYSQL-------------------------------------------

	bool openDataBase ( const QString& connecion_name = QString () );
	bool restart ();
	void doPreliminaryWork ();
	bool createDatabase ( const bool bCreateTables );
	bool checkRootPasswordForMySQL ( QString& password );
	bool changeRootPassword ( const QString& oldpasswd, const QString& newpasswd );
	bool createUser ( const QString& root_passwd );
	bool createAllTables ();
	bool createTable ( const TABLE_INFO* t_info );
	bool insertColumn ( const uint column, const TABLE_INFO* t_info );
	bool removeColumn ( const QString& column_name, const TABLE_INFO* t_info );
	bool renameColumn ( const QString& old_column_name, const uint col_idx, const TABLE_INFO* t_info );
	bool changeColumnProperties ( const uint column, const TABLE_INFO* t_info );
	void populateTable ( const TABLE_INFO* t_info, vmTableWidget* table ) const;
	void updateTable ( const TABLE_INFO* t_info, vmTableWidget* table, const bool b_only_new ) const;
	void updateTable ( const TABLE_INFO* t_info, vmTableWidget* table, const uint id ) const;
	void updateTable ( const TABLE_INFO* t_info, vmTableWidget* table, const podList<uint>& ids ) const;

	static bool deleteDB ( const QString& dbname = QString () );
	static bool optimizeTable ( const TABLE_INFO* t_info );
	static bool lockTable ( const TABLE_INFO* t_info );
	static bool unlockTable ( const TABLE_INFO* t_info );
	static bool unlockAllTables ();
	static bool flushAllTables ();
	static bool deleteTable ( const QString& table_name );
	static inline bool deleteTable ( const TABLE_INFO* t_info ) { return deleteTable ( t_info->table_name ); }
	static bool clearTable ( const QString& table_name );
	static inline bool clearTable ( const TABLE_INFO* t_info ) { return clearTable ( t_info->table_name ); }
	static bool tableExists ( const TABLE_INFO* t_info );
	static uint recordCount ( const TABLE_INFO* t_info );
	static bool databaseIsEmpty ();
	static uint getHighestID ( const uint table );
	static uint getLowestID ( const uint table );
	static uint getNextID ( const uint table );
	static bool recordExists ( const QString& table_name, const int id );

	//-----------------------------------------COMMOM-DBRECORD-------------------------------------------
	bool insertRecord ( const DBRecord* db_rec ) const;
	bool updateRecord ( const DBRecord* db_rec ) const;
	bool removeRecord ( const DBRecord* db_rec ) const;
	void loadDBRecord ( DBRecord* db_rec, const QSqlQuery* const query );
	bool insertDBRecord ( DBRecord* db_rec );
	bool getDBRecord ( DBRecord* db_rec, const uint field = 0, const bool load_data = true );
	bool getDBRecord ( DBRecord* db_rec, DBRecord::st_Query& stquery, const bool load_data = true );

	static bool runSelectLikeQuery ( const QString& query_cmd, QSqlQuery& queryRes );
	static bool runModifyingQuery ( const QString& query_cmd, QSqlQuery& queryRes );
	//-----------------------------------------COMMOM-DBRECORD-------------------------------------------

	//-----------------------------------------IMPORT-EXPORT--------------------------------------------
	bool checkDumpFile ( const QString& dumpfile );
	bool doBackup ( const QString& filepath, const QString& tables );
	bool doRestore ( const QString& filepath );
	bool importFromCSV ( const QString& filename );
	bool exportToCSV ( const uint table, const QString& filename );
	
	//-----------------------------------------IMPORT-EXPORT--------------------------------------------

	//-----------------------------------------OUTSIDE-INTERACTION--------------------------------------------
	void setCallbackForProgressMaxSteps ( const std::function<void ( const uint max_steps )>& func ) {
		m_progressMaxSteps_func = func;
	}
	void setCallbackForSteppingUpProgress ( const std::function<void ()>& func ) {
		m_progressStepUp_func = func;
	}
	void setCallbackForErrorHanddling ( const std::function<void ( const char* err_msg )>& func ) {
		m_errorHandling_func = func;
	}
	//-----------------------------------------OUTSIDE-INTERACTION--------------------------------------------

private:
	QSqlDatabase* m_db;

	static const TABLE_INFO* const table_info[TABLES_IN_DB];

	bool m_ok, mNewDB;
	mutable bool mBackupSynced;

	static podList<uint> currentHighestID_list;

	std::function<void ( const uint max_steps )> m_progressMaxSteps_func;
	std::function<void ()> m_progressStepUp_func;
	std::function<void ( const char* err_msg )> m_errorHandling_func;
};

#undef USE_THREADS
#include <QtCore/QObject>

#ifdef USE_THREADS
class threadedDBOps : public QObject
{
Q_OBJECT

public:
	virtual ~threadedDBOps ();
	
#else
class threadedDBOps
{

public:
	~threadedDBOps () = default;
#endif
	
	explicit threadedDBOps ( VivenciaDB* vdb );
	void populateTable ( const TABLE_INFO* t_info, vmTableWidget* table );
	void updateTable ( const TABLE_INFO* t_info, vmTableWidget* table, const QString& cmd, uint startrow = 0 );
	void updateTable ( const TABLE_INFO* t_info, vmTableWidget* table, const bool b_only_new );
	void updateTable ( const TABLE_INFO* t_info, vmTableWidget* table, const uint id );
	void updateTable ( const TABLE_INFO* t_info, vmTableWidget* table, const podList<uint>& ids );
	inline void setCallbackForFinished ( const std::function<void ()>& func ) { m_finishedFunc = func; }
	
protected:
	VivenciaDB* m_vdb;
	std::function<void ()> m_finishedFunc;
};
#endif // VIVENCIADB_H
