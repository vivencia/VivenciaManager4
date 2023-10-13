#include "ui_backupdialog.h"
#include "backupdialog.h"
#include "global.h"
#include "system_init.h"

#include <vmlist.h>
#include <heapmanager.h>
#include <dbRecords/vivenciadb.h>
#include <vmNotify/vmnotify.h>
#include <vmWidgets/vmwidgets.h>
#include <vmTaskPanel/vmtaskpanel.h>
#include <vmTaskPanel/vmactiongroup.h>

#include <vmUtils/vmtextfile.h>
#include <vmUtils/vmcompress.h>
#include <vmUtils/fileops.h>
#include <vmUtils/configops.h>
#include <vmUtils/crashrestore.h>

#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>

const uint BACKUP_IDX ( 0 );
const uint RESTORE_IDX ( 1 );

auto backupFileList = [] () ->QString  { return CONFIG ()->appDataDir () + QStringLiteral ( "backups.db" ); };

BackupDialog::BackupDialog ()
	: QDialog ( nullptr ), ui ( new Ui::BackupDialog ), tdb ( nullptr ),
	  b_IgnoreItemListChange ( false ), mb_success ( false ), mb_nodb ( false ),
	  dlgNoDB ( nullptr ), mPanel ( nullptr ), m_after_close_action ( ACA_CONTINUE ), mErrorCode ( 0 )
{
	ui->setupUi ( this );

	if ( MAINWINDOW () )
	{
		mPanel = new vmTaskPanel ( emptyString, this );
		ui->verticalLayout->addWidget ( mPanel, 1 );
		vmActionGroup* group = mPanel->createGroup ( emptyString, false, true, false );
		group->addQEntry ( ui->toolBox, nullptr, true );
		group->addQEntry ( ui->frmBottomBar, nullptr, true );
	}

	setWindowTitle ( TR_FUNC ( "Backup/Restore - " ) + PROGRAM_NAME );
	setWindowIcon ( ICON ( APP_ICON ) );
	setupConnections ();
	scanBackupFiles ();
}

BackupDialog::~BackupDialog ()
{
	heap_del ( tdb );
	heap_del ( ui );
}

void BackupDialog::setupConnections ()
{
	ui->pBar->setVisible ( false );
	ui->btnApply->setEnabled ( false );
	connect ( ui->btnApply, &QPushButton::clicked, this, [&] () { return btnApply_clicked (); } );
	connect ( ui->btnClose, &QPushButton::clicked, this, [&] () { return btnClose_clicked (); } );

	connect ( ui->txtBackupFilename, &QLineEdit::textEdited, this, [&] ( const QString& text ) { return ui->btnApply->setEnabled ( text.length () > 3 ); } );
	connect ( ui->txtExportPrefix, &QLineEdit::textEdited, this, [&] ( const QString& text ) { return ui->btnApply->setEnabled ( text.length () > 3 ); } );
	connect ( ui->txtExportFolder, &QLineEdit::textEdited, this, [&] ( const QString& text ) { return ui->btnApply->setEnabled ( fileOps::exists ( text ).isOn () ); } );
	ui->txtRestoreFileName->setCallbackForContentsAltered ( [&] ( const vmWidget* ) {
		return ui->btnApply->setEnabled ( checkFile ( ui->txtRestoreFileName->text () ) ); } );

	connect ( ui->rdChooseKnownFile, &QRadioButton::toggled, this, [&] ( const bool checked ) {
		return rdChooseKnownFile_toggled ( checked ); } );
	connect ( ui->rdChooseAnotherFile, &QRadioButton::toggled, this, [&] ( const bool checked ) {
		return rdChooseAnotherFile_toggled ( checked ); } );

	connect ( ui->btnDefaultFilename, &QToolButton::clicked, this, [&] () {
		return ui->txtBackupFilename->setText ( standardDefaultBackupFilename () ); } );
	connect ( ui->btnDefaultPrefix, &QToolButton::clicked, this, [&] () {
		return ui->txtExportPrefix->setText ( vmNumber::currentDate ().toDate ( vmNumber::VDF_FILE_DATE ) + QStringLiteral ( "-Table_" ) ); } );
	connect ( ui->btnChooseBackupFolder, &QToolButton::clicked, this, [&] () {
		return ui->txtBackupFolder->setText ( fileOps::getExistingDir ( CONFIG ()->backupDir () ) ); } );
	connect ( ui->btnChooseExportFolder, &QToolButton::clicked, this, [&] () {
		return ui->txtExportFolder->setText ( fileOps::getExistingDir ( CONFIG ()->backupDir () ) ); } );
	connect ( ui->btnChooseImportFile, &QToolButton::clicked, this, [&] () {
		return btnChooseImportFile_clicked (); } );
	connect ( ui->btnRemoveFromList, &QToolButton::clicked, this, [&] () {
		return removeSelectedBackupFiles (); } );
	connect ( ui->btnRemoveAndDelete, &QToolButton::clicked, this, [&] () {
		return removeSelectedBackupFiles ( true ); } );

	connect ( ui->grpBackup, &QGroupBox::clicked, this, [&] ( const bool checked ) {
		return grpBackup_clicked ( checked ); } );
	connect ( ui->grpExportToText, &QGroupBox::clicked, this, [&] ( const bool checked ) {
		return grpExportToText_clicked ( checked ); } );

	ui->restoreList->setCallbackForCurrentItemChanged ( [&] (vmListItem* current_item ) { ui->btnApply->setEnabled ( current_item->row () != -1 ); } );

	ui->chkTables->setCallbackForContentsAltered ( [&] ( const vmWidget* ) {
		return ui->tablesList->setEnabled ( ui->chkTables->isChecked () ); } );

	auto widgetitem ( new QListWidgetItem ( ui->tablesList ) );
	widgetitem->setText ( TR_FUNC ( "Select all " ) );
	widgetitem->setFlags ( Qt::ItemIsEnabled|Qt::ItemIsAutoTristate|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable );
	widgetitem->setCheckState ( Qt::Checked );

	for ( uint i ( 0 ); i < TABLES_IN_DB; ++i )
	{
		widgetitem = new QListWidgetItem ( ui->tablesList );
		widgetitem->setText ( VivenciaDB::tableInfo ( i )->table_name );
		widgetitem->setFlags ( Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable );
		widgetitem->setCheckState ( Qt::Checked );
	}
	connect ( ui->tablesList, &QListWidget::itemChanged, this,
			  [&] ( QListWidgetItem* item ) { return selectAll ( item ); } );
}

bool BackupDialog::canDoBackup () const
{
	if ( VDB () != nullptr )
	{
		if ( !VDB ()->databaseIsEmpty () )
		{
			const bool ret ( !fileOps::appPath ( VivenciaDB::backupApp () ).isEmpty () );
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Backup" ), ret
				? TR_FUNC ( "Choose backup method" ) : VivenciaDB::backupApp () + TR_FUNC ( " must be installed to do backups" ) );
			return ret;
		}
		else
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Backup - Error" ), TR_FUNC ( "Database is empty. Nothing to backup" ) );
	}
	else
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Backup - Error" ), TR_FUNC ( "Database does not exist" ) );

	return false;
}

bool BackupDialog::canDoRestore () const
{
	if ( VDB () != nullptr )
	{
		if ( fileOps::appPath ( VivenciaDB::restoreApp () ).isEmpty () )
		{
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Restore - Error" ),
					VivenciaDB::restoreApp () + TR_FUNC ( " must be installed to restore a database" ) );
			return false;
		}
		else
			return true;
	}
	// New database
	NOTIFY ()->notifyMessage ( TR_FUNC ( "Restore - Next step" ), ( ui->restoreList->count () > 0 )
		? TR_FUNC ( "Choose restore method" ) : TR_FUNC ( "Choose a file containing a saved database to be restored" ) );
	return true;
}

void BackupDialog::scanBackupFiles ()
{
	const QString strBackupdir ( CONFIG ()->backupDir () );
	if ( fileOps::isDir ( strBackupdir ).isTrue () )
	{
		const QStringList nameFilters ( QStringList () << QStringLiteral ( ".bz2" ) << QStringLiteral ( ".sql" ) );
		pointersList<fileOps::st_fileInfo*> filesfound;
		filesfound.reserve ( 300 ); // FIX: pointerList has a bug reallocating more items on the fly
		filesfound.setAutoDeleteItem ( true );
		fileOps::lsDir ( filesfound, strBackupdir, nameFilters );
		uint n ( 0 );
		vmListItem* item ( nullptr );

		while ( n < filesfound.count () )
		{
			item = new vmListItem ( filesfound.at ( n )->filename );
			item->setData ( Qt::ToolTipRole, filesfound.at ( n )->fullpath );
			item->addToList ( ui->restoreList, false );
			++n;
		}
	}
	//Now add backup sources outside the default dir
	readFromBackupList ();
}

void BackupDialog::removeSelectedBackupFiles( const bool b_delete_file )
{
	if ( !ui->restoreList->selectedItems ().isEmpty () )
	{
		vmListItem* item ( nullptr );
		const QList<QTableWidgetItem*> sel_items ( ui->restoreList->selectedItems () );
		QList<QTableWidgetItem*>::const_iterator itr ( sel_items.constBegin () );
		const QList<QTableWidgetItem*>::const_iterator itr_end ( sel_items.constEnd () );
		for ( ; itr != itr_end; ++itr )
		{
			item = static_cast<vmListItem*> ( *itr );
			const int pos ( tdb->getRecord ( item->data ( Qt::ToolTipRole ).toString (), 0 ) );
			if ( pos >= 0 ) //If item is found in the backup files list, remove it from there
				tdb->deleteRecord ( pos );
			if ( b_delete_file )
				fileOps::removeFile ( item->data ( Qt::ToolTipRole ).toString () );
			ui->restoreList->removeItem ( item, true );
		}
	}
}

void BackupDialog::showWindow ()
{
	const bool b_backupEnabled ( canDoBackup () );
	const bool b_restoreEnabled ( canDoRestore () );

	ui->pageBackup->setEnabled ( b_backupEnabled );
	ui->pageRestore->setEnabled ( b_restoreEnabled );
	ui->toolBox->setCurrentIndex ( b_backupEnabled ? BACKUP_IDX : ( b_restoreEnabled ? RESTORE_IDX : BACKUP_IDX ) );
	ui->txtBackupFilename->clear ();
	ui->txtBackupFolder->clear ();
	ui->txtExportFolder->clear ();
	ui->txtExportPrefix->clear ();
	ui->txtRestoreFileName->clear ();
	ui->btnApply->setEnabled ( false );

	const bool hasRestoreList ( ui->restoreList->count () > 0 );
	ui->rdChooseKnownFile->setChecked ( hasRestoreList );
	ui->rdChooseAnotherFile->setChecked ( !hasRestoreList );

	m_after_close_action = ACA_RETURN_TO_PREV_WINDOW;

	/*
	 * This is hack used in BackupDialog and configDialog because, when themed, theses classes show a black background
	 * under some widgets. Don't know if it is because of the Qt theme I am using, or if it is not related to it. Don't konw
	 * if it is because the styleSheets described in ActionPanelScheme have some error. But by supplying the panel widget with and
	 * empty style sheet ( the default ), having it being drawn on the screen and then setting the correct stylesheet
	 * as it is displayed fixes the problem.
	 */
	QTimer::singleShot ( 100, this, [&] () { MAINWINDOW ()->appMainStyle ( mPanel ); } );
	this->exec ();
}

bool BackupDialog::doBackup ( const QString& filename, const QString& path, const bool bUserInteraction )
{
	if ( !BackupDialog::checkDir ( path ) )
	{
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Backup - Error"), TR_FUNC ( "You must select a valid directory before proceeding" ) );
		return false;
	}
	if ( filename.isEmpty () )
	{
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Backup - Error"), TR_FUNC ( "A filename must be supplied" ) );
		return false;
	}

	QString backupFile ( path + filename + QStringLiteral ( ".sql" ) );

	bool ok ( false );
	if ( Sys_Init::EXITING_PROGRAM || checkThatFileDoesNotExist ( backupFile + QStringLiteral ( ".bz2" ), bUserInteraction ) )
	{
		QString tables;
		if ( bUserInteraction )
		{
			if ( BACKUP ()->ui->chkDocs->isChecked () )
				BACKUP ()->addDocuments ( backupFile );
			if ( BACKUP ()->ui->chkImages->isChecked () )
				BACKUP ()->addImages ( backupFile );

			if ( BACKUP ()->ui->chkTables->isChecked () )
			{
				VDB ()->setCallbackForProgressMaxSteps ( [&] ( const uint max_steps ) { BACKUP ()->initProgressBar ( max_steps + 3 ); } );

				if ( BACKUP ()->ui->tablesList->item ( 0 )->checkState () == Qt::Unchecked )
				{
					NOTIFY ()->notifyMessage ( TR_FUNC ( "Backup - Error" ), TR_FUNC ( "One table at least must be selected." ) );
					return false;
				}

				if ( BACKUP ()->ui->tablesList->item ( 0 )->checkState () == Qt::PartiallyChecked )
				{
					for ( int i ( 1 ); i < BACKUP ()->ui->tablesList->count (); ++i )
					{
						if ( BACKUP ()->ui->tablesList->item ( i )->checkState () == Qt::Checked )
							tables += BACKUP ()->ui->tablesList->item ( i )->text () + CHR_SPACE;
					}
				}
			}
		}
		ok = VDB ()->doBackup ( backupFile, tables );
		if ( bUserInteraction )
			BACKUP ()->incrementProgress ();


		if ( ok )
		{
			if ( VMCompress::compress ( backupFile, backupFile + QStringLiteral ( ".bz2" ) ) )
			{
				fileOps::removeFile ( backupFile );
				backupFile += QStringLiteral ( ".bz2" );
				if ( bUserInteraction )
					BACKUP ()->incrementProgress ();

				const QString dropBoxDir ( CONFIG ()->dropboxDir () );
				if ( fileOps::isDir ( dropBoxDir ).isOn () )
					fileOps::copyFile ( dropBoxDir, backupFile );
				if ( bUserInteraction )
					BACKUP ()->incrementProgress ();
			}
		}
		if ( bUserInteraction )
		{
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Backup" ), TR_FUNC ( "Standard backup to file %1 was %2" ).arg (
							filename, ( ok ? QStringLiteral ( " successfull" ) : QStringLiteral ( " unsuccessfull" ) ) ) );
		}
		if ( ok )
			BackupDialog::addToBackupList ( backupFile, bUserInteraction ? BACKUP () : nullptr );
	}
	return ok;
}

void BackupDialog::doDayBackup ()
{
	if ( !VDB ()->backUpSynced () )
		BackupDialog::doBackup ( standardDefaultBackupFilename (), CONFIG ()->backupDir () );
}

bool BackupDialog::doExport ( const QString& prefix, const QString& path, const bool bUserInteraction )
{
	BackupDialog* bDlg ( bUserInteraction ? BACKUP () : nullptr );
	if ( bDlg )
	{
		if ( bDlg->ui->tablesList->item ( 0 )->checkState () == Qt::Unchecked )
		{
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Export - Error" ), TR_FUNC ( "One table at least must be selected." ) );
			return false;
		}
	}

	if ( !BackupDialog::checkDir ( path ) )
	{
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Export - Error" ), TR_FUNC ( "Error: you must select a valid directory before proceeding" ) );
		return false;
	}
	if ( prefix.isEmpty () )
	{
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Export - Error" ), TR_FUNC ( "Error: a prefix must be supplied" ) );
		return false;
	}

	QString filepath;
	bool ok ( false );

	if ( bDlg )
	{
		VDB ()->setCallbackForProgressMaxSteps ( [&] ( const uint max_steps ) {
			bDlg->initProgressBar ( max_steps * static_cast<uint>(bDlg->ui->tablesList->count () - 1) );
		} );

		for ( int i ( 1 ); i < bDlg->ui->tablesList->count (); ++i )
		{
			if ( bDlg->ui->tablesList->item ( i )->checkState () == Qt::Checked )
			{
				filepath = path + CHR_F_SLASH + prefix + bDlg->ui->tablesList->item ( i )->text ();
				if ( checkThatFileDoesNotExist ( filepath, true ) )
				{
					ok = VDB ()->exportToCSV ( static_cast<uint>(2<<(i-1)), filepath );
					if ( ok )
						BackupDialog::addToBackupList ( filepath, bDlg );
				}
			}
		}
		
		ui->pBar->hide ();
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Export" ), TR_FUNC ( "Export to CSV file was " ) + ( ok ? TR_FUNC ( " successfull" ) : TR_FUNC ( " unsuccessfull" ) ) );
	}
	else
	{
		for ( uint i ( 0 ); i < TABLES_IN_DB; ++i )
		{
			filepath = path + CHR_F_SLASH + prefix + VDB ()->tableName ( static_cast<TABLE_ORDER>(i) );
			if ( checkThatFileDoesNotExist ( filepath, false ) )
			{
				ok = VDB ()->exportToCSV ( static_cast<uint>(2)<<i, filepath );
				if ( ok )
					BackupDialog::addToBackupList ( filepath, bDlg );
			}
		}	
	}
	return ok;
}

bool BackupDialog::doRestore ( const QString& files )
{
	QStringList files_list ( files.split ( CHR_SPACE, Qt::SkipEmptyParts ) );
	if ( files_list.count () <= 0 )
		return false;

	QString filepath;
	VDB ()->setCallbackForProgressMaxSteps ( [&] ( const uint max_steps ) {
		initProgressBar ( max_steps * static_cast<uint>(files_list.count () - 1) );
	} );

	bool ok ( false );
	bool success ( false );

	QStringList::const_iterator itr ( files_list.constBegin () );
	const QStringList::const_iterator itr_end ( files_list.constEnd () );
	for ( ; itr != itr_end; ++itr )
	{
		filepath = static_cast<QString> ( *itr );
		if ( !checkFile ( filepath ) )
		{
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Restore - Error " ), filepath + TR_FUNC ( "is an invalid file" ) );
			continue;
		}
		if ( textFile::isTextFile ( filepath, textFile::TF_DATA ) )
			ok = VDB ()->importFromCSV ( filepath );
		else
		{
			ok = VDB ()->doRestore ( filepath );
		}
		if ( ok )
			addToBackupList ( filepath, this );
		success |= ok;
	}
	if ( success )
	{
		/* Force rescan of all tables, which is done in updateGeneralTable (called by VivenciaDB
		 * when general table cannot be found or there is a version mismatch
		 */
		VDB ()->clearTable ( VDB ()->tableInfo ( TABLE_GENERAL_ORDER ) );
		m_after_close_action = mb_nodb ? ACA_RETURN_TO_PREV_WINDOW : ACA_RESTART;
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Importing was sucessfull." ),
									  TR_FUNC ( "Click on the Close button to resume the application." ) );
	}
	else
	{
		m_after_close_action = mb_nodb ? ACA_RETURN_TO_PREV_WINDOW : ACA_CONTINUE;
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Importing failed!" ),
									  TR_FUNC ( "Try importing from another file or click on Close to exit the program." ) );
	}
	return ok;
}

//Only save the files that are not in the default backup dir
void BackupDialog::addToBackupList ( const QString& filepath, BackupDialog* bDlg )
{
	if ( fileOps::dirFromPath ( filepath ) == CONFIG ()->backupDir () )
		return;

	if ( bDlg )
	{
		for ( int i ( 0 ); i < bDlg->ui->restoreList->count (); ++i )
			if ( bDlg->ui->restoreList->item ( i )->data ( Qt::ToolTipRole ) == filepath ) return;
		auto item ( new vmListItem ( fileOps::fileNameWithoutPath ( filepath ) ) );
		item->setData ( Qt::ToolTipRole, filepath );
		item->addToList ( bDlg->ui->restoreList, false );
		bDlg->tdb->appendRecord ( filepath );
	}
	else
	{
		dataFile* df ( new dataFile ( backupFileList () ) );
		df->load ();
		df->appendRecord ( filepath );
		delete df;
	}
}

void BackupDialog::readFromBackupList ()
{
	if ( !tdb )
		tdb = new dataFile ( backupFileList () );
	if ( tdb->load ().isOn () )
	{
		stringRecord files;
		uint i ( 0 );
		while ( tdb->getRecord ( files, i ) )
		{
			if ( files.first () )
			{
				if ( checkFile ( files.curValue () ) )
				{
					auto item ( new vmListItem ( fileOps::fileNameWithoutPath ( files.curValue () ) ) );
					item->setData ( Qt::ToolTipRole, files.curValue () );
					item->addToList ( ui->restoreList, false );
				}
				else // file does not exist; do not add to the list and remove it from database
				{
					files.removeFieldByValue ( files.curValue (), true );
					tdb->changeRecord ( 0, files );
				}
			}
			++i;
		}
	}
}

int BackupDialog::showNoDatabaseOptionsWindow ()
{
	mb_nodb = true;
	m_after_close_action = ACA_RETURN_TO_PREV_WINDOW;
	if ( dlgNoDB == nullptr )
	{
		dlgNoDB = new QDialog ( this );
		dlgNoDB->setWindowIcon ( ICON ( APP_ICON ) );
		dlgNoDB->setWindowTitle ( TR_FUNC ( "Database inexistent" ) );

		auto lblExplanation ( new QLabel ( TR_FUNC (
				"There is no database present. This either means that this is the first time this program "
				"is used or the current database was deleted. You must choose one of the options below:" ) ) );
		lblExplanation->setAlignment ( Qt::AlignJustify );
		lblExplanation->setWordWrap ( true );

		rdImport = new QRadioButton ( TR_FUNC ( "Import from backup" ) );
		rdImport->setMinimumHeight ( 30 );
		rdImport->setChecked ( true );
		rdStartNew = new QRadioButton ( TR_FUNC ( "Start a new database" ) );
		rdStartNew->setMinimumHeight ( 30 );
		rdStartNew->setChecked ( false );
		rdNothing = new QRadioButton ( TR_FUNC ( "Do nothing and exit" ) );
		rdNothing->setMinimumHeight ( 30 );
		rdNothing->setChecked ( false );

		btnProceed = new QPushButton ( TR_FUNC ( "&Proceed" ) );
		btnProceed->setMinimumSize ( 100, 30 );
		btnProceed->setDefault ( true );
		static_cast<void>( connect ( btnProceed, &QPushButton::clicked, this, [&] () { return btnNoDBProceed_clicked (); } ) );

		auto layout ( new QVBoxLayout );
		layout->addWidget ( lblExplanation, 1, Qt::AlignJustify );
		layout->addWidget ( rdImport, 1, Qt::AlignLeft );
		layout->addWidget ( rdStartNew, 1, Qt::AlignLeft );
		layout->addWidget ( rdNothing, 1, Qt::AlignLeft );
		layout->addWidget ( btnProceed, 1, Qt::AlignCenter );

		layout->setContentsMargins ( 0, 0, 0, 0 );
		layout->setSpacing ( 1 );
		dlgNoDB->adjustSize ();
		dlgNoDB->setLayout ( layout );

		const QRect desktopGeometry ( QGuiApplication::primaryScreen ()->availableGeometry () );
		dlgNoDB->move ( ( desktopGeometry.width () - width () ) / 2, ( desktopGeometry.height () - height () ) / 2  );
	}
	dlgNoDB->exec ();
	return mErrorCode;
}

void BackupDialog::btnNoDBProceed_clicked ()
{
	if ( !rdNothing->isChecked () )
	{
		ui->toolBox->setCurrentIndex ( 1 );
		ui->toolBox->setEnabled ( true );
		ui->toolBox->widget ( 0 )->setEnabled ( false );
		crashRestore::setNewDatabaseSession ( true );

		//Instruct VivenciaDB::createDatabase to create the tables when not importing from a file. The importing process creates the tables
		//based on the information contained in the file
		mb_success = VDB ()->createDatabase ( !rdImport->isChecked () );
		if ( actionSuccess () )
		{
			if ( rdImport->isChecked () )
			{
				dlgNoDB->hide ();
				exec ();
			}
			if ( actionSuccess () )
			{
				mb_nodb = false;
				dlgNoDB->done ( QDialog::Accepted );
				mErrorCode =  NO_ERR;
				return;
			}
		}
		dlgNoDB->done ( QDialog::Rejected );
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Database creation failed" ),
									  TR_FUNC ( "Please try again. If the problem persists ... who knows?" ), 3000, true );
		close ();
		mErrorCode = ERR_DATABASE_PROBLEM;
	}
	else
		mErrorCode = ERR_NO_DB;
}

void BackupDialog::selectAll ( QListWidgetItem* item )
{
	if ( b_IgnoreItemListChange ) return;

	b_IgnoreItemListChange = true;
	if ( item == ui->tablesList->item ( 0 ) )
	{
		for ( int i ( 1 ); i < ui->tablesList->count (); ++i )
			ui->tablesList->item ( i )->setCheckState ( item->checkState () );
	}
	else
	{
		int checked ( 0 );
		for ( int i ( 1 ); i < ui->tablesList->count (); ++i )
		{
			if ( ui->tablesList->item ( i )->checkState () == Qt::Checked )
				++checked;
			else
				--checked;
		}
		if ( checked == ui->tablesList->count () - 1 )
			ui->tablesList->item ( 0 )->setCheckState ( Qt::Checked );
		else if ( checked == 0 - ( ui->tablesList->count () - 1 ) )
			ui->tablesList->item ( 0 )->setCheckState ( Qt::Unchecked );
		else
			ui->tablesList->item ( 0 )->setCheckState ( Qt::PartiallyChecked );
	}
	b_IgnoreItemListChange = false;
}

void BackupDialog::btnApply_clicked ()
{
	ui->btnClose->setEnabled ( false );
	mb_success = false;
	if ( ui->toolBox->currentIndex () == 0 )
	{ // backup
		if ( ui->grpBackup->isChecked () )
		{
			if ( !ui->txtBackupFolder->text ().isEmpty () )
			{
				if ( ui->txtBackupFolder->text ().at ( ui->txtBackupFolder->text ().length () - 1 ) != '/' )
					ui->txtBackupFolder->setText ( ui->txtBackupFolder->text () + CHR_F_SLASH );
				mb_success = doBackup ( ui->txtBackupFilename->text (), ui->txtBackupFolder->text (), true );
			}
		}
		else
			mb_success = doExport ( ui->txtExportPrefix->text (), ui->txtExportFolder->text (), true );
	}
	else
	{ //restore
		QString selected;
		if ( ui->rdChooseKnownFile->isChecked () )
		{
			if ( ui->restoreList->currentRow () <= 0 )
				ui->restoreList->setCurrentRow ( ui->restoreList->count () - 1, false, true );
			selected = ui->restoreList->currentItem ()->text ();
		}
		else
			selected = ui->txtRestoreFileName->text ();
		mb_success = doRestore ( selected );
	}
	ui->btnApply->setEnabled ( false );
	ui->btnClose->setEnabled ( true );
}

void BackupDialog::btnClose_clicked ()
{
	switch ( m_after_close_action )
	{
		case ACA_RETURN_TO_PREV_WINDOW:
			done ( actionSuccess () );
		break;
		case ACA_RESTART:
			Sys_Init::restartProgram ();
		break;
		case ACA_CONTINUE:
			ui->btnApply->setEnabled ( true );
		break;
	}
}

void BackupDialog::grpBackup_clicked ( const bool checked )
{
	ui->grpExportToText->setChecked ( !checked );
	ui->chkTables->setChecked ( checked );
	ui->tablesList->setEnabled ( checked );
	ui->btnApply->setEnabled ( true );
}

void BackupDialog::grpExportToText_clicked ( const bool checked )
{
	ui->grpBackup->setChecked ( !checked );
	ui->btnApply->setEnabled ( true );
}
//-----------------------------------------------------------RESTORE---------------------------------------------------

bool BackupDialog::getSelectedItems ( QString& selected )
{
	if ( !ui->restoreList->selectedItems ().isEmpty () )
	{
		selected.clear ();
		const QList<QTableWidgetItem*> sel_items ( ui->restoreList->selectedItems () );
		QList<QTableWidgetItem*>::const_iterator itr ( sel_items.constBegin () );
		const QList<QTableWidgetItem*>::const_iterator itr_end ( sel_items.constEnd () );
		for ( ; itr != itr_end; ++itr )
		{
			if ( !selected.isEmpty () )
				selected += CHR_SPACE;
			selected += static_cast<vmListItem*> ( *itr )->text ();
		}
		return true;
	}

	NOTIFY ()->notifyMessage ( TR_FUNC ( "Restore - Error" ), TR_FUNC ( "No file selected" ) );
	return false;
}

void BackupDialog::initProgressBar ( const uint max )
{
	ui->pBar->reset ();
	ui->pBar->setRange ( 0, static_cast<int>( max ) );
	ui->pBar->setVisible ( true );
}

inline void BackupDialog::incrementProgress ()
{
	ui->pBar->setValue ( ui->pBar->value () + 1 );
}

bool BackupDialog::checkThatFileDoesNotExist ( const QString& filepath, const bool bUserInteraction )
{
	if ( fileOps::exists ( filepath ).isOn () )
	{
		if ( bUserInteraction )
		{
			const QString question ( QString ( TR_FUNC ( "The file %1 already exists. Overwrite it?" ) ).arg ( filepath ) );
			if ( !NOTIFY ()->questionBox ( TR_FUNC ( "Same filename" ), question ) )
				return false;
			fileOps::removeFile ( filepath );
		}
		else
			return false;
	}
	return true;
}

bool BackupDialog::checkDir ( const QString& dir )
{
	if ( fileOps::isDir ( dir ).isOn () )
	{
		if ( fileOps::canWrite ( dir ).isOn () )
		{
			if ( fileOps::canRead ( dir ).isOn () )
				return fileOps::canExecute ( dir ).isOn ();
		}
	}
	return false;
}

bool BackupDialog::checkFile ( const QString& file )
{
	if ( fileOps::isFile ( file ).isOn () )
	{
		return fileOps::canRead ( file ).isOn ();
	}
	return false;
}

void BackupDialog::addDocuments ( const QString& tar_file )
{
	tar_file.toInt ();
// Browse CONFIG ()->projectsBaseDir () for files with CONFIG ()->xxxExtension
	// Copy them to the temporary dir (in /tmp)
	// Add temp dir to tar_file
	// erase temp dir whatever the result
	//TODO
}


void BackupDialog::addImages ( const QString& tar_file )
{
	tar_file.toInt ();
}

void BackupDialog::rdChooseKnownFile_toggled ( const bool checked )
{
	ui->rdChooseAnotherFile->setChecked ( !checked );
	ui->txtRestoreFileName->setEnabled ( !checked );
	ui->btnChooseImportFile->setEnabled ( !checked );
	ui->restoreList->setEnabled ( checked );
	ui->btnApply->setEnabled ( checked && ui->restoreList->count () > 0 );
}

void BackupDialog::rdChooseAnotherFile_toggled ( const bool checked )
{
	ui->rdChooseKnownFile->setChecked ( !checked );
	ui->txtRestoreFileName->setEnabled ( checked );
	ui->btnChooseImportFile->setEnabled ( checked );
	ui->restoreList->setEnabled ( !checked );
	ui->btnApply->setEnabled ( checked && !ui->txtRestoreFileName->text ().isEmpty () );
}

void BackupDialog::btnChooseImportFile_clicked ()
{
	const QString selectedFile ( fileOps::getOpenFileName ( CONFIG ()->backupDir (), TR_FUNC ( "Database files ( *.bz2 *.sql )" ) ) );
	if ( !selectedFile.isEmpty () )
		ui->txtRestoreFileName->setText ( selectedFile );
	ui->btnApply->setEnabled ( !selectedFile.isEmpty () );
}
