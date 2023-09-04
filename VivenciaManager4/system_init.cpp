#include "system_init.h"
#include "global.h"
#include "mainwindow.h"
#include "backupdialog.h"

#include <heapmanager.h>
#include <dbRecords/vivenciadb.h>
#include <dbRecords/completers.h>
#include <dbRecords/dbtablewidget.h>
#include <vmUtils/configops.h>
#include <vmUtils/fileops.h>
#include <vmNotify/vmnotify.h>
#include <vmDocumentEditor/documenteditor.h>

#include <QtWidgets/QFileDialog>
#include <QApplication>

extern "C"
{
	#include <unistd.h>
}

bool Sys_Init::EXITING_PROGRAM ( false );
QString Sys_Init::APP_START_CMD;

MainWindow* Sys_Init::mainwindow_instance ( nullptr );
configOps* Sys_Init::config_instance ( nullptr );
VivenciaDB* Sys_Init::vdb_instance ( nullptr );
vmCompleters* Sys_Init::completers_instance ( nullptr );
vmNotify* Sys_Init::notify_instance ( nullptr );
documentEditor* Sys_Init::editor_instance ( nullptr );
companyPurchasesUI* Sys_Init::cp_instance ( nullptr );
suppliersDlg* Sys_Init::sup_instance ( nullptr );
estimateDlg* Sys_Init::estimates_instance ( nullptr );
simpleCalculator* Sys_Init::calc_instance ( nullptr );
configDialog* Sys_Init::configdlg_instance ( nullptr );
spreadSheetEditor* Sys_Init::qp_instance ( nullptr );
BackupDialog* Sys_Init::backup_instance ( nullptr );
restoreManager* Sys_Init::restore_instance ( nullptr );

void deleteInstances ()
{
	heap_del ( Sys_Init::editor_instance );
	heap_del ( Sys_Init::configdlg_instance );
	heap_del ( Sys_Init::cp_instance );
	heap_del ( Sys_Init::sup_instance );
	heap_del ( Sys_Init::estimates_instance );
	heap_del ( Sys_Init::qp_instance );
	heap_del ( Sys_Init::calc_instance );
	heap_del ( Sys_Init::completers_instance );
	heap_del ( Sys_Init::backup_instance );
	heap_del ( Sys_Init::restore_instance );
	heap_del ( Sys_Init::vdb_instance );
	heap_del ( Sys_Init::mainwindow_instance );
	heap_del ( Sys_Init::config_instance );
	heap_del ( Sys_Init::notify_instance );
}
//--------------------------------------STATIC-HELPER-FUNCTIONS---------------------------------------------

void Sys_Init::restartProgram ()
{
	char* args[2] = { nullptr, nullptr };
	args[0] = static_cast<char*> ( ::malloc ( static_cast<size_t>(APP_START_CMD.toUtf8 ().size ()) * sizeof (char) ) );
	::strcpy ( args[0], APP_START_CMD.toUtf8 ().constData () );
	::execv ( args[0], args );
	::free ( args[0] );
	qApp->exit ( 0 );
}

DB_ERROR_CODES Sys_Init::checkSystem ( const bool bFirstPass )
{
	if ( !fileOps::exists ( MYSQL_INIT_SCRIPT ).isOn () )
	{
		NOTIFY ()->criticalBox ( APP_TR_FUNC ( "MYSQL is not installed - Exiting" ),
			APP_TR_FUNC ( "Could not find mysql init script at " ) + MYSQL_INIT_SCRIPT +
			APP_TR_FUNC ( ". Please check if mysql-client and mysql-server are installed." ) );
		return ERR_MYSQL_NOT_FOUND;
	}

	QStringList args ( fileOps::currentUser () );
	const QString groups ( fileOps::executeAndCaptureOutput ( args, QStringLiteral ( "groups" ) ) );
	bool ret ( groups.contains ( QRegExp ( QStringLiteral ( "mysql|root" ) ) ) );
	if ( !ret && bFirstPass )
	{
		static_cast<void>( fileOps::sysExec ( QStringLiteral ( "usermod -a -G " ) + QStringLiteral ( "mysql " ) + fileOps::currentUser () ) );
		ret = checkSystem ( false );
		if ( !ret )
			return ERR_USER_NOT_ADDED_TO_MYSQL_GROUP;
	}
	return NO_ERR;
}

DB_ERROR_CODES Sys_Init::checkLocalSetup ()
{
	if ( !fileOps::exists ( CONFIG ()->projectDocumentFile () ).isOn () )
	{
		const QString installedDir ( QFileDialog::getExistingDirectory ( nullptr,
									 APP_TR_FUNC ( "VivenciaManager needs to be setup. Choose the directory into which the download file was extracted." ),
									 QStringLiteral ( "~" ) )
								   );
		if ( installedDir.isEmpty () )
		{
			if ( NOTIFY ()->criticalBox ( APP_TR_FUNC ( "Setup files are missing" ),
										 APP_TR_FUNC ( "Choose, again, the directory where the extracted setup files are" ), 5000 ) )
				checkSetup ();
			else
				return ERR_SETUP_FILES_MISSING;
		}
		else
		{
			const QString dataDir ( CONFIG ()->appDataDir () ); // the first time, use the default data dir. Later, the user might change it
			fileOps::createDir ( dataDir );
			fileOps::copyFile ( dataDir, installedDir + CHR_F_SLASH + STR_PROJECT_DOCUMENT_FILE );
			fileOps::copyFile ( dataDir, installedDir + CHR_F_SLASH + STR_PROJECT_SPREAD_FILE );
			fileOps::copyFile ( dataDir, installedDir + CHR_F_SLASH + STR_VIVENCIA_LOGO );
		}
	}
	return NO_ERR;
}

void Sys_Init::checkSetup ()
{
	DB_ERROR_CODES err_code ( NO_ERR );

	err_code = checkSystem ();
	if ( err_code != NO_ERR )
		deInit ( err_code );

	err_code = checkLocalSetup ();
	if ( err_code != NO_ERR )
		deInit ( err_code );

	err_code = VivenciaDB::checkDatabase ( VDB () );

	switch ( err_code )
	{
		case NO_ERR: break;
		case ERR_NO_DB:
		case ERR_DB_EMPTY:
			err_code = static_cast<DB_ERROR_CODES>( BACKUP ()->showNoDatabaseOptionsWindow () );
			if ( err_code != NO_ERR )
				deInit ( err_code );
		break;
		case ERR_DATABASE_PROBLEM:
		case ERR_MYSQL_NOT_FOUND:
		case ERR_COMMAND_MYSQL:
		case ERR_USER_NOT_ADDED_TO_MYSQL_GROUP:
		case ERR_SETUP_FILES_MISSING:
			deInit ( err_code );
	}
}

//--------------------------------------STATIC-HELPER-FUNCTIONS---------------------------------------------
void Sys_Init::init ( const QString& cmd )
{
	APP_START_CMD = cmd;
	configOps::setAppName ( PROGRAM_NAME );

	config_instance = new configOps ( configOps::appConfigFile (), "MainWindowConfig" );
	vdb_instance = new VivenciaDB ( true );
	completers_instance = new vmCompleters;
	mainwindow_instance = new MainWindow;
	notify_instance = new vmNotify ( "BR", mainwindow_instance );

	//DBRecord::setDatabaseManager ( vdb_instance );
	DBRecord::setCompleterManager ( completers_instance );
	documentEditor::setCompleterManager ( completers_instance );
	dbTableWidget::setCompleterManager ( completers_instance );
	dbListItem::appStartingProcedures ();
	vmDateEdit::appStartingProcedures ();

	checkSetup ();
	VDB ()->doPreliminaryWork ();
	MAINWINDOW ()->continueStartUp ();
}

void Sys_Init::deInit ( int err_code )
{
	static bool already_called ( false );

	// the user clicked a button to exit the program. This function gets called and do the cleanup
	// job. Now, it asks Qt to exit the program and will be called again because it is hooked up to the
	// signal aboutToQuit (); only, we do not need to do anything this time, so we return.
	if ( already_called )
		return;
	already_called = true;
	
	EXITING_PROGRAM = true;
	dbListItem::appExitingProcedures ();
	vmDateEdit::appExitingProcedures ();
	BackupDialog::doDayBackup ();
	deleteInstances ();
	qApp->exit ( err_code );
	// When the main event loop is not running, the above function does nothing, so we must actually exit, then
	::exit ( err_code );
}

