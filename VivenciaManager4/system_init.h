#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include "companypurchases.h"
#include "suppliersdlg.h"
#include "estimates.h"
#include "configdialog.h"
#include "backupdialog.h"
#include "searchui.h"

#include <dbRecords/vivenciadb.h>
#include <vmDocumentEditor/documenteditor.h>
#include <vmCalculator/simplecalculator.h>
#include <vmDocumentEditor/spreadsheet.h>
#include <vmUtils/crashrestore.h>

#include <QtCore/QString>

class vmCompleters;
class configOps;
class vmNotify;
class MainWindow;

namespace Sys_Init
{
	extern void init ( const QString& cmd );
	extern void deInit ( int err_code = 0 );
	extern void cleanUpApp ();
	
	extern DB_ERROR_CODES checkSystem ( const bool bFirstPass = true );
	extern DB_ERROR_CODES checkLocalSetup ();
	extern void checkSetup ();
	extern void restartProgram ();
	extern bool isMySQLRunning();
	extern QString commandMySQLServer ( const QString& command, const bool only_return_cmd_line = false );

	extern bool EXITING_PROGRAM;
	extern QString APP_START_CMD;

	extern MainWindow* mainwindow_instance;
	extern configOps* config_instance;
	extern VivenciaDB* vdb_instance;
	extern vmCompleters* completers_instance;
	extern vmNotify* notify_instance;
	extern documentEditor* editor_instance;
	extern companyPurchasesUI* cp_instance;
	extern suppliersDlg* sup_instance;
	extern estimateDlg* estimates_instance;
	extern searchUI* search_instance;
	extern simpleCalculator* calc_instance;
	extern configDialog* configdlg_instance;
	extern spreadSheetEditor* qp_instance;
	extern BackupDialog* backup_instance;
	extern restoreManager* restore_instance;

	static inline bool isEditorStarted () { return editor_instance != nullptr; }
}

inline MainWindow* MAINWINDOW ()
{
	return Sys_Init::mainwindow_instance;
}

inline VivenciaDB* VDB ()
{
	return Sys_Init::vdb_instance;
}

inline configOps* CONFIG ()
{
	return Sys_Init::config_instance;
}

inline vmCompleters* COMPLETERS ()
{
	return Sys_Init::completers_instance;
}

inline vmNotify* NOTIFY ()
{
	return Sys_Init::notify_instance;
}

inline documentEditor* EDITOR ()
{
	if ( Sys_Init::editor_instance == nullptr )
		Sys_Init::editor_instance = new documentEditor ( MAINWINDOW (), COMPLETERS () );
	return Sys_Init::editor_instance;
}

inline companyPurchasesUI* COMPANY_PURCHASES ()
{
	if ( Sys_Init::cp_instance == nullptr )
		Sys_Init::cp_instance = new companyPurchasesUI;
	return Sys_Init::cp_instance;
}

inline suppliersDlg *SUPPLIERS ()
{
	if ( Sys_Init::sup_instance == nullptr )
		Sys_Init::sup_instance = new suppliersDlg;
	return Sys_Init::sup_instance;
}

inline estimateDlg *ESTIMATES ()
{
	if ( Sys_Init::estimates_instance == nullptr )
		Sys_Init::estimates_instance = new estimateDlg;
	return Sys_Init::estimates_instance;
}

inline searchUI *SEARCH_UI ()
{
	if ( Sys_Init::search_instance == nullptr )
		Sys_Init::search_instance = new searchUI;
	return Sys_Init::search_instance;
}

inline simpleCalculator *CALCULATOR ()
{
	if ( Sys_Init::calc_instance == nullptr )
		Sys_Init::calc_instance = new simpleCalculator;
	return Sys_Init::calc_instance;
}

inline configDialog* CONFIG_DIALOG ()
{
	if ( Sys_Init::configdlg_instance == nullptr )
		Sys_Init::configdlg_instance = new configDialog;
	return Sys_Init::configdlg_instance;
}

inline spreadSheetEditor* QUICK_PROJECT ()
{
	if ( Sys_Init::qp_instance == nullptr )
		Sys_Init::qp_instance = new spreadSheetEditor ( nullptr, COMPLETERS () );
	return Sys_Init::qp_instance;
}

inline BackupDialog* BACKUP ()
{
	if ( !Sys_Init::backup_instance )
		Sys_Init::backup_instance = new BackupDialog;
	return Sys_Init::backup_instance;
}

inline restoreManager* APP_RESTORER ()
{
	if ( !Sys_Init::restore_instance )
		Sys_Init::restore_instance = new restoreManager;
	return Sys_Init::restore_instance;
}
#endif // SYSTEM_INIT_H
