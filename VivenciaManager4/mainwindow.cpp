#include "ui_mainwindow.h"
#include "mainwindow.h"
#include "global.h"
#include "system_init.h"
#include "backupdialog.h"
#include "suppliersdlg.h"
#include "separatewindow.h"
#include "searchui.h"
#include "machinesdlg.h"
#include "configdialog.h"
#include "dbstatistics.h"
#include "dbtableview.h"
#include "calendarviewui.h"

#include <heapmanager.h>
#include <vmCalculator/simplecalculator.h>
#include <dbRecords/completers.h>
#include <dbRecords/vivenciadb.h>
#include <dbRecords/dbcalendar.h>
#include <dbRecords/supplierrecord.h>
#include <dbRecords/dbsupplies.h>
#include <vmUtils/fast_library_functions.h>
#include <vmUtils/fileops.h>
#include <vmUtils/configops.h>
#include <vmUtils/crashrestore.h>
#include <vmUtils/imageviewer.h>
#include <vmNotify/vmnotify.h>
#include <vmDocumentEditor/spreadsheet.h>
#include <vmDocumentEditor/reportgenerator.h>
#include <vmTaskPanel/vmtaskpanel.h>
#include <vmTaskPanel/vmactiongroup.h>
#include <vmTaskPanel/actionpanelscheme.h>

#include <QtCore/QScopedPointer>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QtEvents>
#include <QtGui/QKeyEvent>
#include <QtGui/QScreen>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QToolBar>

#ifdef DEBUG
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
static void msleep ( unsigned long msecs )
{
	QMutex mutex;
	mutex.lock ();
	QWaitCondition waitCondition;
	waitCondition.wait ( &mutex, msecs );
	mutex.unlock ();
}
#endif

enum JOB_INFO_LIST_USER_ROLES
{
	JILUR_DATE = Qt::UserRole + Job::JRF_DATE,
	JILUR_START_TIME = Qt::UserRole + Job::JRF_START_TIME,
	JILUR_END_TIME = Qt::UserRole + Job::JRF_END_TIME,
	JILUR_WHEATHER = Qt::UserRole + Job::JRF_WHEATHER,
	JILUR_REPORT = Qt::UserRole + Job::JRF_REPORT,
	JILUR_ADDED = JILUR_REPORT + 1,
	JILUR_REMOVED = JILUR_REPORT + 2,
	JILUR_EDITED = JILUR_REPORT + 3,
	JILUR_DAY_TIME = JILUR_REPORT + 4
};

static const QString lstJobReportItemPrefix ( QStringLiteral ( "o dia - " ) );
const char* const PROPERTY_TOTAL_PAYS ( "ptp" );

static const QString chkPayOverdueToolTip_overdue[3] = {
	APP_TR_FUNC ( "The total price paid for the job is met, nothing needs to be done." ),
	APP_TR_FUNC ( "The total paid is less then the total price for the job.\n"
	"Check the box to ignore that discrepancy and mark the payment concluded as is." ),
	APP_TR_FUNC ( "Ignoring whether payment is overdue or not." )
};

static const QString jobPicturesSubDir ( QStringLiteral ( "Pictures/" ) );

const auto screen_width = [] ()-> int
{
	return QGuiApplication::primaryScreen ()->availableGeometry ().width ();
};

MainWindow::MainWindow ()
	: QMainWindow ( nullptr ),
	  ui ( new Ui::MainWindow ),  menuJobDoc ( nullptr ), menuJobXls ( nullptr ), menuJobPdf ( nullptr ),
		  subMenuJobPdfView ( nullptr ), sepWin_JobPictures ( nullptr ), sepWin_JobReport ( nullptr ), mCal ( new dbCalendar ),
	  mTableView ( nullptr ), mCalendarView ( nullptr ), mStatistics ( nullptr ), mMachines ( nullptr ), jobsPicturesMenuAction ( nullptr ),
	  mainTaskPanel ( nullptr ), selJob_callback ( nullptr ), mb_jobPosActions ( false ), mClientCurItem ( nullptr ),
	  mJobCurItem ( nullptr ), mPayCurItem ( nullptr ), mBuyCurItem ( nullptr ), dlgSaveEditItems ( nullptr )
{
	setWindowIcon ( ICON ( APP_ICON ) );
	setWindowTitle ( PROGRAM_NAME + QStringLiteral ( " - " ) + PROGRAM_VERSION );
	createTrayIcon (); // create the icon for NOTIFY () use
	setStatusBar ( nullptr ); // we do not want a statusbar
}

MainWindow::~MainWindow ()
{
	saveEditItems ();
	
	ui->clientsList->setIgnoreChanges ( true );
	ui->jobsList->setIgnoreChanges ( true );
	ui->paysList->setIgnoreChanges ( true );
	ui->buysList->setIgnoreChanges ( true );
	ui->buysJobList->setIgnoreChanges ( true );
	ui->lstJobDayReport->setIgnoreChanges ( true );
	ui->lstBuySuppliers->setIgnoreChanges ( true );
	ui->paysOverdueList->setIgnoreChanges ( true );
	ui->paysOverdueClientList->setIgnoreChanges ( true );
	ui->tableBuyItemsSelectiveView->setIgnoreChanges ( true );

	for ( int i ( ui->clientsList->count () - 1 ); i >= 0; --i )
		delete ui->clientsList->item ( i );
	
	CONFIG ()->saveWindowGeometry ( this, Ui::mw_configSectionName, Ui::mw_configCategoryWindowGeometry );

	heap_del ( sepWin_JobPictures );
	heap_del ( sepWin_JobReport );
	heap_del ( mCal );
	heap_del ( mTableView );
	heap_del ( mCalendarView );
	heap_del ( mStatistics );
	heap_del ( mMachines );
	heap_del ( ui );
}

void MainWindow::continueStartUp ()
{
	mainTaskPanel = new vmTaskPanel;
	CONFIG ()->addManagedSectionName ( Ui::mw_configSectionName );
	CONFIG ()->addManagedSectionName ( configOps::foldersSectionName () );
	CONFIG ()->addManagedSectionName ( configOps::appDefaultsSectionName () );
	QString strStyle ( CONFIG ()->getValue ( Ui::mw_configSectionName, Ui::mw_configCategoryAppScheme, false ) );
	if ( strStyle.isEmpty () )
		strStyle = ActionPanelScheme::PanelStyleStr[0];
	changeSchemeStyle ( strStyle );

	//Having the window size information before adjusting the UI works better for some widgets' placement like the imageViewer
	CONFIG ()->getWindowGeometry ( this, Ui::mw_configSectionName, Ui::mw_configCategoryWindowGeometry );
	show ();

	ui->setupUi ( this );
	setupCustomControls ();
	restoreLastSession ();
	
	// connect for last to avoid unnecessary signal emission when we are removing tabs
	static_cast<void>( connect ( ui->tabMain, &QTabWidget::currentChanged, this, &MainWindow::tabMain_currentTabChanged ) );
	static_cast<void>( connect ( qApp, &QCoreApplication::aboutToQuit, this, [&] () { return Sys_Init::deInit (); } ) );
	this->tabPaysLists_currentChanged ( 0 ); // update pay totals line edit text
}
//--------------------------------------------CLIENT------------------------------------------------------------
//TODO
void MainWindow::saveClientWidget ( vmWidget* widget, const int id )
{
	widget->setID ( id );
	clientWidgetList[id] = widget;
}

void MainWindow::showClientSearchResult ( dbListItem* item, const bool bshow )
{
	if ( bshow )
		displayClient ( static_cast<clientListItem*> ( const_cast<dbListItem*> ( item ) ), true );

	for ( uint i ( 0 ); i < CLIENT_FIELD_COUNT; ++i )
	{
		if ( item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
			clientWidgetList.at ( i )->highlight ( bshow ? vmBlue : vmDefault_Color, SEARCH_UI ()->searchTerm () );
	}
}

void MainWindow::setupClientPanel ()
{
	ui->clientsList->setParentLayout ( ui->gLayoutClientListPanel );
	ui->clientsList->setUtilitiesPlaceLayout ( ui->layoutClientsList );
	ui->clientsList->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) {
		return clientsListWidget_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );
	ui->clientsList->setCallbackForRowActivated ( [&] ( const uint row ) { return insertNavItem ( static_cast<dbListItem*>( ui->clientsList->item ( static_cast<int>( row ) ) ) ); } );
	ui->clientsList->setCallbackForWidgetGettingFocus ( [&] () { ui->lblCurInfoClient->callLabelActivatedFunc (); } );

	saveClientWidget ( ui->txtClientID, FLD_CLIENT_ID );
	saveClientWidget ( ui->txtClientName, FLD_CLIENT_NAME );
	COMPLETERS ()->setCompleterForWidget ( ui->txtClientName, CC_CLIENT_NAME );
	ui->txtClientName->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtClientName_textAltered ( sender->text () ); } );

	saveClientWidget ( ui->txtClientCity, FLD_CLIENT_CITY );
	COMPLETERS ()->setCompleterForWidget ( ui->txtClientCity, CC_ADDRESS );
	ui->txtClientCity->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtClient_textAltered ( sender ); } );

	saveClientWidget ( ui->txtClientDistrict, FLD_CLIENT_DISTRICT );
	COMPLETERS ()->setCompleterForWidget ( ui->txtClientDistrict, CC_ADDRESS );
	ui->txtClientDistrict->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtClient_textAltered ( sender ); } );

	saveClientWidget ( ui->txtClientStreetAddress, FLD_CLIENT_STREET );
	COMPLETERS ()->setCompleterForWidget ( ui->txtClientStreetAddress, CC_ADDRESS );
	ui->txtClientStreetAddress->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtClient_textAltered ( sender ); } );

	saveClientWidget ( ui->txtClientNumberAddress, FLD_CLIENT_NUMBER );
	ui->txtClientNumberAddress->setTextType ( vmWidget::TT_INTEGER );
	ui->txtClientNumberAddress->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtClient_textAltered ( sender ); } );

	saveClientWidget ( ui->txtClientZipCode, FLD_CLIENT_ZIP );
	ui->txtClientZipCode->setTextType ( vmWidget::TT_ZIPCODE );
	ui->txtClientZipCode->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtClientZipCode_textAltered ( sender->text () ); } );

	saveClientWidget ( ui->contactsClientPhones, FLD_CLIENT_PHONES );
	ui->contactsClientPhones->setContactType ( contactsManagerWidget::CMW_PHONES );
	ui->contactsClientPhones->initInterface ();
	ui->contactsClientPhones->setCallbackForInsertion ( [&] ( const QString& phone, const vmWidget* const sender ) {
		return contactsClientAdd ( phone, sender ); } );
	ui->contactsClientPhones->setCallbackForRemoval ( [&] ( const int idx, const vmWidget* const sender ) {
		return contactsClientDel ( idx, sender ); } );
	ui->contactsClientEmails->setID ( FLD_CLIENT_EMAIL );
	ui->contactsClientEmails->setContactType ( contactsManagerWidget::CMW_EMAIL );
	ui->contactsClientEmails->initInterface ();
	ui->contactsClientEmails->setCallbackForInsertion ( [&] ( const QString& addrs, const vmWidget* const sender ) {
		return contactsClientAdd ( addrs, sender ); } );
	ui->contactsClientEmails->setCallbackForRemoval ( [&] ( const int idx, const vmWidget* const sender ) {
		return contactsClientDel ( idx, sender ); } );

	saveClientWidget ( ui->dteClientStart, FLD_CLIENT_STARTDATE );
	ui->dteClientStart->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dteClient_dateAltered ( sender ); } );
	ui->dteClientEnd->setID ( FLD_CLIENT_ENDDATE );

	saveClientWidget ( ui->chkClientActive, FLD_CLIENT_STATUS );
	ui->chkClientActive->setLabel ( TR_FUNC ( "Currently active" ) );
	ui->chkClientActive->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtClient_textAltered ( sender ); } );
}

void MainWindow::clientsListWidget_currentItemChanged ( dbListItem* item )
{
	static_cast<void>( displayClient ( static_cast<clientListItem*>( item ) ) );
}

void MainWindow::controlClientForms ( const clientListItem* const client_item )
{
	const triStateType editing_action ( client_item ? static_cast<TRI_STATE> ( client_item->action () >= ACTION_ADD ) : TRI_UNDEF );

	ui->txtClientName->setEditable ( editing_action.isOn () );
	ui->txtClientCity->setEditable ( editing_action.isOn () );
	ui->txtClientDistrict->setEditable ( editing_action.isOn () );
	ui->txtClientStreetAddress->setEditable ( editing_action.isOn () );
	ui->txtClientNumberAddress->setEditable ( editing_action.isOn () );
	ui->txtClientZipCode->setEditable ( editing_action.isOn () );
	ui->dteClientStart->setEditable ( editing_action.isOn () );
	ui->dteClientEnd->setEditable ( editing_action.isOn () );
	ui->chkClientActive->setEditable ( editing_action.isOn () );

	sectionActions[0].b_canAdd = true;
	sectionActions[0].b_canEdit = editing_action.isOff ();
	sectionActions[0].b_canRemove = editing_action.isOff ();
	sectionActions[0].b_canSave = editing_action.isOn () ? client_item->isGoodToSave () : false;
	sectionActions[0].b_canCancel = editing_action.isOn ();
	updateActionButtonsState ();

	ui->contactsClientPhones->setEditable ( editing_action.isOn () );
	ui->contactsClientEmails->setEditable ( editing_action.isOn () );

	ui->txtClientName->highlight ( editing_action.isOn () ? ( client_item->isGoodToSave () ?
			vmDefault_Color : vmRed ) : ( ui->chkClientActive->isChecked () ? vmGreen : vmDefault_Color ) );
}

bool MainWindow::saveClient ( clientListItem* client_item, const bool b_dbsave )
{
	if ( client_item != nullptr )
	{
		if ( client_item->clientRecord ()->saveRecord ( false, b_dbsave ) )
		{
			client_item->setAction ( ACTION_READ, true ); // now we can change indexes
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Record saved!" ), TR_FUNC ( "Client data saved!" ) );
			controlClientForms ( client_item );
			controlJobForms ( nullptr );
			changeNavLabels ();
			removeEditItem ( static_cast<dbListItem*>(client_item) );
			return true;
		}
	}
	return false;
}

clientListItem* MainWindow::addClient ()
{
	auto client_item ( new clientListItem );
	client_item->setRelation ( RLI_CLIENTITEM );
	client_item->setRelatedItem ( RLI_CLIENTPARENT, client_item );
	static_cast<void>( client_item->loadData  ( false ) );
	client_item->setAction ( ACTION_ADD, true );
	client_item->addToList ( ui->clientsList );
	insertEditItem ( static_cast<dbListItem*>( client_item ) );
	client_item->setDBRecID ( static_cast<uint>( recIntValue ( client_item->clientRecord (), FLD_CLIENT_ID ) ) );
	ui->txtClientName->setFocus ();
	return client_item;
}

bool MainWindow::editClient ( clientListItem* client_item, const bool b_dogui )
{
	if ( client_item )
	{
		if ( client_item->action () == ACTION_READ )
		{
			client_item->setAction ( ACTION_EDIT, true );
			insertEditItem ( static_cast<dbListItem*>( client_item ) );
			if ( b_dogui )
			{
				NOTIFY ()->notifyMessage ( TR_FUNC ( "Editing client record" ),
						recStrValue ( client_item->clientRecord (), FLD_CLIENT_NAME ) );
				controlClientForms ( client_item );
				ui->txtClientName->setFocus ();
			}
			return true;
		}
	}
	return false;
}

bool MainWindow::delClient ( clientListItem* client_item, const bool b_ask )
{
	if ( client_item )
	{
		if ( b_ask )
		{
			const QString text ( TR_FUNC ( "Are you sure you want to remove %1's record?" ) );
			if ( !NOTIFY ()->questionBox ( TR_FUNC ( "Question" ), text.arg ( recStrValue ( client_item->clientRecord (), FLD_CLIENT_NAME ) ) ) )
				return false;
		}
		client_item->setAction ( ACTION_DEL, true );
		return cancelClient ( client_item );
	}
	return false;
}

bool MainWindow::cancelClient ( clientListItem* client_item )
{
	if ( client_item )
	{
		switch ( client_item->action () )
		{
			case ACTION_ADD:
			case ACTION_DEL:
				mClientCurItem = nullptr;
				removeListItem ( client_item );
			break;
			case ACTION_EDIT:
				removeEditItem ( static_cast<dbListItem*>( client_item ) );
				client_item->setAction ( ACTION_REVERT, true );
				displayClient ( client_item );
			break;
			case ACTION_NONE:
			case ACTION_READ:
			case ACTION_REVERT:
				return false;
		}
		return true;
	}
	return false;
}

bool MainWindow::displayClient ( clientListItem* client_item, const bool b_select, jobListItem* job_item, buyListItem* buy_item, const uint job_id , const uint buy_id )
{
	// This function is called by user interaction and programatically. When called programatically, item may be nullptr
	if ( client_item )
	{
		if ( client_item->loadData ( true ) )
		{
			if ( client_item != mClientCurItem )
			{
				if ( b_select )
					ui->clientsList->setCurrentItem ( client_item, true, false );
				alterLastViewedRecord ( mClientCurItem, client_item );
				mClientCurItem = client_item;
				fillAllLists ( client_item, job_item, buy_item, job_id, buy_id );
			}
			controlClientForms ( client_item );
			loadClientInfo ( client_item->clientRecord () );
			if ( job_item == nullptr )
				job_item = client_item->jobs->last ();
			displayJob ( job_item, true, buy_item );
			if ( ui->tabPaysLists->currentIndex () == 1 )
				loadClientOverduesList ();
		}
	}
	changeNavLabels ();
	return ( client_item != nullptr );
}

void MainWindow::loadClientInfo ( const Client* const client )
{
	if ( client != nullptr )
	{
		ui->txtClientID->setText ( recStrValue ( client, FLD_CLIENT_ID ) );
		ui->txtClientName->setText ( recStrValue ( client, FLD_CLIENT_NAME ) );
		ui->txtClientCity->setText ( client->action () != ACTION_ADD ? recStrValue ( client, FLD_CLIENT_CITY ) : QStringLiteral ( "São Pedro" ) );
		ui->txtClientDistrict->setText ( recStrValue ( client, FLD_CLIENT_DISTRICT ) );
		ui->txtClientNumberAddress->setText ( recStrValue ( client, FLD_CLIENT_NUMBER ) );
		ui->txtClientStreetAddress->setText ( recStrValue ( client, FLD_CLIENT_STREET ) );
		ui->txtClientZipCode->setText ( client->action () != ACTION_ADD ? recStrValue ( client, FLD_CLIENT_ZIP ) : QStringLiteral ( "13520-000" ) );
		ui->dteClientStart->setDate ( client->date ( FLD_CLIENT_STARTDATE ) );
		ui->dteClientEnd->setDate ( client->date ( FLD_CLIENT_ENDDATE ) );
		ui->chkClientActive->setChecked ( client->opt ( FLD_CLIENT_STATUS ) );
		ui->contactsClientPhones->decodePhones ( client->recordStr ( FLD_CLIENT_PHONES ) );
		ui->contactsClientEmails->decodeEmails ( client->recordStr ( FLD_CLIENT_EMAIL ) );
	}
}

clientListItem* MainWindow::getClientItem ( const uint id ) const
{
	if ( id >= 1 )
	{
		const int n ( ui->clientsList->count () );
		clientListItem* client_item ( nullptr );
		for ( int i ( 0 ); i < n; ++i )
		{
			client_item = static_cast<clientListItem*>( ui->clientsList->item ( i ) );
			if ( client_item->dbRecID () == id )
				return client_item;
		}
	}
	return nullptr;
}

void MainWindow::loadClients ( clientListItem* &item )
{
	ui->clientsList->setIgnoreChanges ( true );

	clientListItem* client_item ( nullptr );
	uint id ( VDB ()->getLowestID ( TABLE_CLIENT_ORDER ) );
	const uint lastRec ( VDB ()->getHighestID ( TABLE_CLIENT_ORDER ) );
	const uint lastViewedClient_id ( getLastViewedRecord ( &Client::t_info ) );
	Client client;

	do
	{
		if ( client.readRecord ( id, false ) )
		{
			client_item = new clientListItem;
			client_item->setDBRecID ( id );
			client_item->setRelation ( RLI_CLIENTITEM );
			client_item->setRelatedItem ( RLI_CLIENTPARENT, client_item );
			static_cast<void>( client_item->loadData ( false ) );
			client_item->addToList ( ui->clientsList );
			if ( id == lastViewedClient_id )
				item = client_item;
		}
	} while ( ++id <= lastRec );

	ui->clientsList->setEnableSorting ( true );
	ui->clientsList->sortItems ( 0 );
	ui->clientsList->setIgnoreChanges ( false );
}

/* When getXXXItem returns a nullptr, it probably means the item has not yet been loaded. This function will load
 * all the job items for a client when a job item is requested. It will load all payments and jobs when a payment is
 * requested, and it will load all purchases and jobs when a purchase is requested. Therefore I can maintain the logic in
 * fillAllLists () and fillJobBuyList ().
*/
dbListItem* MainWindow::loadItem ( const clientListItem* client_parent, const uint id, const uint table_id )
{
	dbListItem* ret_item ( nullptr );
	if ( client_parent )
	{
		Job job;
		jobListItem* new_job_item ( nullptr );
		if ( job.readFirstRecord ( FLD_JOB_CLIENTID, QString::number ( client_parent->dbRecID () ), false ) )
		{
			do
			{
				new_job_item = new jobListItem;
				new_job_item->setDBRecID ( static_cast<uint>(job.actualRecordInt ( FLD_JOB_ID )) );
				new_job_item->setRelation ( RLI_CLIENTITEM );
				new_job_item->setRelatedItem ( RLI_CLIENTPARENT, static_cast<dbListItem*>( const_cast<clientListItem*>( client_parent ) ) );
				new_job_item->setRelatedItem ( RLI_JOBPARENT, new_job_item );
				new_job_item->setRelatedItem ( RLI_JOBITEM, new_job_item );
				client_parent->jobs->append ( new_job_item );
				static_cast<void>( new_job_item->loadData ( false ) );

				if ( recIntValue ( &job, FLD_JOB_ID ) == static_cast<int>( id ) )
					ret_item = static_cast<dbListItem*>( new_job_item );
			} while ( job.readNextRecord ( true, false ) );
		}

		if ( table_id == PAYMENT_TABLE )
		{
			Payment pay;
			payListItem* new_pay_item ( nullptr );
			jobListItem* job_item ( nullptr );
			if ( pay.readFirstRecord ( FLD_PAY_CLIENTID,  QString::number ( client_parent->dbRecID () ), false ) )
			{
				do
				{
					new_pay_item = new payListItem;
					new_pay_item->setDBRecID ( static_cast<uint>(pay.actualRecordInt ( FLD_PAY_ID )) );
					new_pay_item->setRelation ( RLI_CLIENTITEM );
					new_pay_item->setRelatedItem ( RLI_CLIENTPARENT, static_cast<dbListItem*>( const_cast<clientListItem*>( client_parent ) ) );
					job_item = getJobItem ( client_parent, static_cast<uint>(recIntValue ( &pay, FLD_PAY_JOBID )) );
					if ( job_item ) // there are some errors in the database data. Some records are orphans
					{
						job_item->setPayItem ( new_pay_item );
						new_pay_item->setRelatedItem ( RLI_JOBPARENT, job_item );
						new_pay_item->setRelatedItem ( RLI_JOBITEM, new_pay_item );
					}
					client_parent->pays->append ( new_pay_item );
					static_cast<void>( new_pay_item->loadData ( false ) );

					if ( recIntValue ( &pay, FLD_PAY_ID ) == static_cast<int>( id ) )
						ret_item = static_cast<dbListItem*>( new_pay_item );
				} while ( pay.readNextRecord ( true, false ) );
			}
		}
		else if ( table_id == PURCHASE_TABLE )
		{
			Buy buy;
			buyListItem* new_buy_item ( nullptr );
			buyListItem* new_buy_item2 ( nullptr );
			jobListItem* job_item ( nullptr );

			if ( buy.readFirstRecord ( FLD_BUY_CLIENTID, QString::number ( client_parent->dbRecID () ), false ) )
			{
				do
				{
					new_buy_item = new buyListItem;
					new_buy_item->setDBRecID ( static_cast<uint>(buy.actualRecordInt ( FLD_BUY_ID )) );
					new_buy_item->setRelation ( RLI_CLIENTITEM );
					new_buy_item->setRelatedItem ( RLI_CLIENTPARENT, static_cast<dbListItem*>( const_cast<clientListItem*>( client_parent ) ) );
					client_parent->buys->append ( new_buy_item );

					new_buy_item2 = new buyListItem;
					new_buy_item2->setRelation ( RLI_JOBITEM );
					new_buy_item->syncSiblingWithThis ( new_buy_item2 );

					job_item = getJobItem ( client_parent, static_cast<uint>(recIntValue ( &buy, FLD_BUY_JOBID )) );
					if ( job_item ) // there are some errors in the database data. Some records are orphans
					{
						new_buy_item->setRelatedItem ( RLI_JOBPARENT, job_item );
						new_buy_item2->setRelatedItem ( RLI_JOBPARENT, job_item );
						job_item->buys->append ( new_buy_item2 );
					}
					static_cast<void>( new_buy_item->loadData ( false ) );
					if ( recIntValue ( &buy, FLD_BUY_ID ) == static_cast<int>( id ) )
						ret_item = static_cast<dbListItem*>( new_buy_item );

				} while ( buy.readNextRecord ( true, false ) );
			}
		}
		return ret_item;
	}
	return nullptr;
}

void MainWindow::fillAllLists ( const clientListItem* client_item, jobListItem* &job_item,
								buyListItem* &buy_item, const uint jobid, const uint buyid )
{
	// clears the list but does not delete the items
	ui->jobsList->clear (); 
	ui->paysList->clear ();
	ui->buysList->clear ();

	uint i ( 0 );
	if ( client_item )
	{
		jobListItem* new_job_item ( nullptr );
		const uint lastViewedJob_id ( getLastViewedRecord ( &Job::t_info, client_item->dbRecID () ) );
		if ( client_item->jobs->isEmpty () )
		{
			Job job;
			if ( job.readFirstRecord ( FLD_JOB_CLIENTID, QString::number ( client_item->dbRecID () ), false ) )
			{
				do
				{
					new_job_item = new jobListItem;
					new_job_item->setDBRecID ( static_cast<uint>(job.actualRecordInt ( FLD_JOB_ID )) );
					new_job_item->setRelation ( RLI_CLIENTITEM );
					new_job_item->setRelatedItem ( RLI_CLIENTPARENT, static_cast<dbListItem*>( const_cast<clientListItem*>( client_item ) ) );
					new_job_item->setRelatedItem ( RLI_JOBPARENT, new_job_item );
					new_job_item->setRelatedItem ( RLI_JOBITEM, new_job_item );
					client_item->jobs->append ( new_job_item );
					static_cast<void>( new_job_item->loadData ( false ) );
					new_job_item->addToList ( ui->jobsList );
					if ( new_job_item->dbRecID () == lastViewedJob_id )
					{
						if ( job_item == nullptr )
							job_item = new_job_item;
					}
					if ( jobid == new_job_item->dbRecID () )
						job_item = new_job_item;

				} while ( job.readNextRecord ( true, false ) );
			}
		}
		else
		{
			for ( i = 0; i < client_item->jobs->count (); ++i )
			{
				new_job_item = client_item->jobs->at ( i );
				static_cast<void>( new_job_item->loadData ( false ) );
				new_job_item->addToList ( ui->jobsList );
				if ( new_job_item->dbRecID () == lastViewedJob_id )
				{
					if ( job_item == nullptr )
						job_item = new_job_item;
				}
				if ( jobid == new_job_item->dbRecID () )
					job_item = new_job_item;
			}
		}

		payListItem* pay_item ( nullptr );
		vmNumber totalPays;
		if ( client_item->pays->isEmpty () )
		{
			Payment pay;
			if ( pay.readFirstRecord ( FLD_PAY_CLIENTID,  QString::number ( client_item->dbRecID () ), false ) )
			{
				do
				{
					pay_item = new payListItem;
					pay_item->setDBRecID ( static_cast<uint>(pay.actualRecordInt ( FLD_PAY_ID )) );
					pay_item->setRelation ( RLI_CLIENTITEM );
					pay_item->setRelatedItem ( RLI_CLIENTPARENT, static_cast<dbListItem*>( const_cast<clientListItem*>( client_item ) ) );
					new_job_item = getJobItem ( client_item, static_cast<uint>(recIntValue ( &pay, FLD_PAY_JOBID )) );
					if ( new_job_item ) // there are some errors in the database data. Some records are orphans
					{
						new_job_item->setPayItem ( pay_item );
						pay_item->setRelatedItem ( RLI_JOBPARENT, new_job_item );
						pay_item->setRelatedItem ( RLI_JOBITEM, pay_item );
					}
					client_item->pays->append ( pay_item );
					static_cast<void>( pay_item->loadData ( false ) );
					pay_item->addToList ( ui->paysList );
					totalPays += pay.price ( FLD_PAY_PRICE );
				} while ( pay.readNextRecord ( true, false ) );
			}
		}
		else
		{
			for ( i = 0; i < client_item->pays->count (); ++i )
			{
				pay_item = client_item->pays->at ( i );
				static_cast<void>( pay_item->loadData ( false ) );
				pay_item->addToList ( ui->paysList );
				totalPays += pay_item->payRecord ()->price ( FLD_PAY_PRICE );
			}
		}
		ui->paysList->setProperty ( PROPERTY_TOTAL_PAYS, totalPays.toPrice () );
		ui->txtClientPayTotals->setText ( totalPays.toPrice () );

		buyListItem* new_buy_item ( nullptr );
		const jobListItem* const lastViewedJob ( getJobItem ( client_item, lastViewedJob_id ) );
		const uint lastViewedBuy_id ( getLastViewedRecord ( &Buy::t_info, 0, lastViewedJob ? lastViewedJob->dbRecID () : 0 ) );

		if ( client_item->buys->isEmpty () )
		{
			Buy buy;
			buyListItem* buy_item2 ( nullptr );

			if ( buy.readFirstRecord ( FLD_BUY_CLIENTID, QString::number ( client_item->dbRecID () ), false ) )
			{
				do
				{
					new_buy_item = new buyListItem;
					new_buy_item->setDBRecID ( static_cast<uint>(buy.actualRecordInt ( FLD_BUY_ID )) );
					new_buy_item->setRelation ( RLI_CLIENTITEM );
					new_buy_item->setRelatedItem ( RLI_CLIENTPARENT, static_cast<dbListItem*>( const_cast<clientListItem*>( client_item ) ) );
					client_item->buys->append ( new_buy_item );

					buy_item2 = new buyListItem;
					buy_item2->setRelation ( RLI_JOBITEM );
					new_buy_item->syncSiblingWithThis ( buy_item2 );

					if ( ( new_job_item == nullptr ) ^ ( new_job_item && static_cast<uint>( recIntValue ( &buy, FLD_BUY_JOBID ) ) != new_job_item->dbRecID () ) )
						new_job_item = getJobItem ( client_item, static_cast<uint>(recIntValue ( &buy, FLD_BUY_JOBID )) );
					if ( new_job_item ) // there are some errors in the database data. Some records are orphans
					{
						new_buy_item->setRelatedItem ( RLI_JOBPARENT, new_job_item );
						buy_item2->setRelatedItem ( RLI_JOBPARENT, new_job_item );
						new_job_item->buys->append ( buy_item2 );
					}
					static_cast<void>( new_buy_item->loadData ( false ) );
					new_buy_item->addToList ( ui->buysList );

					if ( new_buy_item->dbRecID () == lastViewedBuy_id )
					{
						if ( buy_item == nullptr )
							buy_item = new_buy_item;
					}
					if ( buyid == new_buy_item->dbRecID () )
						buy_item = new_buy_item;
				} while ( buy.readNextRecord ( true, false ) );
			}
		}
		else
		{
			for ( i = 0; i < client_item->buys->count (); ++i )
			{
				new_buy_item = client_item->buys->at ( i );
				static_cast<void>( new_buy_item->loadData ( false ) );
				new_buy_item->addToList ( ui->buysList );

				if ( new_buy_item->dbRecID () == lastViewedBuy_id )
				{
					if ( buy_item == nullptr )
						buy_item = new_buy_item;
				}
				if ( buyid == new_buy_item->dbRecID () )
					buy_item = new_buy_item;
			}
		}
	}

	ui->jobsList->setIgnoreChanges ( false );
	ui->paysList->setIgnoreChanges ( false );
	ui->buysList->setIgnoreChanges ( false );
}

// Update client's end date. This might be the last job for the client, so we mark what could be the last day working for that client
void MainWindow::alterClientEndDate ( const jobListItem* const job_item )
{
	auto client_item ( static_cast<clientListItem*>( job_item->relatedItem ( RLI_CLIENTPARENT ) ) );
	if ( client_item->clientRecord ()->date ( FLD_CLIENT_ENDDATE ) != job_item->jobRecord ()->date ( FLD_JOB_ENDDATE ) )
	{
		if ( client_item == mClientCurItem )
		{
			static_cast<void>( editClient ( client_item ) );
			ui->dteClientEnd->setDate ( vmNumber ( ui->dteJobEnd->date () ), true );
			static_cast<void>( saveClient ( client_item ) );
		}
		else
		{
			Client* client ( client_item->clientRecord () );
			client->setAction ( ACTION_EDIT );
			setRecValue ( client, FLD_CLIENT_ENDDATE, 
					job_item->jobRecord ()->date ( FLD_JOB_ENDDATE ).toDate ( vmNumber::VDF_DB_DATE ) );
			static_cast<void>( client->saveRecord () );
		}
	}
}

void MainWindow::clientExternalChange ( const uint id, const RECORD_ACTION action )
{
	switch ( action )
	{
		case ACTION_EDIT:
		{
			clientListItem* client_parent ( id != mClientCurItem->dbRecID () ? getClientItem ( id ) : mClientCurItem );
			if ( client_parent != nullptr )
			{
				client_parent->clientRecord ()->setRefreshFromDatabase ( true );
				client_parent->clientRecord ()->readRecord ( id );
				client_parent->update ();
				if ( client_parent->dbRecID () == mClientCurItem->dbRecID () )
					displayClient ( mClientCurItem );
			}
		}
		break;	
		case ACTION_ADD:
		{
			clientListItem* new_client_item ( addClient () );
			Client* new_client ( new_client_item->clientRecord () );
			new_client->setRefreshFromDatabase ( true );
			if ( new_client->readRecord ( id ) )
				static_cast<void>( saveClient ( new_client_item, false ) );
			else
				delClient ( new_client_item, false );
		}
		break;
		case ACTION_DEL:
			delClient ( getClientItem ( id ), false );
		break;
		case ACTION_NONE:
		case ACTION_READ:
		case ACTION_REVERT:
		break;
	}
}
//--------------------------------------------CLIENT------------------------------------------------------------

//-------------------------------------EDITING-FINISHED-CLIENT--------------------------------------------------
void MainWindow::txtClientName_textAltered ( const QString& text )
{
	const bool input_ok ( text.isEmpty () ? false : ( Client::clientID ( text ) == 0 ) );
	mClientCurItem->setFieldInputStatus ( FLD_CLIENT_NAME, input_ok, ui->txtClientName );

	if ( input_ok )
	{
		setRecValue ( mClientCurItem->clientRecord (), FLD_CLIENT_NAME, text );
		mClientCurItem->update ();
	}
	else
	{
		if ( !text.isEmpty () )
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Warning" ), TR_FUNC ( "Client name already in use" ) );
	}
	postFieldAlterationActions ( mClientCurItem );
}

void MainWindow::txtClient_textAltered ( const vmWidget* const sender )
{
	setRecValue ( mClientCurItem->clientRecord (), static_cast<uint>( sender->id () ), sender->text () );
	postFieldAlterationActions ( mClientCurItem );
}

void MainWindow::txtClientZipCode_textAltered ( const QString& text )
{
	QString zipcode ( text );
	while ( zipcode.count () < 8 )
		zipcode.append ( CHR_ZERO );
	if ( !text.contains ( CHR_HYPHEN ) )
		zipcode.insert ( zipcode.length () - 3, CHR_HYPHEN );
	ui->txtClientZipCode->setText ( zipcode );

	setRecValue ( mClientCurItem->clientRecord (), FLD_CLIENT_ZIP, zipcode );
	postFieldAlterationActions ( mClientCurItem );
}

void MainWindow::dteClient_dateAltered ( const vmWidget* const sender )
{
	setRecValue ( mClientCurItem->clientRecord (), static_cast<uint>( sender->id () ),
				  static_cast<const vmDateEdit* const>( sender )->date ().toString ( DATE_FORMAT_DB ) );
	postFieldAlterationActions ( mClientCurItem );
}

void MainWindow::contactsClientAdd ( const QString& info, const vmWidget* const sender )
{
	setRecValue ( mClientCurItem->clientRecord (), static_cast<uint>( sender->id () ),
				  stringRecord::joinStringRecords ( recStrValue ( mClientCurItem->clientRecord (), static_cast<uint>( sender->id () ) ), info ) );
	postFieldAlterationActions ( mClientCurItem );
}

void MainWindow::contactsClientDel ( const int idx, const vmWidget* const sender )
{
	if ( idx >= 0 )
	{
		stringRecord info_rec ( recStrValue ( mClientCurItem->clientRecord (), static_cast<uint>( sender->id () ) ) );
		info_rec.removeField ( static_cast<uint>(idx) );
		setRecValue ( mClientCurItem->clientRecord (), static_cast<uint>( sender->id () ), info_rec.toString () );
		postFieldAlterationActions ( mClientCurItem );
	}
}
//-------------------------------------EDITING-FINISHED-CLIENT---------------------------------------------------

//--------------------------------------------JOB------------------------------------------------------------
void MainWindow::saveJobWidget ( vmWidget* widget, const int id )
{
	widget->setID ( id );
	jobWidgetList[id] = widget;
}

void MainWindow::showJobSearchResult ( dbListItem* item, const bool bshow )
{
	if ( item != nullptr  )
	{
		auto job_item ( static_cast<jobListItem*> ( item ) );

		if ( bshow )
			displayClient ( static_cast<clientListItem*> ( item->relatedItem ( RLI_CLIENTPARENT ) ), true, job_item );

		for ( uint i ( 0 ); i < JOB_FIELD_COUNT; ++i )
		{
			if ( job_item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
			{
				if ( i == FLD_JOB_REPORT )
				{
					for ( uint day ( 0 ); day < job_item->searchSubFields ()->count (); ++day )
					{
						if ( job_item->searchSubFields ()->at ( day ) != job_item->searchSubFields ()->defaultValue () )
						{
							static_cast<dbListItem*>( ui->lstJobDayReport->item( static_cast<int>(day) ) )->highlight ( bshow ? vmBlue : vmDefault_Color );
							//jobWidgetList.at ( FLD_JOB_REPORT + job_item->searchSubFields ()->at ( day ) )->highlight ( bshow ? vmBlue : vmDefault_Color, SEARCH_UI ()->searchTerm () );
						}
					}
				}
				else
					jobWidgetList.at ( i )->highlight ( bshow ? vmBlue : vmDefault_Color, SEARCH_UI ()->searchTerm () );
			}
		}
	}
}

void MainWindow::setupJobPanel ()
{
	actJobSelectJob = new QAction ( ICON ( "project-nbr.png" ), emptyString );
	static_cast<void>( connect ( actJobSelectJob, &QAction::triggered, this, [&] () { return btnJobSelect_clicked (); } ) );
	
	static_cast<void>( connect ( ui->btnQuickProject, &QPushButton::clicked, this, [&] () { return on_btnQuickProject_clicked (); } ) );
	static_cast<void>( connect ( ui->btnJobEditProjectPath, static_cast<void (QToolButton::*)( const bool )>(&QToolButton::clicked),
			  this, [&] ( const bool checked ) { ui->txtJobProjectPath->setEditable ( checked ); if ( checked ) ui->txtJobProjectPath->setFocus (); } ) );

	ui->jobsList->setUtilitiesPlaceLayout ( ui->layoutJobsListUtility );
	ui->jobsList->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) { return jobsListWidget_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );
	ui->jobsList->setCallbackForRowActivated ( [&] ( const uint row ) { return insertNavItem ( static_cast<dbListItem*>( ui->jobsList->item ( static_cast<int>( row ) ) ) ); } );
	ui->jobsList->setCallbackForWidgetGettingFocus ( [&] () { ui->lblCurInfoJob->callLabelActivatedFunc (); } );
	ui->lstJobDayReport->setParentLayout ( ui->gLayoutJobExtraInfo );
	ui->lstJobDayReport->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) { return lstJobDayReport_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );

	saveJobWidget ( ui->txtJobID, FLD_JOB_ID );
	saveJobWidget ( ui->cboJobType, FLD_JOB_TYPE );
	COMPLETERS ()->setCompleterForWidget ( ui->cboJobType, CC_JOB_TYPE );
	ui->cboJobType->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return cboJobType_textAltered ( sender ); } );
	ui->cboJobType->setCallbackForIndexChanged ( [] ( const int ) { return; } ); // this call is just to trigger the signal. With this combo box, we do nothing at an index change
	ui->cboJobType->setIgnoreChanges ( false );

	saveJobWidget ( ui->txtJobAddress, FLD_JOB_ADDRESS );
	ui->txtJobAddress->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtJob_textAltered ( sender ); } );

	saveJobWidget ( ui->txtJobProjectID, FLD_JOB_PROJECT_ID );
	ui->txtJobProjectID->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtJob_textAltered ( sender ); } );

	saveJobWidget ( ui->txtJobPrice, FLD_JOB_PRICE );
	ui->txtJobPrice->setTextType ( vmLineEdit::TT_PRICE );
	ui->txtJobPrice->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtJob_textAltered ( sender ); } );

	saveJobWidget ( ui->txtJobProjectPath, FLD_JOB_PROJECT_PATH );
	ui->txtJobProjectPath->setButtonType ( 0, vmLineEditWithButton::LEBT_DIALOG_BUTTON_DIR );
	ui->txtJobProjectPath->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtJobProjectPath_textAltered ( sender ); } );

	saveJobWidget ( ui->txtJobPicturesPath, FLD_JOB_PICTURE_PATH );
	ui->txtJobPicturesPath->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtJob_textAltered ( sender ); } );

	saveJobWidget ( ui->txtJobWheather, FLD_JOB_REPORT + Job::JRF_WHEATHER );
	ui->txtJobWheather->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtJobWheather_textAltered ( sender ); } );

	saveJobWidget ( ui->txtJobTotalDayTime, FLD_JOB_REPORT + Job::JRF_REPORT + 1 );
	ui->txtJobTotalDayTime->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtJobTotalDayTime_textAltered ( sender ); } );

	saveJobWidget ( ui->txtJobReport, FLD_JOB_REPORT + Job::JRF_REPORT );
	ui->txtJobReport->setCallbackForContentsAltered ( [&] ( const vmWidget* const widget ) {
		return updateJobInfo ( static_cast<textEditWithCompleter*>( const_cast<vmWidget*>( widget ) )->currentText (), JILUR_REPORT ); } );
	ui->txtJobReport->setUtilitiesPlaceLayout ( ui->layoutTxtJobUtilities );
	ui->txtJobReport->setCompleter ( COMPLETERS (), CC_ALL_CATEGORIES );

	saveJobWidget ( ui->timeJobStart, FLD_JOB_REPORT + Job::JRF_START_TIME );
	ui->timeJobStart->setCallbackForContentsAltered ( [&] ( const vmWidget* const ) {
		return timeJobStart_timeAltered (); } );

	saveJobWidget ( ui->timeJobEnd, FLD_JOB_REPORT + Job::JRF_END_TIME );
	ui->timeJobEnd->setCallbackForContentsAltered ( [&] ( const vmWidget* const ) {
		return timeJobEnd_timeAltered (); } );

	saveJobWidget ( ui->dteJobStart, FLD_JOB_STARTDATE );
	ui->dteJobStart->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dteJob_dateAltered ( sender ); } );
	saveJobWidget ( ui->dteJobEnd, FLD_JOB_ENDDATE );
	ui->dteJobEnd->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dteJob_dateAltered ( sender ); } );

	saveJobWidget ( ui->txtJobKeyWords->lineControl (), FLD_JOB_KEYWORDS );
	ui->txtJobKeyWords->setButtonType ( 0, vmLineEditWithButton::LEBT_CUSTOM_BUTTOM );
	ui->txtJobKeyWords->setButtonIcon ( 0, ICON ( "browse-controls/add.png" ) );
	ui->txtJobKeyWords->setCallbackForButtonClicked ( 0, [&] () { return jobAddKeyWord ( ui->txtJobKeyWords->text () ); } );
	ui->txtJobKeyWords->lineControl ()->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
		if ( ke->key () == Qt::Key_Enter || ( ke->key () == Qt::Key_Return ) ) return jobAddKeyWord ( ui->txtJobKeyWords->text () ); } );

	ui->txtJobKeyWords->setButtonType ( 1, vmLineEditWithButton::LEBT_CUSTOM_BUTTOM );
	ui->txtJobKeyWords->setButtonIcon ( 1, ICON ( "browse-controls/remove.png" ) );
	ui->txtJobKeyWords->setCallbackForButtonClicked ( 1, [&] () { return ui->lstJobKeyWords->removeRow ( static_cast<uint>( ui->lstJobKeyWords->currentRow () ) ); } );
	ui->lstJobKeyWords->setCallbackForRowRemoved ( [&] ( const uint row ) { return jobRemoveKeyWord ( row ); } );

	static_cast<void>( connect ( ui->btnJobAddDay, &QToolButton::clicked, this,	&MainWindow::btnJobAddDay_clicked ) );
	static_cast<void>( connect ( ui->btnJobEditDay, &QToolButton::clicked, this, [&] () { return btnJobEditDay_clicked (); } ) );
	static_cast<void>( connect ( ui->btnJobDelDay, &QToolButton::clicked, this, [&] () { return btnJobDelDay_clicked (); } ) );
	static_cast<void>( connect ( ui->btnJobCancelDelDay, &QToolButton::clicked, this, [&] () { return btnJobCancelDelDay_clicked (); } ) );
	static_cast<void>( connect ( ui->btnJobPrevDay, &QToolButton::clicked, this, [&] () { return btnJobPrevDay_clicked (); } ) );
	static_cast<void>( connect ( ui->btnJobNextDay, &QToolButton::clicked, this, [&] () { return btnJobNextDay_clicked (); } ) );
	static_cast<void>( connect ( ui->btnJobMachines, &QToolButton::clicked, this, [&] () { return btnJobMachines_clicked (); } ) );
	
	ui->dteJobAddDate->setCallbackForContentsAltered ( [&] ( const vmWidget* const ) { return dteJobAddDate_dateAltered (); } );
	ui->splitterJobDaysInfo->setSizes ( QList<int> () << 
						static_cast<int>( screen_width () / 4 ) <<
						static_cast<int>( screen_width () / 4 ) <<
						static_cast<int>( 2 * screen_width () / 4 ) );
	
	setupJobPictureControl ();
}

void MainWindow::setUpJobButtons ( const QString& path )
{
	ui->btnJobOpenFileFolder->setEnabled ( false );
	ui->btnJobOpenDoc->setEnabled ( false );
	ui->btnJobOpenXls->setEnabled ( false );
	ui->btnJobOpenPdf->setEnabled ( false );
	if ( fileOps::exists ( path ).isOn () )
	{
		ui->btnJobOpenFileFolder->setEnabled ( true );
		const QStringList nameFilters ( QStringList () << QStringLiteral ( ".doc" ) << QStringLiteral ( ".docx" )
											<< QStringLiteral ( ".pdf" ) << QStringLiteral ( ".xls" )
											<< QStringLiteral ( ".xlsx" ) );

		pointersList<fileOps::st_fileInfo*> filesfound;
		filesfound.setAutoDeleteItem ( true );
		fileOps::lsDir ( filesfound, path, nameFilters );

		if ( !filesfound.isEmpty () )
		{
			if ( menuJobDoc != nullptr )
				menuJobDoc->clear ();
			if ( menuJobPdf != nullptr )
				menuJobPdf->clear ();
			if ( menuJobXls != nullptr )
				menuJobXls->clear ();
			if ( subMenuJobPdfView != nullptr )
				subMenuJobPdfView->clear ();

			QString menutext;
			uint i ( 0 );
			do
			{
				menutext = static_cast<QString> ( filesfound.at ( i )->filename );
				if ( menutext.contains ( configOps::projectDocumentFormerExtension () ) )
				{
					ui->btnJobOpenDoc->setEnabled ( true );
					if ( menuJobDoc == nullptr )
					{
						menuJobDoc = new QMenu ( this );
						ui->btnJobOpenDoc->setMenu ( menuJobDoc );
						static_cast<void>( connect ( menuJobDoc, &QMenu::triggered, this, [&] ( QAction* action ) { return jobOpenProjectFile ( action ); } ) );
						static_cast<void>( connect ( ui->btnJobOpenDoc, &QToolButton::clicked, ui->btnJobOpenDoc, [&] ( const bool ) { return ui->btnJobOpenDoc->showMenu (); } ) );
					}
					menuJobDoc->addAction ( new vmAction ( 0, menutext, this ) );
				}
				else if ( menutext.contains ( configOps::projectPDFExtension () ) )
				{
					ui->btnJobOpenPdf->setEnabled ( true );
					if ( menuJobPdf == nullptr )
					{
						menuJobPdf = new QMenu ( this );
						ui->btnJobOpenPdf->setMenu ( menuJobPdf );
						subMenuJobPdfView = new QMenu ( TR_FUNC ( "View" ), this );
						menuJobPdf->addMenu ( subMenuJobPdfView );
						static_cast<void>( connect ( subMenuJobPdfView, &QMenu::triggered, this, [&] ( QAction* action ) { return jobOpenProjectFile ( action ); } ) );
						static_cast<void>( connect ( ui->btnJobOpenPdf, &QToolButton::clicked, ui->btnJobOpenPdf, [&] ( const bool ) { return ui->btnJobOpenPdf->showMenu (); } ) );
					}
					subMenuJobPdfView->addAction ( new vmAction ( 1, menutext, this ) );
				}
				else
				{
					ui->btnJobOpenXls->setEnabled ( true );
					if ( menuJobXls == nullptr )
					{
						menuJobXls = new QMenu ( this );
						ui->btnJobOpenXls->setMenu ( menuJobXls );
						static_cast<void>( connect ( menuJobXls, &QMenu::triggered, this, [&] ( QAction* action ) { return jobOpenProjectFile ( action ); } ) );
						static_cast<void>( connect ( ui->btnJobOpenXls, &QToolButton::clicked, ui->btnJobOpenXls, [&] ( const bool ) { return ui->btnJobOpenXls->showMenu (); } ) );
					}
					menuJobXls->addAction ( new vmAction ( 2, menutext, this ) );
				}
			} while ( ++i < filesfound.count () );
		}
	}
}

void MainWindow::setupJobPictureControl ()
{
	auto menuJobClientsYearPictures ( new QMenu );
	vmAction* action ( nullptr );

	for ( int i ( 2008 ); static_cast<uint>(i) <= vmNumber::currentDate ().year (); ++i )
	{
		action = new vmAction ( i, QString::number ( i ), this );
		action->setCheckable ( true );
		menuJobClientsYearPictures->addAction ( action );
	}
	menuJobClientsYearPictures->addSeparator ();
	action = new vmAction ( 0, TR_FUNC ( "Return to seeing current job's pictures" ), this );
	action->setCheckable ( true );
	action->setChecked ( true );
	menuJobClientsYearPictures->addAction ( action );
	jobsPicturesMenuAction = action;
	jobImageViewer = new imageViewer ( ui->frmJobPictureViewer, ui->layoutFrmJobImageViewer );

	connect ( menuJobClientsYearPictures, &QMenu::triggered, this, [&] ( QAction* act )
		{
			return showClientsYearPictures ( act );
		} );
	
	connect ( ui->btnJobAddPictures, &QToolButton::clicked, this, [&] ()
		{
			return addJobPictures ();
		} );

	ui->btnJobClientsYearPictures->setMenu ( menuJobClientsYearPictures );
	connect ( ui->btnJobClientsYearPictures, &QToolButton::clicked, ui->btnJobClientsYearPictures, [&] ()
		{
			return ui->btnJobClientsYearPictures->showMenu ();
		} );

	connect ( ui->btnJobPrevPicture, &QToolButton::clicked, this, [&] ()
		{
			return jobImageViewer->showPrevImage ();
		} );

	connect ( ui->btnJobNextPicture, &QToolButton::clicked, this, [&] ()
		{
			return jobImageViewer->showNextImage ();
		} );

	connect ( ui->btnJobOpenPictureFolder, &QToolButton::clicked, this, [&] ()
		{
			QStringList args ( QStringList () << ui->txtJobPicturesPath->text () );
			return fileOps::executeFork ( args, CONFIG ()->fileManager () );
		} );

	connect ( ui->btnJobOpenPictureViewer, &QToolButton::clicked, this, [&] ()
		{
			QStringList args ( QStringList () << jobImageViewer->imageCompletePath () );
			return fileOps::executeFork ( args, CONFIG ()->pictureViewer () );
		} );

	connect ( ui->btnJobOpenPictureEditor, &QToolButton::clicked, this, [&] ()
		{
			QStringList args ( QStringList () << jobImageViewer->imageCompletePath () );
			return fileOps::executeFork ( args, CONFIG ()->pictureEditor () );
		} );

	connect ( ui->btnJobOpenFileFolder, &QToolButton::clicked, this, [&] () -> void
		{
			if ( fileOps::exists ( ui->txtJobProjectPath->text () ).isOn () )
			{
				QStringList args ( QStringList () << fileOps::dirFromPath ( ui->txtJobProjectPath->text () ) );
				(void) fileOps::executeFork ( args, CONFIG ()->fileManager () );
			}
		} );

	connect ( ui->btnJobReloadPictures, &QToolButton::clicked, this, [&] ()
		{
			return btnJobReloadPictures_clicked ();
		} );

	connect ( ui->btnJobRenamePicture, static_cast<void (QToolButton::*)(bool)>( &QToolButton::clicked ), this, [&] ( bool checked )
		{
			return btnJobRenamePicture_clicked ( checked );
		} );

	connect ( ui->btnJobSeparatePicture, static_cast<void (QToolButton::*)(bool)>( &QToolButton::clicked ), this, [&] ( bool checked )
		{
			if ( checked )
				showJobImageInWindow ( false );
			else
				sepWin_JobPictures->returnToParent ();
		} );

	connect ( ui->btnJobSeparateReportWindow, static_cast<void (QToolButton::*)(bool)>( &QToolButton::clicked ), this, [&] ( bool checked )
		{
			return btnJobSeparateReportWindow_clicked ( checked );
		} );

	ui->cboJobPictures->setCallbackForIndexChanged ( [&] ( const int idx ) { return jobImageViewer->showSpecificImage ( idx ); } );
	jobImageViewer->setCallbackForshowImageRequested ( [&] ( const int idx ) { return showJobImageRequested ( idx ); } );
	jobImageViewer->setCallbackForshowMaximized ( [&] ( const bool maximized ) { return showJobImageInWindow ( maximized ); } );
}

void MainWindow::displayJobFromCalendar ( dbListItem* cal_item )
{
	auto client_item ( static_cast<clientListItem*>( cal_item->relatedItem ( RLI_CLIENTPARENT ) ) );
	if ( client_item )
	{
		if ( client_item != mClientCurItem )
			displayClient ( client_item, true, static_cast<jobListItem*>( cal_item->relatedItem ( RLI_CALENDARITEM ) ) );
		else
			displayJob ( static_cast<jobListItem*>( cal_item->relatedItem ( RLI_CALENDARITEM ) ), true );

		ui->lstJobDayReport->setCurrentRow ( cal_item->data ( Qt::UserRole ).toInt () - 1, true, true ); // select pertinent day
		// We do not want to simply call displayJob (), we want to redirect the user to view that job. Unlike notifyExternalChange (),
		// which merely has to update the display but the workflow must remain within DB_TABLE_VIEW
		ui->lblCurInfoJob->callLabelActivatedFunc ();
	}
}

void MainWindow::jobsListWidget_currentItemChanged ( dbListItem* item )
{
	displayJob ( static_cast<jobListItem*>( item ) );
}

void MainWindow::lstJobDayReport_currentItemChanged ( vmListItem* item )
{
	if ( item && !item->data ( JILUR_REMOVED ).toBool () )
	{
		if ( ui->lstJobDayReport->prevItem () )
		{
			vmListItem* prevItem ( ui->lstJobDayReport->prevItem () );
			const bool b_prevdayisediting ( prevItem->data ( JILUR_ADDED ).toBool () || prevItem->data ( JILUR_EDITED ).toBool () );
			if ( b_prevdayisediting )
			{
				ui->txtJobReport->saveContents ( true, false );
				updateJobInfo ( ui->txtJobReport->currentText (), JILUR_REPORT, prevItem );
			}
		}

		const vmNumber time1 ( item->data ( JILUR_START_TIME ).toString (), VMNT_TIME, vmNumber::VTF_DAYS );
		ui->timeJobStart->setTime ( time1 );
		vmNumber timeAndDate ( item->data ( JILUR_END_TIME ).toString (), VMNT_TIME, vmNumber::VTF_DAYS );
		ui->timeJobEnd->setTime ( timeAndDate );
		timeAndDate -= time1;
		ui->txtJobTotalDayTime->setText ( timeAndDate.toTime ( vmNumber::VTF_FANCY ) );
		timeAndDate.fromTrustedStrDate ( item->data ( JILUR_DATE ).toString (), vmNumber::VDF_DB_DATE );
		ui->dteJobAddDate->setDate ( timeAndDate );
		showDayPictures ( timeAndDate );
		ui->txtJobWheather->setText ( item->data ( JILUR_WHEATHER ).toString () );
		ui->txtJobReport->setText ( item->data ( JILUR_REPORT ).toString () );

		const triStateType editing_action ( mJobCurItem ? static_cast<TRI_STATE>( mJobCurItem->action () != ACTION_READ ) : TRI_UNDEF );
		const bool b_hasDays ( ( ui->lstJobDayReport->count () > 0 ) && editing_action.isOn () );

		ui->btnJobAddDay->setEnabled ( editing_action.isOn () );
		ui->btnJobEditDay->setEnabled ( b_hasDays && !item->data ( JILUR_EDITED ).toBool () );
		ui->btnJobDelDay->setEnabled ( b_hasDays );
		ui->btnJobCancelDelDay->setEnabled ( b_hasDays ? ui->lstJobDayReport->currentItem ()->data ( JILUR_REMOVED ).toBool () : false );
		
		const bool b_dayisediting ( item->data ( JILUR_ADDED ).toBool () || item->data ( JILUR_EDITED ).toBool () );
		controlJobDayForms ( b_hasDays ? b_dayisediting : false );
	}
	else
	{
		ui->timeJobStart->setTime ( vmNumber () );
		ui->timeJobEnd->setTime ( vmNumber () );
		ui->txtJobTotalDayTime->setText ( emptyString );
		ui->txtJobWheather->setText ( emptyString );
		ui->txtJobReport->textEditWithCompleter::setText ( item ? TR_FUNC ( " - THIS DAY WAS REMOVED - " ) : emptyString );
		ui->btnJobCancelDelDay->setEnabled ( item ? item->data ( JILUR_REMOVED ).toBool () : false );
		controlJobDayForms ( false );
	}
	ui->btnJobNextDay->setEnabled ( ui->lstJobDayReport->currentRow () < (ui->lstJobDayReport->count () - 1) );
	ui->btnJobPrevDay->setEnabled ( ui->lstJobDayReport->currentRow () >= 1 );
}

void MainWindow::controlJobForms ( const jobListItem* const job_item )
{
	const triStateType editing_action ( job_item ? static_cast<TRI_STATE> ( job_item->action () >= ACTION_ADD ) : TRI_UNDEF );

	ui->cboJobType->vmComboBox::setEditable ( editing_action.isOn () );
	ui->txtJobAddress->setEditable ( editing_action.isOn () );
	ui->txtJobPrice->setEditable ( editing_action.isOn () );
	ui->txtJobProjectPath->setEditable ( editing_action.isOn () );
	ui->txtJobProjectID->setEditable ( editing_action.isOn () );
	ui->txtJobPicturesPath->setEditable ( editing_action.isOn () );
	ui->dteJobStart->setEditable ( editing_action.isOn () );
	ui->dteJobEnd->setEditable ( editing_action.isOn () );

	sectionActions[1].b_canAdd = mClientCurItem != nullptr ? mClientCurItem->action() != ACTION_ADD : false;
	sectionActions[1].b_canEdit = editing_action.isOff ();
	sectionActions[1].b_canRemove = editing_action.isOff ();
	sectionActions[1].b_canSave = editing_action.isOn () ? job_item->isGoodToSave () : false;
	sectionActions[1].b_canCancel = editing_action.isOn ();
	updateActionButtonsState ();
	
	ui->btnJobEditProjectPath->setEnabled ( editing_action.isOn () );

	ui->cboJobType->highlight ( editing_action.isOn () ? ( ui->cboJobType->text ().isEmpty () ? vmRed : vmDefault_Color ) : vmDefault_Color );
	ui->dteJobStart->highlight ( editing_action.isOn () ? ( vmNumber ( ui->dteJobStart->date () ).isDateWithinRange ( vmNumber::currentDate (), 0, 4 ) ? vmDefault_Color : vmRed ) : vmDefault_Color );

	if ( jobsPicturesMenuAction != nullptr )
	{
		jobsPicturesMenuAction->setChecked ( false );
		jobsPicturesMenuAction = ui->btnJobClientsYearPictures->menu ()->actions ().last ();
		jobsPicturesMenuAction->setChecked ( true );
	}
	ui->btnQuickProject->setEnabled ( editing_action.isOff () ? true : ( editing_action.isOn () ? job_item->fieldInputStatus ( FLD_JOB_STARTDATE ) : false ) );
	ui->txtJobKeyWords->setEditable ( editing_action.isOn () );
	ui->lstJobKeyWords->setEditable ( editing_action.isOn () );
	
	controlJobDaySection ( job_item );
	controlJobPictureControls ();
}

void MainWindow::controlJobDaySection ( const jobListItem* const job_item )
{
	const triStateType editing_action ( job_item ? static_cast<TRI_STATE>( job_item->action () != ACTION_READ ) : TRI_UNDEF );
	const vmListItem* const day_item ( ui->lstJobDayReport->currentItem () );
	const bool b_editingDay ( editing_action.isOn () ? day_item != nullptr : false );
	
	ui->btnJobAddDay->setEnabled ( editing_action.isOn () );
	ui->btnJobEditDay->setEnabled ( b_editingDay ? !day_item->data ( JILUR_EDITED ).toBool () && !day_item->data ( JILUR_ADDED ).toBool () : false );
	ui->btnJobDelDay->setEnabled ( b_editingDay ? !day_item->data ( JILUR_REMOVED ).toBool () : false );
	ui->btnJobCancelDelDay->setEnabled ( b_editingDay ? day_item->data ( JILUR_REMOVED ).toBool () : false );
	ui->btnJobSeparateReportWindow->setEnabled ( job_item != nullptr );
	
	ui->btnJobPrevDay->setEnabled ( ui->lstJobDayReport->currentRow () >= 1 );
	ui->btnJobNextDay->setEnabled ( ui->lstJobDayReport->currentRow () < ( ui->lstJobDayReport->count () - 1 ) );
	//lstJobDayReport_currentItemChanged ( static_cast<dbListItem*>( ui->lstJobDayReport->currentItem () ) );
}

void MainWindow::controlJobDayForms ( const bool b_editable )
{
	ui->dteJobAddDate->setEditable ( b_editable );
	ui->timeJobStart->setEditable ( b_editable );
	ui->timeJobEnd->setEditable ( b_editable);
	ui->txtJobWheather->setEditable ( b_editable );
	ui->txtJobTotalDayTime->setEditable ( b_editable );
	ui->txtJobReport->setEditable ( b_editable );
}

void MainWindow::controlJobPictureControls ()
{
	const bool b_hasPictures ( ui->cboJobPictures->count () > 0 );
	ui->btnJobNextPicture->setEnabled ( b_hasPictures );
	ui->btnJobPrevPicture->setEnabled ( ui->cboJobPictures->currentIndex () > 0 );
	ui->btnJobOpenPictureEditor->setEnabled ( b_hasPictures );
	ui->btnJobOpenPictureFolder->setEnabled ( b_hasPictures );
	ui->btnJobOpenPictureViewer->setEnabled ( b_hasPictures );
	ui->btnJobReloadPictures->setEnabled ( !ui->txtJobPicturesPath->text ().isEmpty () );
	ui->btnJobRenamePicture->setEnabled ( b_hasPictures );
	ui->btnJobClientsYearPictures->setEnabled ( mJobCurItem != nullptr );
	ui->btnJobSeparatePicture->setEnabled ( b_hasPictures );
	const QString n_pics ( QString::number ( ui->cboJobPictures->count () ) + TR_FUNC ( " pictures" ) );
	ui->lblJobPicturesCount->setText ( ui->cboJobPictures->count () > 1 ? n_pics : n_pics.left ( n_pics.count () - 1 ) );
}

bool MainWindow::saveJob ( jobListItem* job_item, const bool b_dbsave )
{
	if ( job_item )
	{
		ui->txtJobReport->saveContents ( true, true ); // force committing the newest text to the buffers. Avoid depending on Qt's signals, which might occur later than when reaching here
		Job* job ( job_item->jobRecord () );
		if ( job->saveRecord ( false, b_dbsave ) ) // do not change indexes just now. Wait for dbCalendar actions
		{
			if ( job_item->action () == ACTION_ADD )
			{
				jobImageViewer->setID ( recIntValue ( job_item->jobRecord (), FLD_JOB_ID ) );
				saveJobPayment ( job_item );
			}
			else
			{
				if ( job->wasModified ( FLD_JOB_PRICE ) )
					alterJobPayPrice ( job_item );
				if ( job->wasModified ( FLD_JOB_ENDDATE ) )
					alterClientEndDate ( job_item );
			}
			mCal->updateCalendarWithJobInfo ( job );
			job_item->setAction ( ACTION_READ, true ); // now we can change indexes
			
			rescanJobDaysList ( job_item );
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Record saved" ), TR_FUNC ( "Job data saved!" ) );
			controlJobForms ( job_item );
			//When the job list is empty, we cannot add a purchase. Now that we've saved a job, might be we added one too, so now we can add a buy
			sectionActions[3].b_canAdd = true;
			changeNavLabels ();
			removeEditItem ( static_cast<dbListItem*>( job_item ) );
			return true;
		}
	}
	return false;
}

jobListItem* MainWindow::addJob ( clientListItem* parent_client )
{
	auto job_item ( new jobListItem );
	job_item->setRelation ( RLI_CLIENTITEM );
	job_item->setRelatedItem ( RLI_CLIENTPARENT, static_cast<dbListItem*>( parent_client ) );
	job_item->setRelatedItem ( RLI_JOBPARENT, job_item );
	static_cast<void>( job_item->loadData ( false ) );
	job_item->setAction ( ACTION_ADD, true );
	job_item->addToList ( ui->jobsList );
	parent_client->jobs->append ( job_item );
	insertEditItem ( static_cast<dbListItem*>( job_item ) );
	setRecValue ( job_item->jobRecord (), FLD_JOB_CLIENTID, recStrValue ( parent_client->clientRecord (), FLD_CLIENT_ID ) );
	job_item->setDBRecID ( static_cast<uint>( recIntValue ( job_item->jobRecord (), FLD_JOB_ID ) ) );
	addJobPayment ( job_item );
	ui->cboJobType->setFocus ();
	return job_item;
}

bool MainWindow::editJob ( jobListItem* job_item, const bool b_dogui )
{
	if ( job_item )
	{
		if ( job_item->action () == ACTION_READ )
		{
			job_item->setAction ( ACTION_EDIT, true );
			insertEditItem ( static_cast<dbListItem*>( job_item ) );
			if ( b_dogui )
			{
				NOTIFY ()->notifyMessage ( TR_FUNC ( "Editing job record" ), recStrValue ( job_item->jobRecord (), FLD_JOB_TYPE ) );
				controlJobForms ( job_item );
				ui->txtJobPrice->setFocus ();
			}
			return true;
		}
	}
	return false;
}

bool MainWindow::delJob ( jobListItem* job_item, const bool b_ask )
{
	if ( job_item )
	{
		if ( b_ask )
		{
			const QString text ( TR_FUNC ( "Are you sure you want to remove job %1?" ) );
			if ( !NOTIFY ()->questionBox ( TR_FUNC ( "Question" ), text.arg ( recStrValue ( job_item->jobRecord (), FLD_JOB_TYPE ) ) ) )
				return false;
		}
		job_item->setAction ( ACTION_DEL, true );
		mCal->updateCalendarWithJobInfo ( job_item->jobRecord () ); // remove job info from calendar and clear job days list
		return cancelJob ( job_item );
	}
	return false;
}

bool MainWindow::cancelJob ( jobListItem* job_item )
{
	if ( job_item )
	{
		switch ( job_item->action () )
		{
			case ACTION_ADD:
			case ACTION_DEL:
			{
				removeJobPayment ( job_item->payItem () );
				mJobCurItem = nullptr;
				static_cast<clientListItem*>( job_item->relatedItem ( RLI_CLIENTPARENT ) )->jobs->remove ( job_item->row () );
				removeListItem ( job_item );
			}
			break;
			case ACTION_EDIT:
				removeEditItem ( static_cast<dbListItem*>( job_item ) );
				job_item->setAction ( ACTION_REVERT, true );
				fixJobDaysList ( job_item );
				displayJob ( job_item );
			break;
			case ACTION_NONE:
			case ACTION_READ:
			case ACTION_REVERT:
				return false;
		}
		return true;
	}
	return false;
}

void MainWindow::displayJob ( jobListItem* job_item, const bool b_select, buyListItem* buy_item )
{
	if ( job_item != nullptr )
	{
		//job_item might be a calendar item, one of the extra items. We need to use the main item
		jobListItem* parent_job ( static_cast<jobListItem*>( job_item->relatedItem ( RLI_JOBPARENT ) ) );
		if ( !parent_job ) // if this happens, something is definetely off. Should not happen
			parent_job = job_item;
		if ( parent_job->loadData ( true ) )
		{
			if ( parent_job != mJobCurItem )
			{
				if ( b_select )
					ui->jobsList->setCurrentItem ( parent_job, true, false );
				alterLastViewedRecord ( mJobCurItem ? ( mJobCurItem->relatedItem ( RLI_CLIENTPARENT ) == parent_job->relatedItem ( RLI_CLIENTPARENT ) ?
											mJobCurItem : nullptr ) : nullptr, parent_job );
				mJobCurItem = parent_job;
				displayPay ( parent_job->payItem (), true );
				fillJobBuyList ( parent_job );
			}
			loadJobInfo ( parent_job->jobRecord () );
			controlJobForms ( parent_job );
			if ( !buy_item )
			{
				if ( !parent_job->buys->isEmpty () )
					buy_item = static_cast<buyListItem*>( parent_job->buys->last ()->relatedItem ( RLI_CLIENTITEM ) );
			}
			displayBuy ( buy_item, true );
		}
	}
	else
	{
		mJobCurItem = nullptr;
		loadJobInfo ( nullptr );
		controlJobForms ( nullptr );
		displayPay ( nullptr );
		fillJobBuyList ( nullptr );
		displayBuy ( nullptr );
	}
	if ( b_select )
		changeNavLabels ();
}

void MainWindow::loadJobInfo ( const Job* const job )
{
	if ( job )
	{
		ui->txtJobID->setText ( recStrValue ( job, FLD_JOB_ID ) );
		ui->cboJobType->setText ( recStrValue ( job, FLD_JOB_TYPE ) );
		ui->txtJobAddress->setText ( Job::jobAddress ( job, mClientCurItem->clientRecord () ) );
		ui->txtJobPrice->setText ( recStrValue ( job, FLD_JOB_PRICE ) );
		ui->txtJobProjectID->setText ( recStrValue ( job, FLD_JOB_PROJECT_ID ) );
		ui->dteJobStart->setDate ( job->date ( FLD_JOB_STARTDATE ) );
		ui->dteJobEnd->setDate ( job->date ( FLD_JOB_ENDDATE ) );
		ui->txtJobTotalAllDaysTime->setText ( job->time ( FLD_JOB_TIME ).toTime ( vmNumber::VTF_FANCY ) );
		ui->txtJobProjectPath->setText ( CONFIG ()->projectsBaseDir () + recStrValue ( job, FLD_JOB_PROJECT_PATH ) );
		ui->txtJobPicturesPath->setText ( CONFIG ()->projectsBaseDir () + recStrValue ( job, FLD_JOB_PICTURE_PATH ) );
		jobImageViewer->prepareToShowImages ( recIntValue ( job, FLD_JOB_ID ), ui->txtJobPicturesPath->text () );
	}
	else
	{
		ui->txtJobID->clear ();
		ui->cboJobType->clearEditText ();
		ui->txtJobAddress->clear ();
		ui->txtJobPrice->setText ( vmNumber::zeroedPrice.toPrice () );
		ui->txtJobProjectPath->lineControl ()->clear ();
		ui->txtJobPicturesPath->clear ();
		ui->txtJobProjectID->clear ();
		jobImageViewer->prepareToShowImages ( -1, emptyString );
		ui->cboJobPictures->clearEditText ();
		ui->cboJobPictures->clear ();
		ui->dteJobStart->setDate ( vmNumber () );
		ui->dteJobEnd->setDate ( vmNumber () );
		ui->txtJobTotalAllDaysTime->clear ();
	}
	scanJobImages ();
	setUpJobButtons ( ui->txtJobProjectPath->text () );
	decodeJobReportInfo ( job );
	fillJobKeyWordsList ( job );
}

jobListItem* MainWindow::getJobItem ( const clientListItem* const parent_client, const uint id ) const
{
	if ( id >= 1 && parent_client )
	{
		int i ( 0 );
		while ( i < static_cast<int>( parent_client->jobs->count () ) )
		{
			if ( parent_client->jobs->at ( i )->dbRecID () == id )
				return parent_client->jobs->at ( i );
			++i;
		}
	}
	return nullptr;
}

void MainWindow::scanJobImages ()
{
	ui->cboJobPictures->setIgnoreChanges ( true );
	ui->cboJobPictures->clear ();
	jobImageViewer->addImagesList ( ui->cboJobPictures );
	if ( ui->cboJobPictures->count () == 0 )
	{
		const auto year ( static_cast<uint>( vmNumber::currentDate ().year () ) );
		const QString picturePath ( CONFIG ()->getProjectBasePath ( ui->txtClientName->text () ) + jobPicturesSubDir );
		fileOps::createDir ( picturePath );
		QMenu* yearsMenu ( ui->btnJobClientsYearPictures->menu () );
		for ( uint i ( 0 ), y ( 2008 ); y <= year; ++i, ++y )
		{
			yearsMenu->actions ().at ( static_cast<int>( i ) )->setEnabled (
				fileOps::isDir ( picturePath + QString::number ( y ) ).isOn () );
		}
	}
	ui->cboJobPictures->setEditText ( jobImageViewer->imageFileName () );
	controlJobPictureControls ();
	ui->cboJobPictures->setIgnoreChanges ( false );
}

void MainWindow::decodeJobReportInfo ( const Job* const job )
{
	ui->lstJobDayReport->clear ();
	ui->dteJobAddDate->setDate ( vmNumber () );
	ui->txtJobReport->clear ();
	ui->txtJobTotalDayTime->clear ();
	ui->txtJobWheather->clear ();
	ui->timeJobStart->setTime ( vmNumber () );
	ui->timeJobEnd->setTime ( vmNumber () );

	if ( job != nullptr )
	{
		if ( job->jobItem ()->daysList->isEmpty () )
		{
			const stringTable& str_report ( recStrValue ( job, FLD_JOB_REPORT ) );
			const stringRecord* rec ( nullptr );

			rec = &str_report.first ();
			if ( rec->isOK () )
			{
				QString str_date;
				vmListItem* day_entry ( nullptr );
				uint day ( 1 ), jilur_idx ( 0 );
				vmNumber totalDayTime;
				do
				{
					if ( rec->first () )
					{
						str_date = vmNumber ( rec->curValue (), VMNT_DATE, vmNumber::VDF_DB_DATE ).toDate ( vmNumber::VDF_HUMAN_DATE );
						day_entry = new vmListItem ( QString::number ( day++ ) + lstJobReportItemPrefix + str_date ); //any type id
						day_entry->setData ( JILUR_DATE, rec->curValue () );
						jilur_idx = JILUR_START_TIME;
	
						while ( rec->next () )
						{
							switch ( jilur_idx )
							{
								case JILUR_START_TIME:
									totalDayTime.fromTrustedStrTime ( rec->curValue (), vmNumber::VTF_DAYS );
									totalDayTime.makeOpposite ();
									day_entry->setData ( JILUR_START_TIME, rec->curValue () );
								break;
								case JILUR_END_TIME:
									day_entry->setData ( JILUR_END_TIME, rec->curValue () );
									totalDayTime += vmNumber ( rec->curValue (), VMNT_TIME, vmNumber::VTF_DAYS );
									day_entry->setData ( JILUR_DAY_TIME, totalDayTime.toTime () );
								break;
								case JILUR_WHEATHER:
									day_entry->setData ( JILUR_WHEATHER, rec->curValue () );
								break;
								case JILUR_REPORT:
									day_entry->setData ( JILUR_REPORT, rec->curValue () );
								break;
							}
							if ( ++jilur_idx > JILUR_REPORT )
								break; // it should never reach this statement, but I include it as a failsafe measure
						}
						day_entry->setData ( JILUR_ADDED, false );
						day_entry->setData ( JILUR_REMOVED, false );
						day_entry->setData ( JILUR_EDITED, false );
						ui->lstJobDayReport->addItem ( day_entry );
						job->jobItem ()->daysList->append ( day_entry );
					}
					rec = &str_report.next ();
				} while ( rec->isOK () );
			}
		}
		else
		{
			for ( uint i ( 0 ); i < job->jobItem ()->daysList->count (); ++i )
				ui->lstJobDayReport->addItem ( job->jobItem ()->daysList->at ( i ) );
		}
		ui->lstJobDayReport->setIgnoreChanges ( false );
		ui->lstJobDayReport->setCurrentRow ( ui->lstJobDayReport->count () - 1, true, true );
	}
}

void MainWindow::fillJobKeyWordsList ( const Job* const job )
{
	ui->txtJobKeyWords->lineControl ()->clear ();
	ui->lstJobKeyWords->clear ( true, true );
	if ( job != nullptr )
	{
		const stringRecord& rec ( recStrValue ( job, FLD_JOB_KEYWORDS ) );
		if ( rec.first () )
		{
			do
			{
				ui->lstJobKeyWords->addItem ( new dbListItem ( rec.curValue () ) );
			} while ( rec.next () );
		}
	}
}

void MainWindow::fixJobDaysList ( jobListItem* const job_item )
{
	pointersList<vmListItem*>* daysList ( job_item->daysList );
	vmListItem* day_item ( daysList->last () );
	if ( day_item != nullptr )
	{
		int day ( day_item->row () );
		do
		{
			if ( day_item->data ( JILUR_ADDED ).toBool () )
				daysList->remove ( day, true );
			if ( day_item->data ( JILUR_EDITED ).toBool () )
				day_item->setData ( JILUR_EDITED, false );
			else
			{
				if ( day_item->data ( JILUR_REMOVED ).toBool () )
					revertDayItem ( day_item );
			}
			--day;
		} while ( (day_item = daysList->prev ()) != nullptr );
	}
}

void MainWindow::revertDayItem ( vmListItem* day_item )
{
	QFont fntStriked ( ui->lstJobDayReport->font () );
	fntStriked.setStrikeOut ( false );
	day_item->setFont ( fntStriked );
	day_item->setBackground ( QBrush ( Qt::white ) );
	day_item->setData ( JILUR_REMOVED, false );
}


void MainWindow::updateJobInfo ( const QString& text, const uint user_role, vmListItem* const item )
{
	auto const day_entry ( item ? item : ui->lstJobDayReport->currentItem () );
	if ( day_entry )
	{
		const auto cur_row ( static_cast<uint>( day_entry->row () ) );
		day_entry->setData ( static_cast<int>( user_role ), text );
		stringTable job_report ( recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_REPORT ) );
		stringRecord rec ( job_report.readRecord ( cur_row ) );
		rec.changeValue ( user_role - Qt::UserRole, text );
		job_report.changeRecord ( cur_row, rec );
		setRecValue ( mJobCurItem->jobRecord (), FLD_JOB_REPORT, job_report.toString () );
		postFieldAlterationActions ( mJobCurItem );
	}
}

void MainWindow::addJobPayment ( jobListItem* const job_item )
{
	auto pay_item ( new payListItem );
	pay_item->setRelation ( RLI_CLIENTITEM );
	pay_item->setRelatedItem ( RLI_CLIENTPARENT, job_item->relatedItem ( RLI_CLIENTPARENT ) );
	pay_item->setRelatedItem ( RLI_JOBPARENT, job_item );
	pay_item->createDBRecord ();
	pay_item->setAction ( ACTION_ADD, true );
	pay_item->addToList ( ui->paysList );
	pay_item->setDBRecID ( static_cast<uint>( recIntValue ( pay_item->payRecord (), FLD_PAY_ID ) ) );
	static_cast<clientListItem*>( job_item->relatedItem ( RLI_CLIENTPARENT ) )->pays->append ( pay_item );
	job_item->setPayItem ( pay_item );
	insertEditItem ( static_cast<dbListItem*>( pay_item ) );
	mPayCurItem = pay_item;
}

void MainWindow::saveJobPayment ( jobListItem* const job_item )
{
	Payment* pay ( job_item->payItem ()->payRecord () );
	
	ui->txtPayTotalPrice->setText ( recStrValue ( job_item->jobRecord (), FLD_JOB_PRICE ), true );
	ui->txtPayID->setText ( recStrValue ( job_item->jobRecord (), FLD_JOB_ID ) );
	
	setRecValue ( pay, FLD_PAY_CLIENTID, recStrValue ( job_item->jobRecord (), FLD_JOB_CLIENTID ) );
	setRecValue ( pay, FLD_PAY_JOBID, recStrValue ( job_item->jobRecord (), FLD_JOB_ID ) );
	
	const vmNumber& price ( pay->price ( FLD_PAY_PRICE ) );
	setRecValue ( pay, FLD_PAY_OVERDUE_VALUE, price.toPrice () );
	setRecValue ( pay, FLD_PAY_OVERDUE, price.isNull () ? CHR_ZERO : CHR_ONE );
	
	if ( pay->saveRecord () )
	{
		job_item->payItem ()->setAction ( pay->action () );
		job_item->payItem ()->setDBRecID ( static_cast<uint>( recIntValue ( pay, FLD_PAY_ID ) ) );
		addPaymentOverdueItems ( job_item->payItem () );
		payOverdueGUIActions ( pay, ACTION_ADD );
		ui->paysList->setCurrentItem ( job_item->payItem (), true, true );
		controlPayForms ( job_item->payItem () ); // enable/disable payment controls for immediate editing if the user wants to
		removeEditItem ( static_cast<dbListItem*>( job_item->payItem () ) );
	}
}

void MainWindow::removeJobPayment ( payListItem* pay_item )
{
	if ( pay_item )
	{
		pay_item->setAction ( ACTION_DEL, true );
		payOverdueGUIActions ( pay_item->payRecord (), ACTION_DEL );
		updateTotalPayValue ( vmNumber::zeroedPrice, pay_item->payRecord ()->price ( FLD_PAY_PRICE ) );
		mCal->updateCalendarWithPayInfo ( pay_item->payRecord () );
		removePaymentOverdueItems ( pay_item );
		mClientCurItem->pays->remove ( ui->paysList->currentRow () );
		mPayCurItem = nullptr;
		removeListItem ( pay_item, false, false );
	}
}

void MainWindow::alterJobPayPrice ( jobListItem* const job_item )
{
	auto pay_item ( static_cast<payListItem*>( job_item->payItem () ) );
	if ( pay_item->payRecord ()->price ( FLD_PAY_PRICE ) != job_item->jobRecord ()->price ( FLD_JOB_PRICE ) )
	{
		if ( pay_item == mPayCurItem )
		{
			static_cast<void>( editPay ( pay_item ) );
			ui->txtPayTotalPrice->setText ( recStrValue ( job_item->jobRecord (), FLD_JOB_PRICE ), true );
			static_cast<void>( savePay ( pay_item ) );
		}
		else
		{
			Payment* pay ( pay_item->payRecord () );
			pay->setAction ( ACTION_EDIT );
			setRecValue ( pay, FLD_PAY_PRICE, recStrValue ( job_item->jobRecord (), FLD_JOB_PRICE ) );
			static_cast<void>( pay->saveRecord () );
		}
	}
}

bool MainWindow::jobRemoveKeyWord ( const uint row )
{
	stringRecord rec ( recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_KEYWORDS ) );
	rec.removeField ( row );
	setRecValue ( mJobCurItem->jobRecord (), FLD_JOB_KEYWORDS, rec.toString () );
	return true;
}

void MainWindow::jobAddKeyWord ( const QString& words )
{
	if ( !words.isEmpty () )
	{
		if ( ui->lstJobKeyWords->getRow ( words, Qt::CaseInsensitive, 0 ) == -1 )
		{
			ui->lstJobKeyWords->addItem ( new dbListItem ( words ) );
			stringRecord rec ( recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_KEYWORDS ) );
			rec.fastAppendValue ( words );
			setRecValue ( mJobCurItem->jobRecord (), FLD_JOB_KEYWORDS, rec.toString () );
			ui->txtJobKeyWords->lineControl ()->setText ( emptyString );
			ui->txtJobKeyWords->lineControl ()->setFocus ();
		}
	}
}

void MainWindow::calcJobTime ()
{
	vmNumber time ( ui->timeJobEnd->time () );
	time -= vmNumber ( ui->timeJobStart->time () );
	ui->txtJobTotalDayTime->setText ( time.toTime ( vmNumber::VTF_FANCY ) ); // Just a visual feedback

	vmNumber total_time;
	for ( int i ( 0 ); i < ui->lstJobDayReport->count (); ++i )
	{
		if ( !ui->lstJobDayReport->item ( i )->data ( JILUR_REMOVED ).toBool () )
		{
			if ( i != ui->lstJobDayReport->currentRow () )
			{
				total_time += vmNumber ( ui->lstJobDayReport->item ( i )->data (
										 JILUR_DAY_TIME ).toString (), VMNT_TIME, vmNumber::VTF_DAYS );
			}
			else
				ui->lstJobDayReport->item ( i )->setData ( JILUR_DAY_TIME, time.toTime () );
		}
	}

	total_time += time;
	mJobCurItem->jobRecord ()->setTime ( FLD_JOB_TIME, total_time );
	// I could ask setText to emit the signal and have saving the time from within the textAltered callback function
	// but then the vmNumber would have to parse the fancy format and it is not worth the trouble nor the wasted cpu cycles
	ui->txtJobTotalAllDaysTime->setText ( total_time.toTime ( vmNumber::VTF_FANCY ) );
	postFieldAlterationActions ( mJobCurItem );
}

triStateType MainWindow::dateIsInDaysList ( const QString& str_date )
{
	vmListItem* item ( nullptr );
	int i ( ui->lstJobDayReport->count () - 1 );
	for ( ; i >= 0 ; --i )
	{
		item = ui->lstJobDayReport->item ( i );
		if ( !item->data ( JILUR_REMOVED ).toBool () )
		{
			if ( str_date == item->data ( JILUR_DATE ).toString () )
				return TRI_ON;
		}
	}
	return ui->lstJobDayReport->isEmpty () ? TRI_UNDEF : TRI_OFF;
}

void MainWindow::rescanJobDaysList ( jobListItem *const job_item )
{
	vmListItem* day_entry ( nullptr );
	bool b_reorder ( false );
	int removedRow ( -1 );
	QString itemText;

	stringTable jobReport ( recStrValue ( job_item->jobRecord (), FLD_JOB_REPORT ) );
	for ( int i ( 0 ); i < ui->lstJobDayReport->count (); )
	{
		day_entry = static_cast<dbListItem*>( ui->lstJobDayReport->item ( i ) );
		if ( day_entry->data ( JILUR_REMOVED ).toBool () )
		{
			ui->lstJobDayReport->removeRow_list ( static_cast<uint>( i ) ); // Let the jobListItem handle the item deletion
			job_item->daysList->remove ( i, true );
			b_reorder = true;
			removedRow = i;
			jobReport.removeRecord ( static_cast<uint>( i ) );
		}
		else
		{
			if ( day_entry->data ( JILUR_ADDED ).toBool () )
				day_entry->setData ( JILUR_ADDED, false );
			if ( day_entry->data ( JILUR_EDITED ).toBool () )
				day_entry->setData ( JILUR_EDITED, false );
			if ( b_reorder )
			{
				itemText = day_entry->data ( Qt::DisplayRole ).toString (); //QListWidgetItem::text () is an inline function that calls data ()
				itemText.remove ( 0, itemText.indexOf ( QLatin1Char ( 'o' ) ) ); // Removes all characters before 'o' from lstJobReportItemPrefix ( "o dia")
				itemText.prepend ( QString::number ( i + 1 ) ); //Starts the new label with item index minus number of all items removed. The +1 is because day counting starts at 1, not 0
				day_entry->setText ( itemText, false, false, false );
			}
			++i; //move to the next item only when we did not remove an item
		}
	}
	if ( removedRow >= 0 )
	{
		if ( removedRow >= ui->lstJobDayReport->count () )
			removedRow = ui->lstJobDayReport->count () - 1;
		else if ( removedRow > 0 )
			removedRow--;
		else
			removedRow = 0;
		ui->lstJobDayReport->setCurrentRow ( removedRow, true, true );
		job_item->jobRecord ()->setAction ( ACTION_EDIT );
		setRecValue ( job_item->jobRecord (), FLD_JOB_REPORT, jobReport.toString () );
		job_item->jobRecord ()->saveRecord ();
	}
}

void MainWindow::controlFormsForJobSelection ( const bool bStartSelection )
{
	sectionActions[1].b_canExtra[0] = bStartSelection;
	updateActionButtonsState ();
	ui->tabMain->widget ( TI_CALENDAR )->setEnabled ( !bStartSelection );
	ui->tabMain->widget ( TI_STATISTICS )->setEnabled ( !bStartSelection );
	ui->tabMain->widget ( TI_TABLEWIEW )->setEnabled ( !bStartSelection );
}

void MainWindow::selectJob ()
{
	controlFormsForJobSelection ( true );
	qApp->setActiveWindow ( this );
	this->setFocus ();
	NOTIFY ()->notifyMessage ( TR_FUNC ( "Select the job you want" ), TR_FUNC ( "Then click on the button at the botton of jobs toolbar, SELECT THIS JOB. Press the ESC key to cancel." ) );
}

void MainWindow::btnJobSelect_clicked ()
{
	if ( selJob_callback && mJobCurItem )
	{
		selJob_callback ( mJobCurItem->dbRecID () );
		selJob_callback = nullptr;
		controlFormsForJobSelection ( false );
	}
}

void MainWindow::btnJobAddDay_clicked ()
{
	lstJobDayReport_currentItemChanged ( nullptr ); // clear forms
	ui->dteJobAddDate->setEditable ( true );
	
	const int n_days ( ui->lstJobDayReport->count () + 1 );
	vmListItem* new_item ( new vmListItem ( QString::number ( n_days ) + lstJobReportItemPrefix ) );
	new_item->setData ( JILUR_ADDED, true );
	new_item->setData ( JILUR_REMOVED, false );
	new_item->setData ( JILUR_EDITED, false );
	mJobCurItem->daysList->append ( new_item );
	new_item->addToList ( ui->lstJobDayReport );
	ui->dteJobAddDate->setFocus ();
}

void MainWindow::btnJobEditDay_clicked ()
{
	static_cast<vmListItem*>( ui->lstJobDayReport->currentItem () )->setData ( JILUR_EDITED, true );
	controlJobDayForms ( true );
	ui->dteJobAddDate->setFocus ();
}

void MainWindow::btnJobDelDay_clicked ()
{
	vmListItem* item ( ui->lstJobDayReport->currentItem () );
	if ( item )
	{
		QFont fntStriked ( ui->lstJobDayReport->font () );
		fntStriked.setStrikeOut ( true );
		item->setFont ( fntStriked );
		item->setBackground ( QBrush ( Qt::red ) );
		item->setData ( JILUR_REMOVED, true );
		ui->btnJobCancelDelDay->setEnabled ( true );
		lstJobDayReport_currentItemChanged ( item ); // visual feedback
		if ( ui->lstJobDayReport->count () > 1 )
		{
			if ( item->row () == 0 && ui->lstJobDayReport->count () > 1 )
				ui->dteJobStart->setDate ( vmNumber ( ui->lstJobDayReport->item ( 1 )->data ( JILUR_DATE ).toString (), VMNT_DATE, vmNumber::VDF_DB_DATE ), true );
			if ( item->row () == ui->lstJobDayReport->rowCount () - 1 )
				ui->dteJobEnd->setDate ( vmNumber ( ui->lstJobDayReport->item ( item->row () - 1 )->data ( JILUR_DATE ).toString (), VMNT_DATE, vmNumber::VDF_DB_DATE ), true );
		}
		// Remove this day's time from total time
		ui->timeJobStart->setTime ( vmNumber::emptyNumber, false );
		ui->timeJobEnd->setTime ( vmNumber::emptyNumber, false );
		calcJobTime ();
		ui->btnJobDelDay->setEnabled ( false );
		ui->btnJobCancelDelDay->setEnabled ( true );
	}
}

void MainWindow::btnJobCancelDelDay_clicked ()
{
	vmListItem* item ( ui->lstJobDayReport->currentItem () );
	if ( item && item->data ( JILUR_REMOVED ).toBool () )
	{
		revertDayItem ( item );
		lstJobDayReport_currentItemChanged ( item ); // visual feedback
		calcJobTime (); // uptate job total time
	}
}

void MainWindow::btnJobPrevDay_clicked ()
{
	int current_row ( ui->lstJobDayReport->currentRow () );
	if ( current_row >= 1 )
	{
		ui->lstJobDayReport->setCurrentRow ( --current_row, true, true );
		ui->btnJobNextDay->setEnabled ( true );
	}
	else
		ui->btnJobPrevDay->setEnabled ( false );
}

void MainWindow::btnJobNextDay_clicked ()
{
	int current_row ( ui->lstJobDayReport->currentRow () );
	if ( current_row < ui->lstJobDayReport->count () - 1 )
	{
		ui->lstJobDayReport->setCurrentRow ( ++current_row, true, true );
		ui->btnJobPrevDay->setEnabled ( true );
	}
	else
		ui->btnJobNextDay->setEnabled ( false );
}

void MainWindow::btnJobMachines_clicked ()
{
	if ( !mMachines )
		mMachines = new machinesDlg;
	mMachines->showJobMachineUse ( recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_ID ) );
}

void MainWindow::jobOpenProjectFile ( QAction* action )
{
	QString program;
	switch ( static_cast<vmAction*> ( action )->id () )
	{
		case 0: // doc(x)
			program = CONFIG ()->docEditor ();
		break;
		case 1: // pdf
			program = CONFIG ()->universalViewer ();
		break;
		case 2: // xls(x)
			program = CONFIG ()->xlsEditor ();
		break;
	}
	// There is a bug(feature) in Qt that inserts an & to automatically create a mnemonic to the menu entry
	// The bug is that text () returns include it
	QStringList args ( ui->txtJobProjectPath->text () + action->text ().remove ( QLatin1Char ( '&' ) ) );
	fileOps::executeFork ( args, program );
}

void MainWindow::quickProjectClosed ()
{
	ui->btnQuickProject->setChecked ( false );
	//if ( mJobCurItem->dbRecID () == QUICK_PROJECT ()->jobID ().toUInt () )
	//{
		const bool bReading ( mJobCurItem->action () == ACTION_READ );
		if ( bReading )
			static_cast<void>( editJob ( mJobCurItem ) );
		ui->txtJobProjectID->setText ( QUICK_PROJECT ()->qpString (), true );
		ui->txtJobPrice->setText ( QUICK_PROJECT ()->totalPrice (), true );
		if ( bReading )
			static_cast<void>( saveJob ( mJobCurItem ) );
	//}
}

void MainWindow::on_btnQuickProject_clicked ()
{
	QUICK_PROJECT ()->setCallbackForDialogClosed ( [&] () { return quickProjectClosed (); } );
	QUICK_PROJECT ()->prepareToShow ( mJobCurItem->jobRecord () );
	QUICK_PROJECT ()->show ();
}

void MainWindow::jobExternalChange ( const uint id, const RECORD_ACTION action )
{
	QScopedPointer<Job> job ( new Job );
	if ( !job->readRecord ( id ) )
		return;
	clientListItem* client_parent ( getClientItem ( static_cast<uint>( recIntValue ( job, FLD_JOB_CLIENTID ) ) ) );
	if ( !client_parent )
		return;
									   
	if ( action == ACTION_EDIT )
	{	
		jobListItem* job_item ( getJobItem ( client_parent, id ) );
		if ( job_item )
		{
			if ( job_item->jobRecord () != nullptr )
				job_item->jobRecord ()->setRefreshFromDatabase ( true ); // if job has been loaded before, tell it to ignore the cache and to load data from the database upon next read
			job_item->update ();
			if ( id == mJobCurItem->dbRecID () )
				displayJob ( job_item );
		}
	}
	else
	{
		if ( static_cast<uint>( recIntValue ( job, FLD_JOB_CLIENTID ) ) != mClientCurItem->dbRecID () )
			displayClient ( client_parent, true );

		if ( action == ACTION_ADD )
		{
			jobListItem* new_job_item ( addJob ( client_parent ) );
			Job* new_job ( new_job_item->jobRecord () );
			new_job->setRefreshFromDatabase ( true );
			new_job->readRecord ( id );
			static_cast<void>( saveJob ( new_job_item, false ) );
		}
		else if ( action == ACTION_DEL )
		{
			delJob ( getJobItem ( client_parent, id ), false );
		}
	}
}
//--------------------------------------------JOB------------------------------------------------------------

//-----------------------------------EDITING-FINISHED-JOB----------------------------------------------------
void MainWindow::cboJobType_textAltered ( const vmWidget* const sender )
{
	// Do not use static_cast<uint>( sender->id () ). Could use sender->parentWidget ()->id () but this method
	// is not shared between objects, only cboJobType uses it
	const bool b_ok ( !sender->text ().isEmpty () );
	mJobCurItem->setFieldInputStatus ( FLD_JOB_TYPE, b_ok, sender->vmParent () );
	if ( b_ok )
	{
		setRecValue ( mJobCurItem->jobRecord (), FLD_JOB_TYPE, sender->text () );
		mJobCurItem->update ();
		postFieldAlterationActions ( mJobCurItem );
	}
}

void MainWindow::txtJobProjectPath_textAltered ( const vmWidget* const )
{
	QString new_path ( ui->txtJobProjectPath->text () );
	new_path.replace ( CONFIG ()->projectsBaseDir (), QString (), Qt::CaseInsensitive );

	if ( new_path.size () < ui->txtJobProjectPath->text ().size () )
	{
		if ( new_path.startsWith ( recStrValue ( mClientCurItem->clientRecord (), FLD_CLIENT_NAME ) + CHR_F_SLASH, Qt::CaseInsensitive ) )
		{
			new_path.append ( CHR_F_SLASH );
			ui->txtJobProjectPath->setText ( new_path );
			setRecValue ( mJobCurItem->jobRecord (), FLD_JOB_PROJECT_PATH, new_path );
			ui->txtJobPicturesPath->setText ( new_path + jobPicturesSubDir );
			//postFieldAlterationActions ( mJobCurItem );
			return;
		}
	}

	NOTIFY ()->notifyMessage ( TR_FUNC ( "Wrong path!" ), TR_FUNC ( "You must choose an existing directory under this client's project directory" ) );
	ui->txtJobProjectPath->setText ( CONFIG ()->projectsBaseDir () + recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_PROJECT_PATH ) );
}

void MainWindow::txtJob_textAltered ( const vmWidget* const sender )
{
	if ( sender->id () == FLD_JOB_PICTURE_PATH )
		btnJobReloadPictures_clicked ();
	setRecValue ( mJobCurItem->jobRecord (), static_cast<uint>( sender->id () ), sender->text () );
	postFieldAlterationActions ( mJobCurItem );
}

void MainWindow::txtJobWheather_textAltered ( const vmWidget* const sender )
{
	updateJobInfo ( sender->text (), JILUR_WHEATHER );
}

void MainWindow::txtJobTotalDayTime_textAltered ( const vmWidget* const sender )
{
	const vmNumber time ( sender->text (), VMNT_TIME );
	if ( time.isTime () ) {
		if ( ui->lstJobDayReport->count () == 1 )
		{
			ui->txtJobTotalAllDaysTime->setText ( sender->text () );
			setRecValue ( mJobCurItem->jobRecord (), static_cast<uint>( sender->id () ), sender->text () );
			postFieldAlterationActions ( mJobCurItem );
		}
		else
		{
			ui->timeJobStart->setTime ( vmNumber () );
			ui->timeJobEnd->setTime ( time, true );
		}
	}
}

void MainWindow::timeJobStart_timeAltered ()
{
	calcJobTime ();
	updateJobInfo ( vmNumber ( ui->timeJobStart->time () ).toTime (), JILUR_START_TIME ); // save time for the day
}

void MainWindow::timeJobEnd_timeAltered ()
{
	calcJobTime ();
	updateJobInfo ( vmNumber ( ui->timeJobEnd->time () ).toTime (), JILUR_END_TIME );
}

void MainWindow::dteJob_dateAltered ( const vmWidget* const sender )
{
	const vmNumber date ( static_cast<const vmDateEdit* const>( sender )->date () );
	const bool input_ok ( date.isDateWithinRange ( vmNumber::currentDate (), 0, 4 ) );
	//if ( input_ok )
		setRecValue ( mJobCurItem->jobRecord (), static_cast<uint>( sender->id () ), date.toDate ( vmNumber::VDF_DB_DATE ) );
	
	if ( static_cast<uint>( sender->id () ) == FLD_JOB_STARTDATE )
	{
		mJobCurItem->setFieldInputStatus ( FLD_JOB_STARTDATE, input_ok, sender );
		ui->btnQuickProject->setEnabled ( input_ok );
	}
	postFieldAlterationActions ( mJobCurItem );
}

void MainWindow::dteJobAddDate_dateAltered ()
{
	const vmNumber vmdate ( ui->dteJobAddDate->date () );
	if ( vmdate.isDateWithinRange ( vmNumber::currentDate (), 0, 3 ) )
	{
		const triStateType date_is_in_list ( dateIsInDaysList ( vmdate.toDate ( vmNumber::VDF_DB_DATE ) ) );
		controlJobDayForms ( date_is_in_list.isOff () );
	
		if ( date_is_in_list.isOff () )
		{
			auto day_entry ( static_cast<dbListItem*>( ui->lstJobDayReport->currentItem () ) );
			if ( day_entry != nullptr )
			{
				ui->btnJobEditDay->setEnabled ( !day_entry->data ( JILUR_EDITED ).toBool () && !day_entry->data ( JILUR_ADDED ).toBool () );
				day_entry->setText ( QString::number ( ui->lstJobDayReport->currentRow () + 1 ) + lstJobReportItemPrefix + vmdate.toDate ( vmNumber::VDF_HUMAN_DATE ), false, false, false );
				updateJobInfo ( vmdate.toDate ( vmNumber::VDF_DB_DATE ), JILUR_DATE, day_entry );

				if ( day_entry->row () == 0 ) // first day
					ui->dteJobStart->setDate ( vmdate, true );
				if ( day_entry->row () == ui->lstJobDayReport->count () - 1 )
					ui->dteJobEnd->setDate ( vmdate, true );
				
				ui->timeJobStart->setFocus ();
			}
		}
	}
}
//-----------------------------------EDITING-FINISHED-JOB----------------------------------------------------

//--------------------------------------------PAY------------------------------------------------------------
void MainWindow::savePayWidget ( vmWidget* widget, const int id )
{
	widget->setID ( id );
	payWidgetList[id] = widget;
}

void MainWindow::showPaySearchResult ( dbListItem* item, const bool bshow )
{
	auto pay_item ( static_cast<payListItem*>( item ) );
	if ( bshow ) {
		displayClient ( static_cast<clientListItem*>( item->relatedItem ( RLI_CLIENTPARENT ) ), true,
						static_cast<jobListItem*>( item->relatedItem ( RLI_JOBPARENT ) ) );
		displayPay ( pay_item, true );
	}
	for ( uint i ( 0 ); i < PAY_FIELD_COUNT; ++i )
	{
		if ( pay_item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
		{
			if ( i != FLD_PAY_INFO )
				payWidgetList.at ( i )->highlight ( bshow ? vmBlue : vmDefault_Color, SEARCH_UI ()->searchTerm () );
			else
			{
				if ( bshow )
					ui->tablePayments->searchStart ( SEARCH_UI ()->searchTerm () );
				else
					ui->tablePayments->searchCancel ();
			}
		}
	}
}

void MainWindow::setupPayPanel ()
{
	savePayWidget ( ui->txtPayID, FLD_PAY_ID );
	savePayWidget ( ui->txtPayTotalPrice, FLD_PAY_PRICE );
	ui->txtPayTotalPrice->setTextType ( vmLineEdit::TT_PRICE );
	ui->txtPayTotalPrice->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtPayTotalPrice_textAltered ( sender->text () ); } );

	savePayWidget ( ui->txtPayTotalPaid, FLD_PAY_TOTALPAID );
	ui->txtPayTotalPaid->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtPay_textAltered ( sender ); } );

	savePayWidget ( ui->txtPayObs, FLD_PAY_OBS );
	ui->txtPayObs->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtPay_textAltered ( sender ); } );

	ui->chkPayOverdue->setLabel ( TR_FUNC ( "Ignore if overdue" ) );
	ui->chkPayOverdue->setCallbackForContentsAltered ( [&] ( const bool checked ) {
		return chkPayOverdue_toggled ( checked ); } );

	ui->paysList->setAlwaysEmitCurrentItemChanged ( true );
	ui->paysList->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) { return paysListWidget_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );
	ui->paysList->setCallbackForRowActivated ( [&] ( const uint row ) { return insertNavItem ( static_cast<dbListItem*>( ui->paysList->item ( static_cast<int>( row ) ) ) ); } );
	ui->paysList->setUtilitiesPlaceLayout ( ui->layoutClientPaysUtility );
	ui->paysList->setCallbackForWidgetGettingFocus ( [&] () { ui->lblCurInfoPay->callLabelActivatedFunc (); } );

	ui->paysOverdueList->setAlwaysEmitCurrentItemChanged ( true );
	ui->paysOverdueList->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) { return paysOverdueListWidget_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );
	ui->paysOverdueList->setCallbackForRowActivated ( [&] ( const uint row ) { return insertNavItem ( static_cast<dbListItem*>(	ui->paysOverdueList->item ( static_cast<int>( row ) ) )->relatedItem ( RLI_CLIENTITEM ) ); } );
	ui->paysOverdueList->setUtilitiesPlaceLayout ( ui->layoutOverduePaysUtility );
	ui->paysOverdueList->setCallbackForWidgetGettingFocus ( [&] () { ui->lblCurInfoPay->callLabelActivatedFunc (); } );

	ui->paysOverdueClientList->setAlwaysEmitCurrentItemChanged ( true );
	ui->paysOverdueClientList->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) { return paysOverdueClientListWidget_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );
	ui->paysOverdueClientList->setCallbackForRowActivated ( [&] ( const uint row ) { return insertNavItem ( static_cast<dbListItem*>( ui->paysOverdueClientList->item ( static_cast<int>( row ) ) )->relatedItem ( RLI_CLIENTITEM ) ); } );
	ui->paysOverdueClientList->setUtilitiesPlaceLayout ( ui->layoutClientOverduePaysUtility );
	ui->paysOverdueClientList->setCallbackForWidgetGettingFocus ( [&] () { ui->lblCurInfoPay->callLabelActivatedFunc (); } );

	dbTableWidget::createPayHistoryTable ( ui->tablePayments );
	savePayWidget ( ui->tablePayments, FLD_PAY_INFO );
	ui->tablePayments->setUtilitiesPlaceLayout ( ui->layoutPayTableUtility );
	ui->tablePayments->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) {
		return interceptPaymentCellChange ( item ); } );

	static_cast<void>( connect ( ui->tabPaysLists, &QTabWidget::currentChanged, this, [&] ( const int index ) { return tabPaysLists_currentChanged ( index ); } ) );

	actPayReceipt = new QAction ( ICON ( "generate_paystub.png"), TR_FUNC ( "Generate payment stub for the current selected payment" ) );
	static_cast<void>( connect ( actPayReceipt, &QAction::triggered, this, [&] () { EDITOR ()->startNewReport ()->createPaymentStub ( mPayCurItem->dbRecID () ); } ) );

	actPayUnPaidReport = new QAction ( ICON ( "generate_report_unpaid.png"), TR_FUNC ( "Generate a payments report for all unpaid payments" ) );
	static_cast<void>( connect ( actPayUnPaidReport, &QAction::triggered, this, [&] () { EDITOR ()->startNewReport ()->createJobReport ( mClientCurItem->dbRecID (), false, emptyString, true ); } ) );

	actPayReport = new QAction ( ICON ( "generate_report.png"), TR_FUNC ( "Generate a payments report for all payments" ) );
	static_cast<void>( connect ( actPayReport, &QAction::triggered, this, [&] () { EDITOR ()->startNewReport ()->createJobReport ( mClientCurItem->dbRecID (), true, emptyString, true ); } ) );
}

void MainWindow::displayPayFromCalendar ( dbListItem* cal_item )
{
	auto client_item ( static_cast<clientListItem*>( cal_item->relatedItem ( RLI_CLIENTPARENT ) ) );
	if ( client_item )
	{
		if ( client_item != mClientCurItem )
			displayClient ( client_item, true, static_cast<jobListItem*>( cal_item->relatedItem ( RLI_JOBPARENT ) ) );
		else
			displayJob ( static_cast<jobListItem*>( cal_item->relatedItem ( RLI_JOBPARENT ) ), true );

		ui->tablePayments->setCurrentCell ( cal_item->data ( Qt::UserRole ).toInt () - 1, 0 );
		// We do not want to simply call displayPay (), we want to redirect the user to view that payment. Unlike notifyExternalChange (),
		// which merely has to update the display but the workflow must remain within DB_TABLE_VIEW
		ui->lblCurInfoPay->callLabelActivatedFunc ();
	}
}

void MainWindow::paysListWidget_currentItemChanged ( dbListItem* item )
{
	if ( item != mPayCurItem )
	{
		if ( static_cast<jobListItem*>( item->relatedItem ( RLI_JOBPARENT ) ) != mJobCurItem )
			displayJob ( static_cast<jobListItem*>( item->relatedItem ( RLI_JOBPARENT ) ), true );
	}
}

void MainWindow::paysOverdueListWidget_currentItemChanged ( dbListItem* item )
{
	if ( item->relatedItem ( RLI_CLIENTITEM ) != mPayCurItem )
	{
		auto pay_item ( static_cast<payListItem*>( item ) );
		/* displayClient will ignore a chunk of calls if pay_item->client_parent == mClientCurItem, which it is and supposed to.
		* Force reselection of current client
		*/
		mClientCurItem = nullptr;
		displayClient ( static_cast<clientListItem*>( pay_item->relatedItem ( RLI_CLIENTPARENT ) ), true,
			static_cast<jobListItem*>( pay_item->relatedItem ( RLI_JOBPARENT ) ) );
	}
}

void MainWindow::paysOverdueClientListWidget_currentItemChanged ( dbListItem* item )
{
	if ( item->relatedItem ( RLI_CLIENTITEM ) != mPayCurItem )
	{
		displayJob ( static_cast<jobListItem*>( item->relatedItem ( RLI_JOBPARENT ) ), true );
	}
}

void MainWindow::addPaymentOverdueItems ( payListItem* pay_item )
{
	bool bOldState ( true );
	if ( ui->tabPaysLists->currentIndex () == 1 )
	{
		bOldState = ui->paysOverdueClientList->isIgnoringChanges ();
		ui->paysOverdueClientList->setIgnoreChanges ( true );
		auto pay_item_overdue_client ( new payListItem );
		pay_item_overdue_client->setRelation ( PAY_ITEM_OVERDUE_CLIENT );
		pay_item->syncSiblingWithThis ( pay_item_overdue_client );
		pay_item_overdue_client->update ();
		pay_item_overdue_client->addToList ( ui->paysOverdueClientList );
		ui->paysOverdueClientList->setIgnoreChanges ( bOldState );
	}
	
	if ( ui->paysOverdueList->count () != 0 )
	{
		bOldState = ui->paysOverdueList->isIgnoringChanges ();
		ui->paysOverdueList->setIgnoreChanges ( true );
		auto pay_item_overdue ( new payListItem );
		pay_item_overdue->setRelation ( PAY_ITEM_OVERDUE_ALL );
		pay_item->syncSiblingWithThis ( pay_item_overdue );
		pay_item_overdue->update ();
		pay_item_overdue->addToList ( ui->paysOverdueList );
		ui->paysOverdueList->setIgnoreChanges ( bOldState );
	}
}

void MainWindow::removePaymentOverdueItems ( payListItem* pay_item )
{
	if ( pay_item->relatedItem ( PAY_ITEM_OVERDUE_CLIENT ) )
	{
		ui->paysOverdueClientList->removeRow_list ( static_cast<uint>( pay_item->relatedItem ( PAY_ITEM_OVERDUE_CLIENT )->row () ), 1, true );
	}
	
	if ( pay_item->relatedItem ( PAY_ITEM_OVERDUE_ALL ) )
	{
		ui->paysOverdueList->removeRow_list ( static_cast<uint>( pay_item->relatedItem ( PAY_ITEM_OVERDUE_ALL )->row () ), 1, true );
	}
}

void MainWindow::updateTotalPayValue ( const vmNumber& nAdd, const vmNumber& nSub )
{
	vmNumber totalPrice;
	totalPrice.fromTrustedStrPrice ( ui->paysList->property ( PROPERTY_TOTAL_PAYS ).toString (), false );
	totalPrice -= nSub;
	totalPrice += nAdd;
	ui->paysList->setProperty ( PROPERTY_TOTAL_PAYS, totalPrice.toPrice () );
	if ( ui->tabPaysLists->currentIndex () == 0 )
		ui->txtClientPayTotals->setText ( totalPrice.toPrice () );	
}

void MainWindow::updatePayOverdueTotals ( const vmNumber& nAdd, const vmNumber& nSub )
{
	vmNumber totalPrice;
	if ( ui->paysOverdueClientList->count () != 0 )
	{
		totalPrice.fromTrustedStrPrice ( ui->paysOverdueClientList->property ( PROPERTY_TOTAL_PAYS ).toString (), false );
		totalPrice -= nSub;
		totalPrice += nAdd;
		ui->paysOverdueClientList->setProperty ( PROPERTY_TOTAL_PAYS, totalPrice.toPrice () );
		ui->txtClientPayOverdueTotals->setText ( totalPrice.toPrice () );
	}
	if ( ui->paysOverdueList->count () != 0 )
	{
		totalPrice.fromTrustedStrPrice ( ui->paysOverdueList->property ( PROPERTY_TOTAL_PAYS ).toString (), false );
		totalPrice -= nSub;
		totalPrice += nAdd;
		ui->paysOverdueList->setProperty ( PROPERTY_TOTAL_PAYS, totalPrice.toPrice () );
		ui->txtAllOverdueTotals->setText ( totalPrice.toPrice () );
	}
}

void MainWindow::payOverdueGUIActions ( Payment* const pay, const RECORD_ACTION new_action )
{
	bool bIsOverdue ( false );
	if ( pay )
	{
		const int overdueIdx ( recStrValue ( pay, FLD_PAY_OVERDUE ).toInt () );
		if ( new_action != ACTION_NONE )
		{
			vmNumber new_price, old_price;
			vmNumber new_paid, old_paid;
			if ( overdueIdx == 2 && new_action == ACTION_EDIT )
				*(const_cast<RECORD_ACTION*>( &new_action )) = ACTION_DEL;
			switch ( new_action )
			{
				case ACTION_EDIT:
					new_price.fromTrustedStrPrice ( ui->txtPayTotalPrice->text (), false );
					old_price.fromTrustedStrPrice ( pay->backupRecordStr ( FLD_PAY_PRICE ), false );
					new_paid.fromTrustedStrPrice ( ui->txtPayTotalPaid->text (), false );
					old_paid.fromTrustedStrPrice ( pay->backupRecordStr ( FLD_PAY_TOTALPAID ), false );
				break;
				case ACTION_REVERT:
				case ACTION_READ:
					new_price.fromTrustedStrPrice ( pay->actualRecordStr ( FLD_PAY_PRICE ), false );
					old_price.fromTrustedStrPrice ( pay->backupRecordStr ( FLD_PAY_PRICE ), false );
					new_paid.fromTrustedStrPrice ( pay->actualRecordStr ( FLD_PAY_TOTALPAID ), false );
					old_paid.fromTrustedStrPrice ( pay->backupRecordStr ( FLD_PAY_TOTALPAID ), false );
				break;
				case ACTION_ADD:
					new_price.fromTrustedStrPrice ( pay->actualRecordStr( FLD_PAY_PRICE ), false );
					old_price = vmNumber::zeroedPrice;
					new_paid.fromTrustedStrPrice ( pay->actualRecordStr( FLD_PAY_TOTALPAID ), false );
					old_paid = vmNumber::zeroedPrice;
				break;
				case ACTION_DEL:
					new_price = vmNumber::zeroedPrice;
					old_price.fromTrustedStrPrice ( recStrValue ( pay, FLD_PAY_PRICE ) );
					new_paid = vmNumber::zeroedPrice;
					old_paid.fromTrustedStrPrice ( recStrValue ( pay, FLD_PAY_TOTALPAID ) );
				break;
				case ACTION_NONE: break;
			}
			
			bIsOverdue = new_paid < new_price;
			if ( bIsOverdue && overdueIdx == 0 )
				setRecValue ( pay, FLD_PAY_OVERDUE, CHR_ONE );
			else if ( !bIsOverdue && overdueIdx == 1 )
				setRecValue ( pay, FLD_PAY_OVERDUE, CHR_ZERO );
			
			setRecValue ( pay, FLD_PAY_OVERDUE_VALUE, ( new_price - new_paid ).toPrice () );
			updatePayOverdueTotals ( new_price - new_paid, old_price - old_paid );
			bIsOverdue &= overdueIdx != 2;
		}
		else //reading record. Only update the one control
		{
			ui->chkPayOverdue->setChecked ( overdueIdx == 2 );
			bIsOverdue = ( overdueIdx == 1 );
		}
		ui->chkPayOverdue->setToolTip ( chkPayOverdueToolTip_overdue[overdueIdx] );
		pay->payItem ()->update ();
	}
	else
	{
		ui->chkPayOverdue->setToolTip (	TR_FUNC ( "Check this box to signal that payment overdue is to be ignored." ) );
		ui->chkPayOverdue->setChecked ( false );
	}
	ui->chkPayOverdue->highlight ( bIsOverdue ? vmYellow : vmDefault_Color );
	ui->txtPayTotalPrice->highlight ( bIsOverdue ? vmYellow : vmDefault_Color );
}

void MainWindow::controlPayForms ( const payListItem* const pay_item )
{
	const triStateType editing_action ( pay_item ? static_cast<TRI_STATE> ( pay_item->action () == ACTION_EDIT ) : TRI_UNDEF );

	//Make tables editable only when editing a record, because in such situations there is no clearing and loading involved, but a mere display change
	ui->tablePayments->setEditable ( editing_action.isOn () );

	ui->txtPayObs->setEditable ( editing_action.isOn () );
	ui->txtPayTotalPrice->setEditable ( editing_action.isOn () );
	ui->chkPayOverdue->setEditable ( editing_action.isOn () );

	sectionActions[2].b_canEdit = editing_action.isOff ();
	sectionActions[2].b_canRemove = editing_action.isOn ();
	sectionActions[2].b_canSave = editing_action.isOn () ? pay_item->isGoodToSave () : false;
	sectionActions[2].b_canCancel = editing_action.isOn ();
	sectionActions[2].b_canExtra[0] = ui->paysList->count () > 0;
	sectionActions[2].b_canExtra[1] = ui->paysList->count () > 0;
	sectionActions[2].b_canExtra[2] = pay_item != nullptr && !pay_item->payRecord ()->price ( FLD_PAY_TOTALPAID ).isNull ();
	updateActionButtonsState ();

	const vmNumber paid_price ( ui->txtPayTotalPaid->text (), VMNT_PRICE, 1 );
	const vmNumber total_price ( ui->txtPayTotalPrice->text (), VMNT_PRICE, 1 );
	ui->txtPayTotalPaid->highlight ( paid_price < total_price ? vmYellow : vmDefault_Color );

	ui->tablePayments->scrollTo ( ui->tablePayments->indexAt ( QPoint ( 0, 0 ) ) );
}

bool MainWindow::savePay ( payListItem* pay_item )
{
	if ( pay_item != nullptr )
	{
		Payment* pay ( pay_item->payRecord () );
		const bool bWasOverdue ( pay->actualRecordStr ( FLD_PAY_OVERDUE ) == CHR_ONE );
		
		if ( pay->saveRecord ( false ) ) // do not change indexes just now. Wait for dbCalendar actions
		{	
			mCal->updateCalendarWithPayInfo ( pay );
			ui->tablePayments->setTableUpdated ();
			if ( pay->wasModified ( FLD_PAY_OVERDUE ) )
			{
				const bool bIsOverdue ( recStrValue ( pay, FLD_PAY_OVERDUE ) == CHR_ONE );
				if ( bIsOverdue && !bWasOverdue )
					addPaymentOverdueItems ( pay_item );
				else if ( !bIsOverdue && bWasOverdue )
					removePaymentOverdueItems ( pay_item );
			}
			pay_item->setAction ( ACTION_READ, true );
			
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Record saved" ), TR_FUNC ( "Payment data saved!" ) );
			controlPayForms ( pay_item );
			removeEditItem ( static_cast<dbListItem*>(pay_item) );
			return true;
		}
	}
	return false;
}

bool MainWindow::editPay ( payListItem* pay_item, const bool b_dogui )
{
	if ( pay_item )
	{
		if ( pay_item->action () == ACTION_READ )
		{
			pay_item->setAction ( ACTION_EDIT, true );
			insertEditItem ( static_cast<dbListItem*>( pay_item ) );
			if ( b_dogui )
			{
				controlPayForms ( pay_item );
				ui->tablePayments->setFocus ();
			}
			return true;
		}
	}
	return false;
}

bool MainWindow::delPay ( payListItem* pay_item )
{
	if ( pay_item )
	{
		const uint row ( ui->tablePayments->currentRow () == -1 ? 0 : static_cast<uint> ( ui->tablePayments->currentRow () ) );
		ui->tablePayments->removeRow ( row );
		mCal->updateCalendarWithPayInfo ( pay_item->payRecord () );
		return true;
	}
	return false;
}

bool MainWindow::cancelPay ( payListItem* pay_item )
{
	if ( pay_item )
	{
		if ( pay_item->action () == ACTION_EDIT )
		{
			removeEditItem ( static_cast<dbListItem*>(pay_item) );
			payOverdueGUIActions ( pay_item->payRecord (), ACTION_REVERT );
			pay_item->setAction ( ACTION_REVERT, true );
			displayPay ( pay_item );
			return true;
		}
	}
	return false;
}

void MainWindow::displayPay ( payListItem* pay_item, const bool b_select )
{
	if ( pay_item )
	{
		//pay_item might be a calendar item, one of the extra items. We need to use the main item
		payListItem* parent_pay ( static_cast<payListItem*>( pay_item->relatedItem ( RLI_CLIENTITEM ) ) );
		if ( !parent_pay ) // if this happens, something is definetely off. Should not happen
			parent_pay = pay_item;
		if ( parent_pay->loadData ( true ) )
		{
			if ( parent_pay != mPayCurItem )
			{
				if ( b_select )
					ui->paysList->setCurrentItem ( parent_pay, true, false );
				mPayCurItem = pay_item;
				ui->paysOverdueClientList->setCurrentItem ( mPayCurItem->relatedItem ( PAY_ITEM_OVERDUE_CLIENT ), true, false );
				ui->paysOverdueList->setCurrentItem ( mPayCurItem->relatedItem ( PAY_ITEM_OVERDUE_ALL ), true, false );
			}
			loadPayInfo ( parent_pay->payRecord () );
			controlPayForms ( parent_pay );
		}
	}
	else
	{
		mPayCurItem = nullptr;
		loadPayInfo ( nullptr );
		controlPayForms ( nullptr );
	}
	if ( b_select )
		changeNavLabels ();
}

void MainWindow::loadPayInfo ( const Payment* const pay )
{
	if ( pay != nullptr )
	{
		ui->txtPayID->setText ( recStrValue ( pay, FLD_PAY_ID ) );
		ui->txtPayTotalPrice->setText ( pay->action () != ACTION_ADD ? recStrValue ( pay, FLD_PAY_PRICE ) :
					( mJobCurItem ? recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_PRICE ) :
						  vmNumber::zeroedPrice.toPrice () ) );
		ui->txtPayTotalPaid->setText ( recStrValue ( pay, FLD_PAY_TOTALPAID ) );
		ui->txtPayObs->setText ( recStrValue ( pay, FLD_PAY_OBS ) );
		ui->tablePayments->loadFromStringTable ( recStrValue ( pay, FLD_PAY_INFO ) );

		payOverdueGUIActions ( const_cast<Payment*>( pay ), ACTION_NONE );
	}
	else
	{
		ui->tablePayments->setEditable ( false );
		ui->tablePayments->clear ( true );
		ui->txtPayID->setText ( QStringLiteral ( "-1" ) );
		ui->txtPayTotalPrice->setText ( vmNumber::zeroedPrice.toPrice () );
		ui->txtPayObs->setText ( QStringLiteral ( "N/A" ) );
		ui->txtPayTotalPaid->setText ( vmNumber::zeroedPrice.toPrice () );
	}
}

payListItem* MainWindow::getPayItem ( const clientListItem* const parent_client, const uint id ) const
{
	if ( id >= 1 && parent_client )
	{
		int i ( 0 );
		while ( i < static_cast<int> ( parent_client->pays->count () ) )
		{
			if ( parent_client->pays->at ( i )->dbRecID () == id )
				return parent_client->pays->at ( i );
			++i;
		}
	}
	return nullptr;
}

void MainWindow::loadClientOverduesList ()
{
	if ( !mClientCurItem ) return;

	if ( ui->paysOverdueClientList->count () != 0 )
	{
		if ( static_cast<clientListItem*>( static_cast<payListItem*>( ui->paysOverdueClientList->item ( 0 ) )->relatedItem ( RLI_CLIENTPARENT ) ) == mClientCurItem )
			return;
		ui->paysOverdueClientList->clear ();
	}
	ui->paysOverdueClientList->setIgnoreChanges ( true );
	payListItem* pay_item_overdue_client ( nullptr ), *pay_item ( nullptr );
	vmNumber totalOverdue;
	for ( uint x ( 0 ); x < mClientCurItem->pays->count (); ++x )
	{
		pay_item = mClientCurItem->pays->at ( x );
		pay_item_overdue_client = static_cast<payListItem*>( pay_item->relatedItem ( PAY_ITEM_OVERDUE_CLIENT ) );
		if ( !pay_item_overdue_client )
		{
			if ( pay_item->loadData ( true ) )
			{
				if ( recStrValue ( pay_item->payRecord (), FLD_PAY_OVERDUE ) == CHR_ONE )
				{
					pay_item_overdue_client = new payListItem;
					pay_item_overdue_client->setRelation ( PAY_ITEM_OVERDUE_CLIENT );
					pay_item->syncSiblingWithThis ( pay_item_overdue_client );
					pay_item_overdue_client->update ();
					pay_item_overdue_client->addToList ( ui->paysOverdueClientList );
					totalOverdue += pay_item->payRecord ()->price ( FLD_PAY_OVERDUE_VALUE );
				}
				else
					continue;
			}
			else
				continue;
		}
	}
	ui->paysOverdueClientList->setProperty ( PROPERTY_TOTAL_PAYS, totalOverdue.toPrice () );
	ui->txtClientPayOverdueTotals->setText ( totalOverdue.toPrice () );
	ui->paysOverdueClientList->setIgnoreChanges ( false );
}

void MainWindow::loadAllOverduesList ()
{
	if ( ui->paysOverdueList->count () == 0 )
	{
		payListItem* pay_item_overdue ( nullptr ), *pay_item ( nullptr );
		clientListItem* client_item ( nullptr );
		const int n ( ui->clientsList->count () );
		vmNumber totalOverdue;
		ui->paysOverdueList->setIgnoreChanges ( true );
		for ( int i ( 0 ); i < n; ++i )
		{
			client_item = static_cast<clientListItem*> ( ui->clientsList->item ( i ) );
			for ( uint x ( 0 ); x < client_item->pays->count (); ++x )
			{
				pay_item = client_item->pays->at ( x );
				if ( pay_item->loadData ( true ) )
				{
					if ( recStrValue ( pay_item->payRecord (), FLD_PAY_OVERDUE ) == CHR_ONE ) 
					{
						pay_item_overdue = new payListItem;
						pay_item_overdue->setRelation ( PAY_ITEM_OVERDUE_ALL );
						pay_item->syncSiblingWithThis ( pay_item_overdue );
						pay_item_overdue->update ();
						pay_item_overdue->addToList ( ui->paysOverdueList );
						totalOverdue += pay_item->payRecord ()->price ( FLD_PAY_OVERDUE_VALUE );
					}
				}
			}
		}
		ui->paysOverdueList->setProperty ( PROPERTY_TOTAL_PAYS, totalOverdue.toPrice () );
		ui->txtAllOverdueTotals->setText ( totalOverdue.toPrice () );
		ui->paysOverdueList->setIgnoreChanges ( false );
	}
}

void MainWindow::interceptPaymentCellChange ( const vmTableItem* const item )
{
	const auto row ( static_cast<uint>( item->row () ) );
	if ( static_cast<int>( row ) == ui->tablePayments->totalsRow () )
		return;

	const auto col ( static_cast<uint>( item->column () ) );
	const bool b_paid ( ui->tablePayments->sheetItem ( row, PHR_PAID )->text () == CHR_ONE );

	triStateType input_ok;
	switch ( col )
	{
		case PHR_DATE:
		case PHR_USE_DATE:
		{
			input_ok = ui->tablePayments->isRowEmpty ( row );
			if ( input_ok.isOff () )
			{
				const vmNumber& cell_date ( item->date () );
				input_ok = cell_date.isDateWithinRange ( vmNumber::currentDate (), 0, 4 );
			}
			mPayCurItem->setFieldInputStatus ( col + INFO_TABLE_COLUMNS_OFFSET, input_ok.isOn (), item );
			if ( col == PHR_DATE )
				ui->tablePayments->sheetItem ( row, PHR_VALUE )->setText ( 
							recStrValue ( mPayCurItem->payRecord (), FLD_PAY_OVERDUE_VALUE ), false, false, false ) ;
		}
		break;
		case PHR_PAID:
		case PHR_VALUE:
			updatePayTotalPaidValue ();
			if ( b_paid && !ui->tablePayments->sheetItem ( row, PHR_DATE )->cellIsAltered () )
				ui->tablePayments->sheetItem ( row, PHR_DATE )->setDate ( vmNumber::currentDate () );

			input_ok = ( !b_paid == ui->tablePayments->sheetItem ( row, PHR_VALUE )->number ().isNull () );
			mPayCurItem->setFieldInputStatus ( PHR_VALUE + INFO_TABLE_COLUMNS_OFFSET, input_ok.isOn (), ui->tablePayments->sheetItem ( row, PHR_VALUE ) );

			if ( col == PHR_PAID )
			{
				// dates in the future are ok. Probably the payment is scheduled to happen at some future date
				if ( ui->tablePayments->sheetItem ( row, PHR_DATE )->date () <= vmNumber::currentDate () )
				{
					input_ok = ( b_paid == !( ui->tablePayments->sheetItem ( row, PHR_METHOD )->text ().isEmpty () ) );
					mPayCurItem->setFieldInputStatus ( PHR_METHOD + INFO_TABLE_COLUMNS_OFFSET, input_ok.isOn (), ui->tablePayments->sheetItem ( row, PHR_METHOD ) );
					input_ok = ( b_paid == !( ui->tablePayments->sheetItem ( row, PHR_ACCOUNT )->text ().isEmpty () ) );
					mPayCurItem->setFieldInputStatus ( PHR_ACCOUNT + INFO_TABLE_COLUMNS_OFFSET, input_ok.isOn (), ui->tablePayments->sheetItem ( row, PHR_ACCOUNT ) );
				}
			}
		break;
		case PHR_METHOD:
		case PHR_ACCOUNT:
			input_ok = ( b_paid == !( ui->tablePayments->sheetItem ( row, col )->text ().isEmpty () ) );
			mPayCurItem->setFieldInputStatus ( col + INFO_TABLE_COLUMNS_OFFSET, input_ok.isOn (), item );
		break;
		default:
		break;
	}

	if ( col == PHR_DATE || col == PHR_METHOD )
	{
		const QString strMethod ( ui->tablePayments->sheetItem ( row, PHR_METHOD )->text () );
		if ( strMethod == QStringLiteral ( "TED" ) || strMethod == QStringLiteral ( "DOC" ) || 
			 strMethod.contains ( QStringLiteral ( "Transferência" ), Qt::CaseInsensitive ) )
		{
			 ui->tablePayments->sheetItem ( row, PHR_USE_DATE )->setDate (
								ui->tablePayments->sheetItem ( row, PHR_DATE )->date () );
			 ui->tablePayments->sheetItem ( row, PHR_ACCOUNT )->setText ( QStringLiteral ( "BB - CC" ), false, true, false );
		}
	}
	
	//if ( input_ok.isOn () )
	//{
		if ( ( row + 1 ) > recStrValue ( mPayCurItem->payRecord (), FLD_PAY_TOTALPAYS ).toUInt () )
			 setRecValue ( mPayCurItem->payRecord (), FLD_PAY_TOTALPAYS, QString::number ( row + 1 ) );
		
		stringTable pay_data;
		ui->tablePayments->makeStringTable ( pay_data );

		switch ( col )
		{
			case PHR_METHOD:
				COMPLETERS ()->updateCompleter ( item->text (), CC_PAYMENT_METHOD );
			break;
			case PHR_ACCOUNT:
				COMPLETERS ()->updateCompleter ( item->text (), CC_ACCOUNT );
			break;
			default:
			break;
		}
		
		setRecValue ( mPayCurItem->payRecord (), FLD_PAY_INFO, pay_data.toString () );
	//}
	postFieldAlterationActions ( mPayCurItem );
}

void MainWindow::updatePayTotalPaidValue ()
{
	vmNumber total_paid;
	for ( int i ( 0 ); i <= ui->tablePayments->lastUsedRow (); ++i )
	{
		if ( ui->tablePayments->sheetItem ( static_cast<uint>(i), PHR_PAID )->text () == CHR_ONE )
			total_paid += vmNumber ( ui->tablePayments->sheetItem ( static_cast<uint>(i), PHR_VALUE )->text (), VMNT_PRICE );
	}
	ui->txtPayTotalPaid->setText ( total_paid.toPrice () ); // do not notify text change so that payOverdueGUIActions can use the current stores paid value
	payOverdueGUIActions ( mPayCurItem->payRecord (), ACTION_EDIT );
	setRecValue ( mPayCurItem->payRecord (), FLD_PAY_TOTALPAID, total_paid.toPrice () ); // now we can update the record
}

// Delay loading lists until they are necessary
void MainWindow::tabPaysLists_currentChanged ( const int index )
{
	switch ( index )
	{
		case 0: break;
		case 1: loadClientOverduesList (); break;
		case 2:	loadAllOverduesList ();	break;
	}
	ui->paysOverdueClientList->setIgnoreChanges ( index != 1 );
	ui->paysOverdueList->setIgnoreChanges ( index != 2 );
}

void MainWindow::payExternalChange ( const uint id, const RECORD_ACTION action )
{
	QScopedPointer<Payment> pay ( new Payment );
	if ( !pay->readRecord ( id ) )
		return;
	clientListItem* client_parent ( getClientItem ( static_cast<uint>( recIntValue ( pay, FLD_PAY_CLIENTID ) ) ) );
	if ( !client_parent )
		return;
									   
	if ( action == ACTION_EDIT )
	{	
		payListItem* pay_item ( getPayItem ( client_parent, id ) );
		if ( pay_item )
		{
			if ( pay_item->payRecord () != nullptr )
				pay_item->payRecord ()->setRefreshFromDatabase ( true ); // if job has been loaded before, tell it to ignore the cache and to load data from the database upon next read
			pay_item->update ();
			if ( id == mPayCurItem->dbRecID () )
				displayPay ( pay_item );
		}
	}
	else
	{
		if ( static_cast<uint>( recIntValue ( pay, FLD_PAY_CLIENTID ) ) != mClientCurItem->dbRecID () )
			displayClient (	client_parent, true, getJobItem ( client_parent, static_cast<uint>( recIntValue ( pay, FLD_PAY_JOBID ) ) ) );
			
		if ( action == ACTION_ADD )
		{
			jobListItem* new_job_item ( addJob ( client_parent ) );
			Job* new_job ( new_job_item->jobRecord () );
			new_job->readRecord ( VDB ()->getHighestID ( TABLE_JOB_ORDER ) + 1 );
			if ( saveJob ( new_job_item, false ) )
			{
				payListItem* new_pay_item ( new_job_item->payItem () );
				Payment* new_pay ( new_pay_item->payRecord () );
				new_pay->setRefreshFromDatabase ( true );
				new_pay->readRecord ( id );
				static_cast<void>( savePay ( new_pay_item ) );
			}
		}
		else if ( action == ACTION_DEL )
		{
			delPay ( getPayItem ( client_parent, id ) );
		}
	}
}
//--------------------------------------------PAY------------------------------------------------------------

//-------------------------------------EDITING-FINISHED-PAY--------------------------------------------------
void MainWindow::txtPayTotalPrice_textAltered ( const QString& text )
{
	const vmNumber new_price ( text, VMNT_PRICE, 1 );
	payOverdueGUIActions ( mPayCurItem->payRecord (), mPayCurItem->payRecord ()->action () );
	updateTotalPayValue ( new_price, vmNumber ( recStrValue ( mPayCurItem->payRecord (), FLD_PAY_PRICE ), VMNT_PRICE, 1 ) );

	bool input_ok ( true );
	if ( static_cast<jobListItem*> ( mPayCurItem->relatedItem ( RLI_JOBPARENT ) )->jobRecord ()->price ( FLD_JOB_PRICE ).isNull () )
		input_ok = !new_price.isNull ();
	else
	{
		ui->txtPayTotalPrice->setText ( new_price.toPrice () );
		setRecValue ( mPayCurItem->payRecord (), FLD_PAY_PRICE, new_price.toPrice () );
	}
	mPayCurItem->setFieldInputStatus ( FLD_PAY_PRICE, input_ok, ui->txtPayTotalPrice );
	postFieldAlterationActions ( mPayCurItem );
}

void MainWindow::txtPay_textAltered ( const vmWidget* const sender )
{
	setRecValue ( mPayCurItem->payRecord (), static_cast<uint>( sender->id () ), sender->text () );
	/* No need to call postFieldAlterationActions because the controls bound to the other fields
	 * are readonly and that function will be called from the table change callback */
	if ( static_cast<uint>( sender->id () ) == FLD_PAY_OBS )
		postFieldAlterationActions ( mPayCurItem );
}

void MainWindow::chkPayOverdue_toggled ( const bool checked )
{
	const QString* chr ( &CHR_ZERO );
	if ( checked )
		chr = &CHR_TWO; //ignore price differences
	else
	{
		const vmNumber& paid_price ( mPayCurItem->payRecord ()->price ( FLD_PAY_TOTALPAID ) );
		if ( paid_price < mPayCurItem->payRecord ()->price ( FLD_PAY_PRICE ) )
			chr = &CHR_ONE;
	}
	setRecValue ( mPayCurItem->payRecord (), FLD_PAY_OVERDUE, *chr );
	payOverdueGUIActions ( mPayCurItem->payRecord (), mPayCurItem->payRecord ()->action () );
	postFieldAlterationActions ( mPayCurItem );
}
//--------------------------------------------EDITING-FINISHED-PAY-----------------------------------------------------------

//--------------------------------------------BUY------------------------------------------------------------
void MainWindow::saveBuyWidget ( vmWidget* widget, const int id )
{
	widget->setID ( id );
	buyWidgetList[id] = widget;
}

void MainWindow::showBuySearchResult ( dbListItem* item, const bool bshow )
{
	auto buy_item ( static_cast<buyListItem*> ( item ) );
	if ( bshow )
	{
		displayClient ( static_cast<clientListItem*>( buy_item->relatedItem ( RLI_CLIENTPARENT ) ), true,
						static_cast<jobListItem*>( buy_item->relatedItem ( RLI_JOBPARENT ) ),
						buy_item );
	}

	for ( uint i ( 0 ); i < BUY_FIELD_COUNT; ++i )
	{
		if ( buy_item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
		{
			if ( i == FLD_BUY_PAYINFO )
			{
				if ( bshow )
					ui->tableBuyPayments->searchStart ( SEARCH_UI ()->searchTerm () );
				else
					ui->tableBuyPayments->searchCancel ();
			}
			else if ( i == FLD_BUY_REPORT )
			{
				if ( bshow)
					ui->tableBuyItems->searchStart ( SEARCH_UI ()->searchTerm () );
				else
					ui->tableBuyItems->searchCancel ();
			}
			else
				buyWidgetList.at ( i )->highlight ( bshow ? vmBlue : vmDefault_Color, SEARCH_UI ()->searchTerm () );
		}
	}
}

void MainWindow::setupBuyPanel ()
{
	ui->buysList->setUtilitiesPlaceLayout ( ui->layoutBuysListUtility );
	ui->buysList->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) { return buysListWidget_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );
	ui->buysList->setCallbackForRowActivated ( [&] ( const uint row ) {
		return insertNavItem ( static_cast<dbListItem*>( ui->buysList->item ( static_cast<int>( row ) ) ) ); } );
	ui->buysList->setCallbackForWidgetGettingFocus ( [&] () { ui->lblCurInfoBuy->callLabelActivatedFunc (); } );
	
	ui->buysJobList->setUtilitiesPlaceLayout ( ui->vLayoutBuyJobTotals );
	ui->buysJobList->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) { return buysJobListWidget_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );
	ui->buysJobList->setCallbackForRowActivated ( [&] ( const uint row ) {
		return insertNavItem ( static_cast<dbListItem*>( ui->buysJobList->item ( static_cast<int>( row ) ) )->relatedItem ( RLI_CLIENTITEM ) ); } );
	ui->buysJobList->setCallbackForWidgetGettingFocus ( [&] () { ui->lblCurInfoBuy->callLabelActivatedFunc (); } );

	ui->tableBuyItemsSelectiveView->setUtilitiesPlaceLayout( ui->vLayoutBuyJobSelectiveTotals );
	ui->tableBuyItemsSelectiveView->setIsPlainTable ( false );
	ui->tableBuyItemsSelectiveView->setSelectionBehavior ( QAbstractItemView::SelectRows );
	ui->tableBuyItemsSelectiveView->setSelectionMode ( QAbstractItemView::ExtendedSelection );
	dbTableWidget::createPurchasesTable ( ui->tableBuyItemsSelectiveView );
	connect ( ui->tableBuyItemsSelectiveView, &QTableWidget::itemSelectionChanged, this, [&] () { updateBuySelectionTotals (); } );
	ui->tableBuyItemsSelectiveView->setCallbackForNewViewAfterFilter ( [&] () { updateBuyFilteredViewTotals (); } );
	connect ( ui->tabBuysSelectiveView, &QTabWidget::currentChanged, this, [&] ( const int index ) {
		if ( index == 0 ) ui->buysJobList->setFocus(); else ui->tableBuyItemsSelectiveView->setFocus ();
	} );

	ui->lstBuySuppliers->setUtilitiesPlaceLayout ( ui->layoutSuppliersUtility );
	ui->lstBuySuppliers->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) {
		return buySuppliersListWidget_currentItemChanged ( static_cast<dbListItem*>( item ) ); } );
	ui->lstBuySuppliers->setCallbackForWidgetGettingFocus ( [&] () { ui->lblCurInfoBuy->callLabelActivatedFunc (); } );

	saveBuyWidget ( ui->txtBuyID, FLD_BUY_ID );
	saveBuyWidget ( ui->txtBuyTotalPrice->lineControl (), FLD_BUY_PRICE );
	ui->txtBuyTotalPrice->lineControl ()->setTextType ( vmLineEdit::TT_PRICE );
	ui->txtBuyTotalPrice->setButtonType ( 0, vmLineEditWithButton::LEBT_CALC_BUTTON );
	ui->txtBuyTotalPrice->lineControl ()->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtBuy_textAltered ( sender ); } );

	saveBuyWidget ( ui->txtBuyTotalPaid->lineControl (), FLD_BUY_TOTALPAID );
	ui->txtBuyTotalPaid->lineControl ()->setTextType ( vmLineEdit::TT_PRICE );
	ui->txtBuyTotalPaid->setButtonType ( 0, vmLineEditWithButton::LEBT_CALC_BUTTON );
	ui->txtBuyTotalPaid->lineControl ()->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtBuy_textAltered ( sender ); } );

	ui->txtBuyTotalPurchasePriceForJob->setTextType ( vmLineEdit::TT_PRICE );
	ui->txtBuyTotalPaidForJob->setTextType ( vmLineEdit::TT_PRICE );
	ui->txtBuySelectiveViewTotal->setTextType ( vmLineEdit::TT_PRICE );
	ui->txtBuySelectiveViewSelected->setTextType ( vmLineEdit::TT_PRICE );

	saveBuyWidget ( ui->txtBuyNotes, FLD_BUY_NOTES );
	ui->txtBuyNotes->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtBuy_textAltered ( sender ); } );

	saveBuyWidget ( ui->txtBuyDeliveryMethod, FLD_BUY_DELIVERMETHOD );
	COMPLETERS ()->setCompleterForWidget ( ui->txtBuyDeliveryMethod, CC_DELIVERY_METHOD );
	ui->txtBuyDeliveryMethod->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtBuy_textAltered ( sender ); } );

	saveBuyWidget ( ui->dteBuyDate, FLD_BUY_DATE );
	ui->dteBuyDate->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dteBuy_dateAltered ( sender ); } );
	saveBuyWidget ( ui->dteBuyDeliveryDate, FLD_BUY_DELIVERDATE );
	ui->dteBuyDeliveryDate->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dteBuy_dateAltered ( sender ); } );

	saveBuyWidget ( ui->cboBuySuppliers, FLD_BUY_SUPPLIER );
	COMPLETERS ()->setCompleterForWidget ( ui->cboBuySuppliers, CC_SUPPLIER );
	ui->cboBuySuppliers->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return cboBuySuppliers_textAltered ( sender->text () ); } );
	ui->cboBuySuppliers->setCallbackForIndexChanged ( [&] ( const int index ) {
		return cboBuySuppliers_indexChanged ( index ); } );
	ui->cboBuySuppliers->setIgnoreChanges ( false );

	dbTableWidget::createPurchasesTable ( ui->tableBuyItems );
	ui->tableBuyItems->setKeepModificationRecords ( false );
	saveBuyWidget ( ui->tableBuyItems, FLD_BUY_REPORT );
	ui->tableBuyItems->setUtilitiesPlaceLayout ( ui->layoutTableBuyUtility );
	ui->tableBuyItems->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) {
		return interceptBuyItemsCellChange ( item ); } );
	ui->tableBuyItems->setCallbackForMonitoredCellChanged ( [&] ( const vmTableItem* const item ) {
		return ui->txtBuyTotalPrice->setText ( item->text (), true ); } );

	dbTableWidget::createPayHistoryTable ( ui->tableBuyPayments, PHR_METHOD );
	ui->tableBuyPayments->setKeepModificationRecords ( false );
	saveBuyWidget ( ui->tableBuyPayments, FLD_BUY_PAYINFO );
	ui->tableBuyPayments->setUtilitiesPlaceLayout ( ui->layoutTableBuyPaysUtility );
	ui->tableBuyPayments->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) {
		return interceptBuyPaymentCellChange ( item ); } );

	static_cast<void>( connect ( ui->btnShowSuppliersDlg, &QToolButton::clicked, this, [&] () {
		if ( SUPPLIERS ()->isVisible () )
			SUPPLIERS ()->hideDialog ();
		else
			SUPPLIERS ()->displaySupplier ( ui->cboBuySuppliers->currentText (), true );
	} ) );

	ui->splitterBuyInfo->setSizes ( QList<int> () << static_cast<int>( 3* screen_width () / 8 ) << static_cast<int>( 5 * screen_width () / 8 ) );
}

void MainWindow::displayBuyFromCalendar ( dbListItem* cal_item )
{
	auto client_item ( static_cast<clientListItem*>( cal_item->relatedItem ( RLI_CLIENTPARENT ) ) );
	if ( client_item )
	{
		if ( client_item != mClientCurItem )
			displayClient ( client_item, true, static_cast<jobListItem*>( cal_item->relatedItem ( RLI_JOBPARENT ) ), static_cast<buyListItem*>( cal_item ) );
		else
			displayJob ( static_cast<jobListItem*>( cal_item->relatedItem ( RLI_JOBPARENT ) ), true, static_cast<buyListItem*>( cal_item ) );
		
		ui->tableBuyPayments->setCurrentCell ( cal_item->data ( Qt::UserRole ).toInt () - 1, 0 );
		// We do not want to simply call displayBuy (), we want to redirect the user to view that buy. Unlike notifyExternalChange (),
		// which merely has to update the display but the workflow must remain within DB_TABLE_VIEW
		ui->lblCurInfoBuy->callLabelActivatedFunc ();
	}
}

void MainWindow::buysListWidget_currentItemChanged ( dbListItem* item )
{
	if ( item )
	{
		if ( static_cast<jobListItem*> ( item->relatedItem ( RLI_JOBPARENT ) ) != mJobCurItem )
			displayJob ( static_cast<jobListItem*>( item->relatedItem ( RLI_JOBPARENT ) ), true, static_cast<buyListItem*>( item ) );
		else
			displayBuy ( static_cast<buyListItem*>( item ) );
	}
	else
		displayBuy ( nullptr );
}

void MainWindow::buysJobListWidget_currentItemChanged ( dbListItem* item )
{
	displayBuy ( item ? static_cast<buyListItem*>( item->relatedItem ( RLI_CLIENTITEM ) ) : nullptr, true );
}

void MainWindow::buySuppliersListWidget_currentItemChanged ( dbListItem* item )
{
	if ( item )
	{
		ui->lstBuySuppliers->setIgnoreChanges ( true );
		buyListItem* buy_item ( nullptr );
		jobListItem* job_item ( nullptr );
		const auto job_id ( static_cast<uint>( recIntValue ( item->dbRec (), FLD_BUY_JOBID ) ) );
		const auto buy_id ( static_cast<uint>( recIntValue ( item->dbRec (), FLD_BUY_ID ) ) );

		if ( static_cast<clientListItem*>( item->relatedItem ( RLI_CLIENTPARENT ) ) != mClientCurItem )
		{
			displayClient ( static_cast<clientListItem*>( item->relatedItem ( RLI_CLIENTPARENT ) ), true,
							job_item, buy_item, job_id, buy_id );
		}
		else
		{
			buy_item = getBuyItem ( static_cast<clientListItem*>( item->relatedItem ( RLI_CLIENTPARENT ) ), buy_id );
			if ( job_id != mJobCurItem->dbRecID () )
			{
				displayJob ( getJobItem ( static_cast<clientListItem*> (
						item->relatedItem ( RLI_CLIENTPARENT ) ), job_id ), true, buy_item ) ;
			}
			else
				displayBuy ( buy_item, true );
		}
		ui->lstBuySuppliers->setIgnoreChanges ( false );
	}
}

void MainWindow::updateBuyTotalPriceWidgets ( const buyListItem* const buy_item )
{
	const Buy* buy ( buy_item->buyRecord () );
	if ( buy->wasModified ( FLD_BUY_PRICE ) )
	{
		vmNumber totalPrice ( ui->txtBuyTotalPurchasePriceForJob->text (), VMNT_PRICE, 1 );
		if ( buy->prevAction () == ACTION_EDIT )
			totalPrice -=  vmNumber ( buy->backupRecordStr ( FLD_BUY_PRICE ), VMNT_PRICE, 1 );
		totalPrice += buy->price ( FLD_BUY_PRICE );
		ui->txtBuyTotalPurchasePriceForJob->setText ( totalPrice.toPrice () );
	}
	
	if ( buy->wasModified ( FLD_BUY_TOTALPAID ) )
	{
		vmNumber totalPaid ( ui->txtBuyTotalPaidForJob->text (), VMNT_PRICE, 1 );
		if ( buy->prevAction () == ACTION_EDIT )
			totalPaid -=  vmNumber ( buy->backupRecordStr ( FLD_BUY_PRICE ), VMNT_PRICE, 1 );
		totalPaid += buy->price ( FLD_BUY_TOTALPAID );
		ui->txtBuyTotalPaidForJob->setText ( totalPaid.toPrice () );
	}
}

void MainWindow::controlBuyForms ( const buyListItem* const buy_item )
{
	const triStateType editing_action ( buy_item ? static_cast<TRI_STATE> ( buy_item->action () >= ACTION_ADD ) : TRI_UNDEF );

	//Make tables editable only when editing a record, because in such situations there is no clearing and loading involved, but a mere display change
	ui->tableBuyItems->setEditable ( editing_action.isOn () );
	ui->tableBuyPayments->setEditable ( editing_action.isOn () );
	ui->txtBuyTotalPrice->setEditable ( editing_action.isOn () );
	ui->txtBuyDeliveryMethod->setEditable ( editing_action.isOn () );
	ui->cboBuySuppliers->vmComboBox::setEditable ( editing_action.isOn () );
	ui->dteBuyDate->setEditable ( editing_action.isOn () );
	ui->dteBuyDeliveryDate->setEditable ( editing_action.isOn () );
	ui->txtBuyNotes->setEditable ( editing_action.isOn () );

	sectionActions[3].b_canAdd = mJobCurItem != nullptr ? mJobCurItem->action() != ACTION_ADD : false;
	sectionActions[3].b_canEdit = editing_action.isOff ();
	sectionActions[3].b_canRemove = editing_action.isOff ();
	sectionActions[3].b_canSave = editing_action.isOn () ? buy_item->isGoodToSave () : false;
	sectionActions[3].b_canCancel = editing_action.isOn ();
	updateActionButtonsState ();
	
	static const Buy static_buy;
	const Buy* const buy ( buy_item ? buy_item->buyRecord () : &static_buy );
	const vmNumber buyTotalPrice ( recStrValue ( buy, FLD_BUY_PRICE ), VMNT_PRICE, 1 );
	ui->cboBuySuppliers->highlight ( editing_action.isOn () ? ( ui->cboBuySuppliers->text ().isEmpty () ? vmRed : vmDefault_Color ) : vmDefault_Color );
	ui->txtBuyTotalPrice->highlight ( editing_action.isOn () ? ( buyTotalPrice.isNull () ? vmRed : vmDefault_Color ) : vmDefault_Color );
	ui->dteBuyDate->highlight ( editing_action.isOn () ? ( vmNumber ( ui->dteBuyDate->date () ).isDateWithinRange ( vmNumber::currentDate (), 0, 4 ) ? vmDefault_Color : vmRed ) : vmDefault_Color );
	ui->dteBuyDeliveryDate->highlight ( editing_action.isOn () ? ( vmNumber ( ui->dteBuyDeliveryDate->date () ).isDateWithinRange ( vmNumber::currentDate (), 0, 4 ) ? vmDefault_Color : vmRed ) : vmDefault_Color );
	ui->txtBuyTotalPaid->highlight ( buyTotalPrice > buy->price ( FLD_BUY_TOTALPAID ) ? vmYellow : vmDefault_Color );

	ui->tableBuyItems->scrollTo ( ui->tableBuyItems->indexAt ( QPoint ( 0, 0 ) ) );
	ui->tableBuyPayments->scrollTo ( ui->tableBuyPayments->indexAt ( QPoint ( 0, 0 ) ) );
}

bool MainWindow::saveBuy ( buyListItem* buy_item, const bool b_dbsave )
{
	if ( buy_item )
	{
		Buy* buy ( buy_item->buyRecord () );
		if ( buy->saveRecord ( false, b_dbsave ) ) // do not change indexes just now. Wait for dbCalendar actions
		{
			mCal->updateCalendarWithBuyInfo ( buy );
			buy_item->setAction ( ACTION_READ, true ); // now we can change indexes

			dbSupplies sup_rec ( true );
			ui->tableBuyItems->exportPurchaseToSupplies ( buy, &sup_rec );
			ui->tableBuyPayments->setTableUpdated ();
			updateBuyTotalPriceWidgets ( buy_item );

			ui->cboBuySuppliers->insertItemSorted ( recStrValue ( buy, FLD_BUY_SUPPLIER ) );
			cboBuySuppliers_indexChanged ( ui->cboBuySuppliers->findText ( ui->cboBuySuppliers->text() ) ); // update suppliers' list with - possibly - a new entry

			NOTIFY ()->notifyMessage ( TR_FUNC ( "Record saved" ), recStrValue ( buy_item->buyRecord (), FLD_BUY_SUPPLIER ) +
										  TR_FUNC ( " purchase info saved!" ) );
			controlBuyForms ( buy_item );
			changeNavLabels ();
			removeEditItem ( static_cast<dbListItem*>( buy_item ) );
			return true;
		}
	}
	return false;
}

buyListItem* MainWindow::addBuy ( clientListItem* client_parent, jobListItem* job_parent )
{
	auto buy_item ( new buyListItem );
	buy_item->setRelation ( RLI_CLIENTITEM );
	buy_item->setRelatedItem ( RLI_CLIENTPARENT, client_parent );
	buy_item->setRelatedItem ( RLI_JOBPARENT, job_parent );
	static_cast<void>(buy_item->loadData ( false ));
	buy_item->setAction ( ACTION_ADD, true );
	client_parent->buys->append ( buy_item );

	auto buy_item2 ( new buyListItem );
	buy_item2->setRelation ( RLI_JOBITEM );
	buy_item->syncSiblingWithThis ( buy_item2 );
	buy_item2->addToList ( ui->buysJobList, false );
	job_parent->buys->append ( buy_item2 );
	
	buy_item->addToList ( ui->buysList );
	setRecValue ( buy_item->buyRecord (), FLD_BUY_CLIENTID, recStrValue ( client_parent->clientRecord (), FLD_CLIENT_ID ) );
	setRecValue ( buy_item->buyRecord (), FLD_BUY_JOBID, recStrValue ( job_parent->jobRecord (), FLD_JOB_ID ) );
	buy_item->setDBRecID ( static_cast<uint>( recIntValue ( buy_item->buyRecord (), FLD_BUY_ID ) ) );
	buy_item2->setDBRecID ( static_cast<uint>( recIntValue ( buy_item->buyRecord (), FLD_BUY_ID ) ) );
	insertEditItem ( static_cast<dbListItem*>( buy_item ) );
	
	ui->cboBuySuppliers->setFocus ();
	return buy_item;
}

bool MainWindow::editBuy ( buyListItem* buy_item, const bool b_dogui )
{
	if ( buy_item )
	{
		if ( buy_item->action () == ACTION_READ )
		{
			buy_item->setAction ( ACTION_EDIT, true );
			insertEditItem ( static_cast<dbListItem*>( buy_item ) );
			if ( b_dogui )
			{
				controlBuyForms ( buy_item );
				ui->txtBuyTotalPrice->setFocus ();
			}
			return true;
		}
	}
	return false;
}

bool MainWindow::delBuy ( buyListItem* buy_item, const bool b_ask )
{
	if ( buy_item )
	{
		if ( b_ask )
		{
			const QString text ( TR_FUNC ( "Are you sure you want to remove this purchase from %1 (%2)?" ) );
			if ( !NOTIFY ()->questionBox ( TR_FUNC ( "Question" ), text.arg ( recStrValue ( buy_item->buyRecord (), FLD_BUY_SUPPLIER ), recStrValue ( buy_item->buyRecord (), FLD_BUY_PRICE ) ) ) )
				return false;
		}
		buy_item->setAction ( ACTION_DEL, true );
		mCal->updateCalendarWithBuyInfo ( buy_item->buyRecord () );
		return cancelBuy ( buy_item );
	}
	return false;
}

bool MainWindow::cancelBuy ( buyListItem* buy_item )
{
	if ( buy_item )
	{
		switch ( buy_item->action () )
		{
			case ACTION_ADD:
			case ACTION_DEL:
			{
				auto buy_jobitem ( static_cast<buyListItem*>( buy_item->relatedItem ( RLI_JOBITEM ) ) );
				ui->buysJobList->setIgnoreChanges ( true );
				static_cast<jobListItem*>( buy_item->relatedItem ( RLI_JOBPARENT ) )->buys->remove ( buy_jobitem->row () );
				removeListItem ( buy_jobitem, false, false );
				ui->buysJobList->setIgnoreChanges ( false );
			
				buy_item->setAction ( ACTION_DEL, true );
				updateBuyTotalPriceWidgets ( buy_item );
				static_cast<clientListItem*>( buy_item->relatedItem ( RLI_CLIENTPARENT ) )->buys->remove ( buy_item->row () );
				mBuyCurItem = nullptr;
				removeListItem ( buy_item );
				
				// If the list is empty, no other item will be selected. We won't have a mBuyCurItem. If the deleted item
				// was the first to be selected, no prev item also, so we will land in an undefined state unless we force
				// a null purchase object pointer through
				if ( ui->buysJobList->isEmpty () )
					displayBuy ( nullptr );
			}
			break;
			case ACTION_EDIT:
				removeEditItem ( static_cast<dbListItem*>( buy_item ) );
				buy_item->setAction ( ACTION_REVERT, true );
				displayBuy ( buy_item );
			break;
			case ACTION_NONE:
			case ACTION_READ:
			case ACTION_REVERT:
				return false;
		}
		return true;
	}
	return false;
}

void MainWindow::displayBuy ( buyListItem* buy_item, const bool b_select )
{
	if ( buy_item )
	{
		//buy_item might be a calendar item, one of the extra items. We need to use the main item
		buyListItem* parent_buy ( static_cast<buyListItem*>( buy_item->relatedItem ( RLI_CLIENTITEM ) ) );
		if ( !parent_buy ) // if this happens, something is definetely off. Should not happen
			parent_buy = buy_item;

		if ( parent_buy->loadData ( true ) )
		{
			if ( parent_buy != mBuyCurItem )
			{
				if ( b_select ) // Sync both lists, but make sure no signals are emitted
				{
					ui->buysList->setCurrentItem ( parent_buy, true, false );
					ui->buysJobList->setCurrentItem ( parent_buy->relatedItem ( RLI_JOBITEM ), true, false );
				}
				alterLastViewedRecord ( mBuyCurItem ? ( mBuyCurItem->relatedItem ( RLI_JOBPARENT ) == parent_buy->relatedItem ( RLI_JOBPARENT ) ?
										mBuyCurItem : nullptr ) : nullptr, buy_item );
				mBuyCurItem = parent_buy;
				if ( parent_buy->relatedItem ( RLI_EXTRAITEMS ) != nullptr )
					ui->lstBuySuppliers->setCurrentItem ( static_cast<buyListItem*>( parent_buy->relatedItem ( RLI_EXTRAITEMS ) ), true, false );
			}
			loadBuyInfo ( parent_buy->buyRecord () );
			controlBuyForms ( parent_buy );
		}
	}
	else
	{
		mBuyCurItem = nullptr;
		loadBuyInfo ( nullptr );
		controlBuyForms ( nullptr );
	}
	if ( b_select )
		changeNavLabels ();
}

void MainWindow::loadBuyInfo ( const Buy* const buy )
{
	if ( buy )
	{
		ui->txtBuyID->setText ( recStrValue ( buy, FLD_BUY_ID ) );
		ui->txtBuyTotalPrice->setText ( recStrValue ( buy, FLD_BUY_PRICE ) );
		ui->txtBuyTotalPaid->setText ( recStrValue ( buy, FLD_BUY_TOTALPAID ) );
		ui->txtBuyNotes->setText ( recStrValue ( buy, FLD_BUY_NOTES ) );
		ui->txtBuyDeliveryMethod->setText ( recStrValue ( buy, FLD_BUY_DELIVERMETHOD ) );
		ui->dteBuyDate->setDate ( buy->date ( FLD_BUY_DATE ) );
		ui->dteBuyDeliveryDate->setDate ( buy->date ( FLD_BUY_DELIVERDATE ) );
		ui->tableBuyItems->loadFromStringTable ( recStrValue ( buy, FLD_BUY_REPORT ) );
		ui->tableBuyPayments->loadFromStringTable ( recStrValue ( buy, FLD_BUY_PAYINFO ) );

		const int cur_idx ( ui->cboBuySuppliers->findText ( recStrValue ( buy, FLD_BUY_SUPPLIER ) ) );
		ui->cboBuySuppliers->setCurrentIndex ( cur_idx );
	}
	else
	{
		ui->tableBuyItems->clear ( true );
		ui->tableBuyPayments->clear ( true );
		ui->txtBuyID->setText ( QStringLiteral ( "-1" ) );
		ui->txtBuyTotalPrice->setText ( vmNumber::zeroedPrice.toPrice () );
		ui->txtBuyTotalPaid->setText ( vmNumber::zeroedPrice.toPrice () );
		ui->txtBuyNotes->setText ( emptyString );
		ui->txtBuyDeliveryMethod->setText ( emptyString );
		ui->dteBuyDate->setText ( emptyString );
		ui->dteBuyDeliveryDate->setText ( emptyString );
		ui->cboBuySuppliers->setText ( emptyString );
	}
}

buyListItem* MainWindow::getBuyItem ( const clientListItem* const parent_client, const uint id ) const
{
	if ( id >= 1 && parent_client )
	{
		uint i ( 0 );
		while ( i < parent_client->buys->count () )
		{
			if ( parent_client->buys->at ( i )->dbRecID () == id )
				return parent_client->buys->at ( i );
			++i;
		}
	}
	return nullptr;
}

void MainWindow::fillJobBuyList ( const jobListItem* parent_job )
{
	ui->buysJobList->clear ();
	//ui->tableBuyItemsSelectiveView->setIgnoreChanges ( true );
	ui->tableBuyItemsSelectiveView->clear ();
	if ( parent_job )
	{
		vmNumber totalPrice, totalPaid;
		buyListItem* buy_item ( nullptr );
		for ( uint i ( 0 ); i < parent_job->buys->count (); ++i )
		{
			buy_item = parent_job->buys->at ( i );
			buy_item->addToList ( ui->buysJobList );
			totalPrice += buy_item->buyRecord ()->price ( FLD_BUY_PRICE );
			totalPaid += buy_item->buyRecord ()->price ( FLD_BUY_TOTALPAID );
			ui->tableBuyItemsSelectiveView->loadFromStringTable ( recStrValue ( buy_item->buyRecord (), FLD_BUY_REPORT ), true );
		}
		ui->txtBuyTotalPurchasePriceForJob->setText ( totalPrice.toPrice () );
		ui->txtBuyTotalPaidForJob->setText ( totalPaid.toPrice () );
		ui->txtBuySelectiveViewTotal->setText ( totalPrice.toPrice () );
		ui->txtBuySelectiveViewSelected->setText ( vmNumber::zeroedPrice.toPrice () );
	}
	//ui->tableBuyItemsSelectiveView->setIgnoreChanges ( false );
	ui->buysJobList->setIgnoreChanges ( false );
}

void MainWindow::interceptBuyItemsCellChange ( const vmTableItem* const item )
{
	const auto row ( static_cast<uint>(item->row ()) );
	if ( static_cast<int>(row) == ui->tableBuyItems->totalsRow () )
		return;

	stringTable buy_items;
	ui->tableBuyItems->makeStringTable ( buy_items );
	setRecValue ( mBuyCurItem->buyRecord (), FLD_BUY_REPORT, buy_items.toString () );
	postFieldAlterationActions ( mBuyCurItem );
}

/* The highlight calls here are only meant as a visual aid, and do not prevent the information being saved.
 * Perhaps in the future I will add a NOTIFY ()->notifyMessage ()
*/
void MainWindow::interceptBuyPaymentCellChange ( const vmTableItem* const item )
{
	const auto row ( static_cast<uint>(item->row ()) );
	if ( static_cast<int>(row) == ui->tableBuyPayments->totalsRow () )
		return;

	const auto col ( static_cast<uint>(item->column ()) );
	switch ( col )
	{
		case PHR_ACCOUNT: return; // never reached
		case PHR_DATE:
		case PHR_USE_DATE:
		{
			const vmNumber& cell_date ( item->date () );
			const bool bDateOk ( cell_date.isDateWithinRange ( vmNumber::currentDate (), 0, 4 ) );
			ui->tableBuyPayments->sheetItem ( row, col )->highlight ( bDateOk ? vmDefault_Color : vmRed );
		}
		break;
		default:
		{
			const bool bPaid ( ui->tableBuyPayments->sheetItem ( row, PHR_PAID )->text () == CHR_ONE );
			const bool bValueOk ( bPaid == !ui->tableBuyPayments->sheetItem ( row, PHR_VALUE )->text ().isEmpty () );
			const bool bMethodOk ( bValueOk == !ui->tableBuyPayments->sheetItem ( row, PHR_METHOD )->text ().isEmpty () );
			ui->tableBuyPayments->sheetItem ( row, PHR_VALUE )->highlight ( bValueOk ? vmDefault_Color : vmRed );
			ui->tableBuyPayments->sheetItem ( row, PHR_METHOD )->highlight ( bMethodOk ? vmDefault_Color : vmRed );
			if ( col != PHR_METHOD )
			{
				updateBuyTotalPaidValue ();
				if ( bPaid && !ui->tableBuyPayments->sheetItem ( row, PHR_DATE )->cellIsAltered () )
					ui->tableBuyPayments->sheetItem ( row, PHR_DATE )->setDate ( vmNumber::currentDate () );
			}
			else
			{
				if ( bMethodOk )
					COMPLETERS ()->updateCompleter ( ui->tableBuyPayments->sheetItem ( row, PHR_METHOD )->text (), CC_PAYMENT_METHOD );
			}
		}
		break;
	}

	if ( signed( row + 1 ) >= ui->tableBuyPayments->lastUsedRow () )
		setRecValue ( mBuyCurItem->buyRecord (), FLD_BUY_TOTALPAYS, QString::number ( row + 1 ) );

	stringTable pay_data;
	ui->tableBuyPayments->makeStringTable ( pay_data );
	setRecValue ( mBuyCurItem->buyRecord (), FLD_BUY_PAYINFO, pay_data.toString () );
	postFieldAlterationActions ( mBuyCurItem );
}

void MainWindow::updateBuyTotalPaidValue ()
{
	vmNumber total;
	for ( int i ( 0 ); i <= ui->tableBuyPayments->lastUsedRow (); ++i )
	{
		if ( ui->tableBuyPayments->sheetItem ( static_cast<uint>(i), PHR_PAID )->text () == CHR_ONE )
			total += vmNumber ( ui->tableBuyPayments->sheetItem ( static_cast<uint>(i), PHR_VALUE )->text (), VMNT_PRICE );
	}
	ui->txtBuyTotalPaid->setText ( total.toPrice () );
	setRecValue ( mBuyCurItem->buyRecord (), FLD_BUY_TOTALPAID, total.toPrice () );
}

void MainWindow::updateBuySelectionTotals ()
{
	const QList<QTableWidgetItem*>& selItens ( ui->tableBuyItemsSelectiveView->selectedItems () );
	if ( selItens.count () > 0 )
	{
		QList<QTableWidgetItem*>::const_iterator itr ( selItens.constBegin () );
		const QList<QTableWidgetItem*>::const_iterator& itr_end ( selItens.constEnd () );
		vmNumber total_price;
		for ( ; itr != itr_end; ++itr )
		{
			if ( (*itr)->column () == ISR_TOTAL_PRICE )
			{
				total_price += vmNumber ( (*itr)->text(), VMNT_PRICE, 1 );
			}
		}
		ui->txtBuySelectiveViewSelected->setText ( total_price.toPrice () );
	}
}

void MainWindow::updateBuyFilteredViewTotals ()
{
	if ( !Sys_Init::EXITING_PROGRAM )
	{
		vmNumber total_price;
		for ( int i_row ( 0 ); i_row <= ui->tableBuyItemsSelectiveView->lastUsedRow (); ++i_row )
		{
			if ( !ui->tableBuyItemsSelectiveView->isRowHidden ( i_row ) )
			{
				total_price += vmNumber ( ui->tableBuyItemsSelectiveView->sheetItem
					( static_cast<uint>(i_row), ISR_TOTAL_PRICE )->text (), VMNT_PRICE, 1 );
			}
		}
		ui->txtBuySelectiveViewTotal->setText ( total_price.toPrice () );
	}
}

void MainWindow::getPurchasesForSuppliers ( const QString& supplier )
{
	static QString current_supplier;
	if ( current_supplier != supplier )
		ui->lstBuySuppliers->clear ( true, true );
	current_supplier = supplier;
	Buy buy;
	buyListItem* new_buyitem ( nullptr );

	buy.stquery.str_query = QStringLiteral ( "SELECT * FROM PURCHASES WHERE `SUPPLIER`='" ) + supplier + CHR_CHRMARK;
	if ( buy.readFirstRecord ( FLD_BUY_SUPPLIER, supplier, false ) )
	{
		do
		{
			new_buyitem = new buyListItem;
			new_buyitem->setRelation ( RLI_EXTRAITEMS );
			new_buyitem->setLastRelation ( RLI_EXTRAITEMS );
			new_buyitem->setDBRecID ( static_cast<uint>( recIntValue ( &buy, FLD_BUY_ID ) ) );
			new_buyitem->setRelatedItem ( RLI_CLIENTPARENT, getClientItem ( static_cast<uint>( recIntValue ( &buy, FLD_BUY_CLIENTID ) ) ) );
			new_buyitem->addToList ( ui->lstBuySuppliers );
		} while ( buy.readNextRecord ( true, false ) );
	}
}

void MainWindow::setBuyPayValueToBuyPrice ( const QString& price )
{
	int row ( ui->tableBuyPayments->lastUsedRow () );
	if ( row < 0 )
	{
		row = 0;
		ui->tableBuyPayments->setCurrentCell ( 0, 0, QItemSelectionModel::ClearAndSelect );
	}
	ui->tableBuyPayments->sheetItem ( static_cast<uint>( row ), PHR_VALUE )->setText ( price, true );
	ui->tableBuyPayments->sheetItem ( static_cast<uint>( row ), PHR_DATE )->setDate ( vmNumber ( ui->dteBuyDate->date () ) );
}

void MainWindow::buyExternalChange ( const uint id, const RECORD_ACTION action )
{
	QScopedPointer<Buy> buy ( new Buy );
	if ( !buy->readRecord ( id ) )
		return;
	clientListItem* client_parent ( getClientItem ( static_cast<uint>( recIntValue ( buy, FLD_BUY_CLIENTID ) ) ) );
	if ( !client_parent )
		return;

	if ( action == ACTION_EDIT )
	{	
		buyListItem* buy_item ( getBuyItem ( client_parent, id ) );
		if ( buy_item )
		{
			if ( buy_item->buyRecord () != nullptr )
				buy_item->buyRecord ()->setRefreshFromDatabase ( true ); // if buy has been loaded before, tell it to ignore the cache and to load data from the database upon next read
			buy_item->update ();
			if ( id == mBuyCurItem->dbRecID () )
				displayBuy ( buy_item );
		}
	}
	else
	{
		jobListItem* job_parent ( getJobItem ( client_parent, static_cast<uint>( recIntValue ( buy, FLD_BUY_JOBID ) ) ) );
		if ( static_cast<uint>( recIntValue ( buy, FLD_BUY_CLIENTID ) ) != mClientCurItem->dbRecID () )
			displayClient ( client_parent, true, job_parent );
		else if ( static_cast<uint>( recIntValue ( buy, FLD_BUY_JOBID ) ) != mJobCurItem->dbRecID () )
			displayJob ( job_parent, true );

		if ( action == ACTION_ADD )
		{
			buyListItem* new_buy_item ( addBuy ( client_parent, job_parent ) );
			Buy* new_buy ( new_buy_item->buyRecord () );
			new_buy->setRefreshFromDatabase ( true );
			new_buy->readRecord ( id );
			static_cast<void>( saveBuy ( new_buy_item, false ) );
		}
		else if ( action == ACTION_DEL )
		{
			delBuy ( getBuyItem ( client_parent, id ), false );
		}
	}
}
//--------------------------------------------BUY------------------------------------------------------------

//--------------------------------------------EDITING-FINISHED-BUY-----------------------------------------------------------	
void MainWindow::txtBuy_textAltered ( const vmWidget* const sender )
{
	bool input_ok ( true );
	if ( static_cast<uint>( sender->id () ) == FLD_BUY_PRICE )
	{
		const vmNumber price ( sender->text (), VMNT_PRICE, 1 );
		input_ok = price > 0;
		mBuyCurItem->setFieldInputStatus ( FLD_BUY_PRICE, input_ok, ui->txtBuyTotalPrice );
		setBuyPayValueToBuyPrice ( sender->text () );
	}
	setRecValue ( mBuyCurItem->buyRecord (), static_cast<uint>( sender->id () ), sender->text () );
	postFieldAlterationActions ( mBuyCurItem );
}

void MainWindow::dteBuy_dateAltered ( const vmWidget* const sender )
{
	const vmNumber date ( dynamic_cast<const vmDateEdit* const>( sender )->date () );
	const bool input_ok ( date.isDateWithinRange ( vmNumber::currentDate (), 0, 4 ) );
	{
		setRecValue ( mBuyCurItem->buyRecord (), static_cast<uint>( sender->id () ), date.toDate ( vmNumber::VDF_DB_DATE ) );
		mBuyCurItem->update ();
	}
	mBuyCurItem->setFieldInputStatus ( static_cast<uint>( sender->id () ), input_ok, sender );
	postFieldAlterationActions ( mBuyCurItem );
}

void MainWindow::cboBuySuppliers_textAltered ( const QString& text )
{
	int idx ( -2 );
	if ( !text.isEmpty () )
	{
		setRecValue ( mBuyCurItem->buyRecord (), FLD_BUY_SUPPLIER, ui->cboBuySuppliers->text () );
		mBuyCurItem->update ();
		idx = ui->cboBuySuppliers->findText ( text );
	}
	cboBuySuppliers_indexChanged ( idx );
	mBuyCurItem->setFieldInputStatus ( FLD_BUY_SUPPLIER, idx != -2, ui->cboBuySuppliers );
	postFieldAlterationActions ( mBuyCurItem );
}

void MainWindow::cboBuySuppliers_indexChanged ( const int idx )
{
	ui->lstBuySuppliers->setIgnoreChanges ( true );
	if ( idx != -1 )
	{
		const QString supplier ( ui->cboBuySuppliers->itemText ( idx ) );
		getPurchasesForSuppliers ( supplier );
		ui->lblBuySupplierBuys->setText ( TR_FUNC ( "Purchases from " ) + supplier );
		SUPPLIERS ()->displaySupplier ( supplier, false );
	}
	else
	{
		ui->lblBuySupplierBuys->setText ( TR_FUNC ( "Supplier list empty" ) );
		ui->lstBuySuppliers->clear ( true, true );
	}
	ui->lstBuySuppliers->setIgnoreChanges ( false );
}
//--------------------------------------------EDITING-FINISHED-BUY-----------------------------------------------------------

//----------------------------------SETUP-CUSTOM-CONTROLS-NAVIGATION--------------------------------------
void MainWindow::setupCustomControls ()
{
	APP_RESTORER ()->addRestoreInfo ( m_crash = new crashRestore ( QStringLiteral ( "vmmain" ) ) );

	setupClientPanel ();
	setupJobPanel ();
	setupPayPanel ();
	setupBuyPanel ();
	reOrderTabSequence ();
	setupWorkFlow ();

	static_cast<void>( connect ( ui->btnReportGenerator, &QToolButton::clicked, this, [&] () { return btnReportGenerator_clicked (); } ) );
	static_cast<void>( connect ( ui->btnBackupRestore, &QToolButton::clicked, this, [&] () { return btnBackupRestore_clicked (); } ) );
	static_cast<void>( connect ( ui->btnSearch, &QToolButton::clicked, this, [&] () { return btnSearch_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCalculator, &QToolButton::clicked, this, [&] () { return btnCalculator_clicked (); } ) );
	static_cast<void>( connect ( ui->btnEstimates, &QToolButton::clicked, this, [&] () { return btnEstimates_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCompanyPurchases, &QToolButton::clicked, this, [&] () { return btnCompanyPurchases_clicked (); } ) );
	static_cast<void>( connect ( ui->btnConfiguration, &QToolButton::clicked, this, [&] () { return btnConfiguration_clicked (); } ) );
	static_cast<void>( connect ( ui->btnExitProgram, &QToolButton::clicked, this, [&] () { return btnExitProgram_clicked (); } ) );

	installEventFilter ( this );
}

void MainWindow::restoreCrashedItems ( crashRestore* const crash, clientListItem* &client_item, jobListItem* &job_item, buyListItem* &buy_item)
{
	const stringRecord* __restrict savedRec ( nullptr );
	if ( (savedRec = &crash->restoreFirstState ()) )
	{
		bool ok ( true );
		int type_id ( -1 );
		RECORD_ACTION action ( ACTION_NONE );
		payListItem* pay_item ( nullptr );
		dbListItem* item ( nullptr );
		
		while ( savedRec->isOK () )
		{
			type_id = savedRec->fieldValue ( CF_SUBTYPE ).toInt ( &ok );
			if ( ok && type_id >= 0 )
			{
				action = static_cast<RECORD_ACTION>( savedRec->fieldValue ( CF_ACTION ).toInt ( &ok ) );

				switch ( type_id )
				{
					case CLIENT_TABLE:
						if ( action == ACTION_EDIT )
						{
							client_item = getClientItem ( savedRec->fieldValue ( CF_DBRECORD + FLD_CLIENT_ID ).toUInt ( &ok ) );
							if ( ( ok = (client_item != nullptr) ) )
							{
								client_item->loadData ( true );
								static_cast<void>( editClient ( client_item, false ) );
							}
						}
						else
							client_item = addClient ();
						item = client_item;
					break;
					case JOB_TABLE:
						client_item = getClientItem ( savedRec->fieldValue ( CF_DBRECORD + FLD_JOB_CLIENTID ).toUInt ( &ok ) );
						if ( ( ok = (client_item != nullptr) ) )
						{
							if ( action == ACTION_EDIT )
							{
								job_item = getJobItem ( client_item, savedRec->fieldValue ( CF_DBRECORD + FLD_JOB_ID ).toUInt ( &ok ) );
								if ( ( ok = (job_item != nullptr) ) )
								{
									job_item->loadData ( true );
									static_cast<void>( editJob ( job_item, false ) );
								}
							}
							else
								job_item = addJob ( client_item );
							item = job_item;
						}
					break;
					case PAYMENT_TABLE:
						client_item = getClientItem ( savedRec->fieldValue ( CF_DBRECORD + FLD_PAY_CLIENTID ).toUInt ( &ok ) );
						if ( ( ok = (client_item != nullptr) ) )
						{
							job_item = getJobItem ( client_item, savedRec->fieldValue ( CF_DBRECORD + FLD_PAY_JOBID ).toUInt ( &ok ) );
							if ( ( ok = (job_item != nullptr) ) )
							{
								if ( action == ACTION_EDIT )
								{
									pay_item = getPayItem ( client_item, savedRec->fieldValue ( CF_SUBTYPE + FLD_PAY_ID ).toUInt ( &ok ) );
									if ( !pay_item )
										pay_item = job_item->payItem ();
									pay_item->loadData ( true );
									static_cast<void>( editPay ( pay_item, false ) );
								}
								else
									pay_item = job_item->payItem ();
								item = pay_item;
							}
						}
					break;
					case PURCHASE_TABLE:
						client_item = getClientItem ( savedRec->fieldValue ( CF_DBRECORD + FLD_BUY_CLIENTID ).toUInt ( &ok ) );
						if ( ( ok = (client_item != nullptr ) ) )
						{
							job_item = getJobItem ( client_item, savedRec->fieldValue ( CF_DBRECORD + FLD_BUY_JOBID ).toUInt ( &ok ) );
							if ( ( ok = (job_item != nullptr) ) )
							{
								if ( action == ACTION_EDIT )
								{
									buy_item = getBuyItem ( client_item, savedRec->fieldValue ( CF_DBRECORD + FLD_BUY_ID ).toUInt ( &ok ) );
									if ( ( ok = (buy_item != nullptr) ) )
									{
										buy_item->loadData ( true );
										static_cast<void>( editBuy ( buy_item, false ) );
									}
								}
								else
									buy_item = addBuy ( client_item, job_item );
								item = buy_item;
							}
						}
					break;
					default:
					break;
				}
			}

			int n_fails ( 0 );
			if ( item != nullptr )
			{
				item->setCrashID ( savedRec->fieldValue ( CF_CRASHID ).toInt () );
				item->dbRec ()->fromStringRecord ( *savedRec, CF_DBRECORD );
				item = nullptr;
			}
			else if ( !ok )
			{
				n_fails = 1;
				n_fails += crash->eliminateRestoreInfo ( savedRec->fieldValue ( CF_CRASHID ).toInt ( &ok ) ) ? -1 : 1 ;
			}

			savedRec = &crash->restoreNextState ();
			if ( n_fails == 2 )
			{
				if ( !savedRec->isOK () ) // third fail. Corrupt crash file. Eliminate it and be done with it
					static_cast<void>( crash->eliminateRestoreInfo ( -1 ) );
			}
		}
	}
}

void MainWindow::restoreLastSession ()
{
	ui->tabMain->setCurrentIndex ( 0 );
	
	clientListItem* client_item ( nullptr );
	jobListItem* job_item ( nullptr );
	buyListItem* buy_item ( nullptr );
	
	loadClients ( client_item );
	if ( !client_item )
		client_item = static_cast<clientListItem*>( ui->clientsList->item ( ui->clientsList->count () - 1 ) );

	if ( m_crash->needRestore () )
	{
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Session Manager" ), TR_FUNC ( "Restoring last session due to unclean program termination" ) );
		restoreCrashedItems ( m_crash, client_item, job_item, buy_item );
	}

	findSectionByScrollPosition ( 0 ); // when I implement the code to restore screen controls' position, this function will not be called with a hard-coded number
	displayClient ( client_item, true, job_item, buy_item );
	ui->clientsList->setFocus ();
}

void MainWindow::reOrderTabSequence ()
{
	setTabOrder ( ui->chkClientActive, ui->contactsClientPhones->combo () );
	setTabOrder ( ui->contactsClientPhones->combo (), ui->contactsClientEmails->combo () );
	setTabOrder ( ui->contactsClientEmails->combo (), ui->dteClientStart );
	setTabOrder ( ui->dteClientStart, ui->dteClientEnd );
	
	setTabOrder ( ui->btnQuickProject, ui->txtJobKeyWords );
	setTabOrder ( ui->txtJobKeyWords, ui->lstJobKeyWords );
	setTabOrder ( ui->btnJobNextDay, ui->dteJobAddDate );
	setTabOrder ( ui->dteJobAddDate, ui->btnJobAddDay );
	setTabOrder ( ui->btnJobSeparatePicture, jobImageViewer );
	
	setTabOrder ( ui->buysJobList, ui->dteBuyDate );
	setTabOrder ( ui->dteBuyDate, ui->dteBuyDeliveryDate );
	setTabOrder ( ui->dteBuyDeliveryDate, ui->txtBuyDeliveryMethod );
}

void MainWindow::changeSchemeStyle ( const QString& style, const bool b_save )
{
	mainTaskPanel->setScheme ( style );
	vmWidget::changeThemeColors ( mainTaskPanel->currentScheme ()->colorStyle1, mainTaskPanel->currentScheme ()->colorStyle2 );
	if ( b_save )
		return CONFIG ()->setValue ( Ui::mw_configSectionName, Ui::mw_configCategoryAppScheme, style );
}

void MainWindow::setupWorkFlow ()
{
	grpClients = mainTaskPanel->createGroup ( TR_FUNC ( "CLIENT INFORMATION" ), true, false );
	grpClients->setMinimumHeight ( ui->frmClientInfo->sizeHint ().height () + 1 );
	grpClients->addQEntry ( ui->frmClientInfo, new QHBoxLayout );

	grpJobs = mainTaskPanel->createGroup ( TR_FUNC ( "JOB REPORT AND IMAGES" ), true, false );
	grpJobs->setMinimumHeight ( ui->frmJobInfo->sizeHint ().height () + ui->frmJobInfo_2->sizeHint ().height () );
	grpJobs->addQEntry ( ui->frmJobInfo, new QHBoxLayout );
	grpJobs->addQEntry ( ui->frmJobInfo_2, new QHBoxLayout );

	grpPays = mainTaskPanel->createGroup ( TR_FUNC ( "PAYMENTS FOR JOB" ), true, false );
	grpPays->setMinimumHeight ( ui->frmPayInfo->sizeHint ().height () + 1 );
	grpPays->addQEntry ( ui->frmPayInfo, new QHBoxLayout );

	grpBuys = mainTaskPanel->createGroup ( TR_FUNC ( "PURCHASES FOR JOB" ), true, false );
	grpBuys->setMinimumHeight ( ui->splitterBuyInfo->sizeHint ().height () + 1 );
	grpBuys->addQEntry ( ui->splitterBuyInfo, new QHBoxLayout );

	ui->scrollWorkFlow->setWidget ( mainTaskPanel );
	setupSectionNavigation ();

	mCalendarView = new calendarViewUI ( ui->tabMain, TI_CALENDAR, this, mCal );

	ui->tabMain->removeTab ( 7 );
	ui->tabMain->removeTab ( 6 );
	ui->tabMain->removeTab ( 5 );
	ui->tabMain->removeTab ( 4 );
}

void MainWindow::setupSectionNavigation ()
{
	createFloatToolBar ();
	
	clientSectionPos = grpClients->pos ().y () + grpClients->height () - 100;
	jobSectionPos = grpJobs->pos ().y () + grpJobs->height () - 100;
	paySectionPos = grpPays->pos ().y () + grpPays->height () - 200;
	buySectionPos = grpBuys->pos ().y () + grpBuys->height () - 100;

	ui->lblCurInfoClient->setCallbackForLabelActivated ( [&] ()
	{
		if ( grpActive != grpClients )
		{
			ui->scrollWorkFlow->verticalScrollBar ()->setValue ( grpClients->pos ().y () );
			ui->clientsList->setFocus ();
			ui->tabMain->setCurrentIndex ( static_cast<int>( TI_MAIN ) );
		}
	} );

	ui->lblCurInfoJob->setCallbackForLabelActivated ( [&] ()
	{
		if ( grpActive != grpJobs )
		{
			ui->scrollWorkFlow->verticalScrollBar ()->setValue ( grpJobs->pos ().y () );
			ui->jobsList->setFocus ();
			ui->tabMain->setCurrentIndex ( static_cast<int>( TI_MAIN ) );
		}
	} );

	ui->lblCurInfoPay->setCallbackForLabelActivated ( [&] ()
	{
		if ( grpActive != grpPays )
		{
			ui->scrollWorkFlow->verticalScrollBar ()->setValue ( grpPays->pos ().y () );
			ui->paysList->setFocus ();
			ui->tabMain->setCurrentIndex ( static_cast<int>( TI_MAIN ) );
		}
	} );

	ui->lblCurInfoBuy->setCallbackForLabelActivated ( [&] ()
	{
		if ( grpActive != grpBuys )
		{
			ui->scrollWorkFlow->verticalScrollBar ()->setValue ( grpBuys->pos ().y () );
			ui->buysList->setFocus ();
			ui->tabMain->setCurrentIndex ( static_cast<int>( TI_MAIN ) );
		}
	} );

	static_cast<void>( connect ( ui->scrollWorkFlow->verticalScrollBar (), &QScrollBar::valueChanged, 
						this, [&] ( const int value ) { findSectionByScrollPosition ( value ); } ) );
	setupTabNavigationButtons ();
}

void MainWindow::findSectionByScrollPosition ( const int scrollBar_value )
{
	static vmActionLabel* curLabel ( nullptr ), *prevLabel ( nullptr );
	
	prevLabel = curLabel;
	if ( scrollBar_value <= ( clientSectionPos ) )
	{
		grpActive = grpClients;
		curLabel = ui->lblCurInfoClient;
	}
	else if ( scrollBar_value <= ( jobSectionPos ) )
	{
		grpActive = grpJobs;
		curLabel = ui->lblCurInfoJob;
	}
	else if ( scrollBar_value <= ( paySectionPos ) )
	{
		grpActive = grpPays;
		curLabel = ui->lblCurInfoPay;
	}
	else if ( scrollBar_value <= ( buySectionPos  ) )
	{
		grpActive = grpBuys;
		curLabel = ui->lblCurInfoBuy;
	}
	
	if ( prevLabel != curLabel )
	{
		curLabel->highlight ( true );
		changeFuncActionPointers ( grpActive );
		if ( prevLabel )
			prevLabel->highlight ( false );
	}
}

void MainWindow::changeFuncActionPointers ( vmActionGroup* grpActive )
{
	static QAction* sepAction ( nullptr );
	QAction *newSep ( nullptr );
	
	if ( grpActive == grpClients )
	{
		curSectionAction = &sectionActions[0];
		func_Save = [&] ( dbListItem* ) { return saveClient ( mClientCurItem ); };
		func_Add = [&] () { return ( mClientCurItem = addClient () ); };
		func_Edit = [&] ( dbListItem* ) { return editClient ( mClientCurItem ); };
		func_Del = [&] ( dbListItem* ) { return delClient ( mClientCurItem ); };
		func_Cancel = [&] ( dbListItem* ) { return cancelClient ( mClientCurItem ); };
		ui->clientsList->setFocus ();
		activeRecord = static_cast<dbListItem*>( ui->clientsList->currentItem () );
	}
	else if ( grpActive == grpJobs )
	{
		curSectionAction = &sectionActions[1];
		func_Save = [&] ( dbListItem* ) { return saveJob ( mJobCurItem ); };
		func_Add = [&] () { return ( mJobCurItem = addJob ( mClientCurItem ) ); };
		func_Edit = [&] ( dbListItem* ) { return editJob ( mJobCurItem ); };
		func_Del = [&] ( dbListItem* ) { return delJob ( mJobCurItem ); };
		func_Cancel = [&] ( dbListItem* ) { return cancelJob ( mJobCurItem ); };
		ui->jobsList->setFocus ();
		activeRecord = static_cast<dbListItem*>( ui->jobsList->currentItem () );
		
		newSep = mActionsToolBar->addSeparator ();
		mActionsToolBar->addAction ( actJobSelectJob );
	}
	else if ( grpActive == grpPays )
	{
		curSectionAction = &sectionActions[2];
		func_Save = [&] ( dbListItem* ) { return savePay ( mPayCurItem ); };
		func_Add = nullptr;
		func_Edit = [&] ( dbListItem* ) { return editPay ( mPayCurItem ); };
		func_Del = [&] ( dbListItem* ) { return delPay ( mPayCurItem ); };
		func_Cancel = [&] ( dbListItem* ) { return cancelPay ( mPayCurItem ); };
		ui->paysList->setFocus ();
		activeRecord = static_cast<dbListItem*>( ui->paysList->currentItem () );
		
		newSep = mActionsToolBar->addSeparator ();
		mActionsToolBar->addAction ( actPayReceipt );
		mActionsToolBar->addAction ( actPayUnPaidReport );
		mActionsToolBar->addAction ( actPayReport );
	}
	else if ( grpActive == grpBuys )
	{
		curSectionAction = &sectionActions[3];
		func_Save = [&] ( dbListItem* ) { return saveBuy ( mBuyCurItem ); };
		func_Add = [&] () { return ( mBuyCurItem = addBuy ( mClientCurItem, mJobCurItem ) ); };
		func_Edit = [&] ( dbListItem* ) { return editBuy ( mBuyCurItem ); };
		func_Del = [&] ( dbListItem* ) { return delBuy ( mBuyCurItem ); };
		func_Cancel = [&] ( dbListItem* ) { return cancelBuy ( mBuyCurItem ); };
		ui->buysList->setFocus ();
		activeRecord = static_cast<dbListItem*>( ui->buysList->currentItem () );
	}

	if ( sepAction != newSep )
	{
		QList<QAction*> actions ( mActionsToolBar->actions () );
		QList<QAction*>::const_iterator itr ( actions.constBegin () );
		const QList<QAction*>::const_iterator& itr_end ( actions.constEnd () );
		bool b_beginRemoval ( false );

		for ( ; itr != itr_end; ++itr )
		{
			if ( *itr == sepAction )
				b_beginRemoval = true;
			if ( b_beginRemoval )
			{
				if ( *itr == newSep )
					break;
				mActionsToolBar->removeAction ( *itr );
			}
		}
		sepAction = newSep;
		mActionsToolBar->resize ( mActionsToolBar->width (), mActionsToolBar->sizeHint ().height () );
	}
	updateActionButtonsState ();
}

void MainWindow::updateActionButtonsState ()
{
	actionAdd->setEnabled ( curSectionAction->b_canAdd );
	actionEdit->setEnabled ( curSectionAction->b_canEdit );
	actionRemove->setEnabled ( curSectionAction->b_canRemove );
	actionSave->setEnabled ( curSectionAction->b_canSave );
	actionCancel->setEnabled ( curSectionAction->b_canCancel );
	actJobSelectJob->setEnabled ( curSectionAction->b_canExtra[0] );
	actPayReceipt->setEnabled ( curSectionAction->b_canExtra[0] );
	actPayUnPaidReport->setEnabled ( curSectionAction->b_canExtra[1] );
	actPayReport->setEnabled ( curSectionAction->b_canExtra[2] );
}

bool MainWindow::execRecordAction ( const int key )
{
	switch ( key )
	{
		case Qt::Key_S:
			updateWindowBeforeSave ();
			func_Save ( activeRecord ); 
		break;
		case Qt::Key_A:
			activeRecord = func_Add (); break;
		case Qt::Key_E:
			func_Edit ( activeRecord ); break;
		case Qt::Key_R:
			func_Del ( activeRecord ); break;
		case Qt::Key_Escape:
			func_Cancel ( activeRecord ); break;
		default:
			return false;
	}
	return true;
}

void MainWindow::setupTabNavigationButtons ()
{
	mBtnNavPrev = new QToolButton;
	mBtnNavPrev->setIcon ( ICON ( "go-previous" ) );
	mBtnNavPrev->setEnabled ( false );
	static_cast<void>( connect ( mBtnNavPrev, &QToolButton::clicked, this, [&] () { return navigatePrev (); } ) );
	mBtnNavNext = new QToolButton;
	mBtnNavNext->setIcon ( ICON ( "go-next" ) );
	mBtnNavNext->setEnabled ( false );
	static_cast<void>( connect ( mBtnNavNext, &QToolButton::clicked, this, [&] () { return navigateNext (); } ) );

	auto lButtons ( new QHBoxLayout );
	lButtons->setContentsMargins ( 1, 1, 1, 1 );
	lButtons->setSpacing ( 1 );
	lButtons->addWidget ( mBtnNavPrev );
	lButtons->addWidget ( mBtnNavNext );
	QFrame* frmContainer ( new QFrame );
	frmContainer->setLayout ( lButtons );
	ui->tabMain->tabBar ()->setTabButton ( 0, QTabBar::LeftSide, frmContainer );
	navItems.setPreAllocNumber ( 50 );
	editItems.setPreAllocNumber ( 10 );
}
//----------------------------------SETUP-CUSTOM-CONTROLS-NAVIGATION--------------------------------------

//--------------------------------------------WINDOWS-KEYS-EVENTS-EXTRAS-------------------------------------------------
void MainWindow::closeEvent ( QCloseEvent* e )
{
	e->ignore ();
	trayActivated ( QSystemTrayIcon::Trigger );
}

// return false: standard event processing
// return true: we process the event
bool MainWindow::eventFilter ( QObject* o, QEvent* e )
{
	bool b_accepted ( false );
	if ( ui->tabMain->currentIndex () == TI_MAIN )
	{
		if ( e->type () == QEvent::KeyPress )
		{
			const QKeyEvent* k ( static_cast<QKeyEvent*> ( e ) );
			if ( k->modifiers () & Qt::ControlModifier )
			{
				if ( (k->key () - Qt::Key_A) <= ( Qt::Key_Z - Qt::Key_A) )
				{
					if ( k->key () == Qt::Key_F )
					{
						b_accepted = true;
						btnSearch_clicked ();
					}
					else
						b_accepted = execRecordAction ( k->key () );
				}
			}
			else
			{ // no control key pressed
				b_accepted = (k->key () - Qt::Key_F4) <= (Qt::Key_F12 - Qt::Key_F4);
				switch ( k->key () )
				{
					case Qt::Key_Escape:
						b_accepted = execRecordAction ( k->key () );
					break;

					case Qt::Key_F4: btnReportGenerator_clicked (); break;
					case Qt::Key_F5: btnBackupRestore_clicked (); break;
					case Qt::Key_F6: btnCalculator_clicked (); break;
					case Qt::Key_F7: break;
					case Qt::Key_F8: btnEstimates_clicked (); break;
					case Qt::Key_F9: btnCompanyPurchases_clicked (); break;
					case Qt::Key_F10: btnConfiguration_clicked (); break;
					case Qt::Key_F12: btnExitProgram_clicked (); break;
				}
			}
		}
	}
	e->setAccepted ( b_accepted );
	return b_accepted ? true : QMainWindow::eventFilter ( o, e );
}

//---------------------------------------------SESSION----------------------------------------------------------
void MainWindow::showTab ( const TAB_INDEXES ti )
{
	showNormal ();
	ui->tabMain->setCurrentIndex ( static_cast<int> ( ti ) );
}

void MainWindow::tabMain_currentTabChanged ( const int tab_idx )
{
	if ( Sys_Init::EXITING_PROGRAM )
		return;

	switch ( tab_idx )
	{
		case TI_CALENDAR:
			mActionsToolBar->hide ();
			if ( mCalendarView )
				mCalendarView->activate ();
		break;
		case TI_STATISTICS:
			mActionsToolBar->hide ();
			if ( !mStatistics )
				mStatistics = new dbStatistics;
			ui->tabStatistics->setLayout ( mStatistics->layout () );
			mStatistics->reload ();
		break;
		case TI_TABLEWIEW:
			mActionsToolBar->hide ();
			if ( !mTableView )
			{
				mTableView = new dbTableView;
				ui->tabTableView->setLayout ( mTableView->layout () );
			}
			mTableView->reload ();
		break;
		default:
			mActionsToolBar->show ();
		break;
	}
}

void MainWindow::alterLastViewedRecord ( dbListItem* old_item, dbListItem* new_item )
{
	static const QString query_template ( QStringLiteral ( "UPDATE %1 SET `LAST_VIEWED`='%2' WHERE ID='%3'" ) );
	DBRecord* old_rec ( nullptr ), *new_rec ( nullptr );
	QSqlQuery queryRes;

	if ( old_item )
	{
		old_rec = old_item->dbRec ();
		VDB ()->runModifyingQuery ( query_template.arg (
				old_rec->tableInfo ()->table_name, CHR_ZERO, recStrValue ( old_rec, 0 ) ), queryRes );
		setRecValue ( old_rec, static_cast<uint>( VivenciaDB::getTableColumnIndex ( old_rec->tableInfo (), QStringLiteral ( "`LAST_VIEWED`" ) ) ), CHR_ZERO );
	}
	if ( new_item )
	{
		new_rec = new_item->dbRec ();
		VDB ()->runModifyingQuery ( query_template.arg (
				new_rec->tableInfo ()->table_name, CHR_ONE, recStrValue ( new_rec, 0 ) ), queryRes );
		setRecValue ( new_rec, static_cast<uint>( VivenciaDB::getTableColumnIndex ( new_rec->tableInfo (), QStringLiteral ( "`LAST_VIEWED`" ) ) ), CHR_ONE );
	}
}

uint MainWindow::getLastViewedRecord ( const TABLE_INFO* t_info, const uint client_id, const uint job_id )
{
	static const QString query_template ( QStringLiteral ( "SELECT ID FROM %1 WHERE `LAST_VIEWED`='1' %2" ) );
	QSqlQuery queryRes;
	if ( VDB ()->runSelectLikeQuery	( query_template.arg ( t_info->table_name,
		client_id > 0 ? ( QStringLiteral ( "AND `CLIENTID`='" ) + QString::number ( client_id ) + CHR_CHRMARK ) :
			job_id > 0 ? ( QStringLiteral ( "AND `JOBID`='" ) + QString::number ( job_id ) + CHR_CHRMARK ) :
				QString () ), queryRes ) )
	{
		bool b_ok ( true );
		const uint ret ( queryRes.value ( 0 ).toUInt ( &b_ok ) );
		if ( b_ok )
			return ret;
	}
	return 0;
}

void MainWindow::changeNavLabels ()
{
	ui->lblCurInfoClient->setText ( mClientCurItem ? mClientCurItem->text () : TR_FUNC ( "No client selected" ) );
	ui->lblCurInfoJob->setText ( mJobCurItem ? mJobCurItem->text () : TR_FUNC ( "No job selected" ) );
	ui->lblCurInfoPay->setText ( mPayCurItem ? mPayCurItem->text () : TR_FUNC ( "No payment selected" ) );
	ui->lblCurInfoBuy->setText ( mBuyCurItem ? mBuyCurItem->text () : TR_FUNC ( "No purchase selected" ) );
	ui->lblBuyAllPurchases_client->setText ( mClientCurItem ? mClientCurItem->text () : TR_FUNC ( "NONE" ) );
}

void MainWindow::navigatePrev ()
{
	static_cast<void>( navItems.prev () );
	mBtnNavNext->setEnabled ( true );
	displayNav ();
	mBtnNavPrev->setEnabled ( navItems.peekPrev () != nullptr );
}

void MainWindow::navigateNext ()
{
	static_cast<void>( navItems.next () );
	mBtnNavPrev->setEnabled ( true );
	displayNav ();
	mBtnNavNext->setEnabled ( navItems.peekNext () != nullptr );
}

void MainWindow::insertNavItem ( dbListItem* item )
{
	if ( item != nullptr )
	{
		navItems.insert ( static_cast<uint>( navItems.currentIndex () + 1 ), item );
		mBtnNavPrev->setEnabled ( navItems.currentIndex () > 0 );
	}
}

void MainWindow::displayNav ()
{
	auto parentClient ( static_cast<clientListItem*>( navItems.current ()->relatedItem ( RLI_CLIENTPARENT ) ) );
	auto parentJob ( static_cast<jobListItem*>( navItems.current ()->relatedItem ( RLI_JOBPARENT ) ) );
	const bool bSelectClient ( parentClient != mClientCurItem );
	
	switch ( navItems.current ()->subType () )
	{
		case CLIENT_TABLE:
			displayClient ( parentClient, true );
		break;
		case JOB_TABLE:
			if ( bSelectClient )
				displayClient ( parentClient, true, static_cast<jobListItem*>( navItems.current () ) );
			else
				displayJob ( static_cast<jobListItem*>( navItems.current () ), true );
		break;
		case PAYMENT_TABLE:
			if ( bSelectClient )
				displayClient ( parentClient, true, parentJob );
			else
			{
				if ( parentJob != mJobCurItem )
					displayJob ( parentJob, true );
				else
					displayPay ( static_cast<payListItem*>( navItems.current () ), true ); 
			}
		break;
 		case PURCHASE_TABLE:
			if ( bSelectClient )
				displayClient ( parentClient, true, parentJob, static_cast<buyListItem*>( navItems.current () ) );
			else
			{
				if ( parentJob != mJobCurItem )
					displayJob ( parentJob, true );
				else
					displayPay ( static_cast<payListItem*>( navItems.current () ), true ); 
			}
		break;
		default: break;
	}
}

void MainWindow::insertEditItem ( dbListItem* item )
{
	if ( editItems.contains ( item ) == -1 )
		editItems.append ( item );
}

void MainWindow::removeEditItem ( dbListItem* item )
{
	m_crash->eliminateRestoreInfo ( item->crashID () );
	editItems.removeOne ( item );
}

void MainWindow::saveEditItems ()
{
	if ( !Sys_Init::EXITING_PROGRAM || editItems.isEmpty () )
		return;
	
	dlgSaveEditItems = new QDialog ( this, Qt::Tool );
	dlgSaveEditItems->setWindowTitle ( TR_FUNC ( "Save these alterations?" ) );
	
	auto editItemsTable ( new vmTableWidget ( dlgSaveEditItems ) );
	editItemsTable->setIsPlainTable ();
	editItemsTable->setPlainTableEditable ( true );
	editItemsTable->setColumnsAutoResize ( true );
	vmTableColumn* cols ( editItemsTable->createColumns ( 3 ) );
	cols[0].label = TR_FUNC ( "Pending operation" );
	cols[0].editable = false;
	cols[1].label = TR_FUNC ( "Status" );
	cols[1].editable = false;
	cols[2].label = TR_FUNC ( "Save?" );
	cols[2].wtype = WT_CHECKBOX;
	cols[2].editable = true;
	
	editItemsTable->initTable ( editItems.count () );
	auto s_row ( new spreadRow );
	s_row->column[0] = 0;
	s_row->column[1] = 1;
	s_row->column[2] = 2;
	dbListItem* edititem ( nullptr );
	
	for ( uint i ( 0 ); i < editItems.count (); ++i )
	{
		edititem = editItems.at ( i );
		if ( edititem == nullptr )
			continue;
		s_row->row = static_cast<int> ( i );
		s_row->field_value[0] = edititem->text ();
		s_row->field_value[1] = edititem->action () == ACTION_EDIT ? TR_FUNC ( "Editing" ) : TR_FUNC ( "New entry" );
		s_row->field_value[2] = CHR_ONE;
		editItemsTable->setRowData ( s_row );
	}
	delete s_row;
	
	auto btnOK ( new QPushButton ( TR_FUNC ( "Continue" ) ) );
	static_cast<void>( connect ( btnOK, &QPushButton::clicked, this, [&] () { return dlgSaveEditItems->done ( 0 ); } ) );
	auto vLayout ( new QVBoxLayout );
	vLayout->setContentsMargins ( 2, 2, 2, 2 );
	vLayout->setSpacing ( 2 );
	vLayout->addWidget ( editItemsTable, 2 );
	vLayout->addWidget ( btnOK, 0, Qt::AlignCenter );
	dlgSaveEditItems->setLayout ( vLayout );
	dlgSaveEditItems->exec ();
	
	for ( int i ( 0 ); i < editItemsTable->lastRow (); ++i )
	{
		if ( editItemsTable->sheetItem ( static_cast<uint>(i), 2 )->text () == CHR_ONE )
		{
			edititem = editItems.at ( i );
			if ( edititem )
			{
				switch ( edititem->subType () )
				{
					case CLIENT_TABLE:
						static_cast<void>( saveClient ( static_cast<clientListItem*>( edititem ) ) ) ;
					break;
					case JOB_TABLE:
						static_cast<void>( saveJob ( static_cast<jobListItem*>( edititem ) ) );
					break;
					case PAYMENT_TABLE:
						static_cast<void>( savePay ( static_cast<payListItem*>( edititem ) ) );
					break;
					case PURCHASE_TABLE:
						static_cast<void>( saveBuy ( static_cast<buyListItem*>( edititem ) ) );
					break;
					default:
					break;
				}
			}
		}
	}
	delete dlgSaveEditItems;
}
//---------------------------------------------SESSION----------------------------------------------------------
void MainWindow::receiveWidgetBack ( QWidget* widget )
{
	// if the UI changes, the widget positions might change too, se we always need to check back against ui_mainwindow.h to see if those numbers match
	// Still easier than having a function to query the layouts for the rows, columns and spans for the widgets: the UI does not change that often
	if ( widget == ui->frmJobReport )
	{
		ui->splitterJobDaysInfo->addWidget ( ui->frmJobReport );
		ui->btnJobSeparateReportWindow->setChecked ( false );
	}
	else if ( widget == ui->frmJobPicturesControls )
	{
		ui->gLayoutJobExtraInfo->addWidget ( ui->frmJobPicturesControls );
		ui->btnJobSeparatePicture->setChecked ( false );
	}
	widget->show ();
}

inline void MainWindow::btnReportGenerator_clicked ()
{
	if ( !EDITOR ()->isVisible () )
		EDITOR ()->show ();

	EDITOR ()->activateWindow ();
	EDITOR ()->raise ();
}

inline void MainWindow::btnBackupRestore_clicked () { BACKUP ()->isVisible () ? 
		BACKUP ()->hide () : BACKUP ()->showWindow (); }

inline void MainWindow::btnSearch_clicked () { SEARCH_UI ()->isVisible () ?
		SEARCH_UI ()->hide () : SEARCH_UI ()->show (); }

inline void MainWindow::btnCalculator_clicked () { CALCULATOR ()->isVisible () ?
		CALCULATOR ()->hide () : CALCULATOR ()->showCalc ( emptyString, mapToGlobal ( ui->btnCalculator->pos () ) ); }

inline void MainWindow::btnEstimates_clicked () { ESTIMATES ()->isVisible () ?
		ESTIMATES ()->hide () : ESTIMATES ()->showWindow ( recStrValue ( mClientCurItem->clientRecord (), FLD_CLIENT_NAME ) ); }

inline void MainWindow::btnCompanyPurchases_clicked () { COMPANY_PURCHASES ()->isVisible () ?
		COMPANY_PURCHASES ()->hide () : COMPANY_PURCHASES ()->show (); }

inline void MainWindow::btnConfiguration_clicked () { CONFIG_DIALOG ()->isVisible () ?
		CONFIG_DIALOG ()->hide () : CONFIG_DIALOG ()->show (); }

inline void MainWindow::btnExitProgram_clicked () { Sys_Init::deInit (); }
//--------------------------------------------SLOTS------------------------------------------------------------

//--------------------------------------------TRAY-----------------------------------------------------------
void MainWindow::createTrayIcon ()
{
	trayIcon = new QSystemTrayIcon ();
	trayIcon->setIcon ( ICON ( APP_ICON ) );
	trayIcon->setToolTip ( windowTitle () );
	static_cast<void>( connect ( trayIcon, &QSystemTrayIcon::activated, this,
			  [&] ( QSystemTrayIcon::ActivationReason actReason ) { return trayActivated ( actReason ); } ) );
	trayIcon->show ();
	
	trayIconMenu = new QMenu ( this );
	trayIconMenu->addAction ( new vmAction ( 101, TR_FUNC ( "Hide" ), this ) );
	trayIconMenu->addAction ( new vmAction ( 100, TR_FUNC ( "Exit" ), this ) );
	static_cast<void>( connect ( trayIconMenu, &QMenu::triggered, this, [&] ( QAction* act ) { return trayMenuTriggered ( act ); } ) );
	trayIcon->setContextMenu ( trayIconMenu );
}

void MainWindow::createFloatToolBar ()
{
	mActionsToolBar = new QToolBar ( this );
	
	actionAdd = mActionsToolBar->addAction ( ICON ( "browse-controls/add.png" ), TR_FUNC ( "Add (CRTL+A)" ),
							 this, [&] () { return execRecordAction ( Qt::Key_A ); } );
	actionEdit = mActionsToolBar->addAction ( ICON ( "browse-controls/edit.png" ), TR_FUNC ( "Edit (CRTL+E)" ),
							 this, [&] () { return execRecordAction ( Qt::Key_E ); } );
	actionRemove = mActionsToolBar->addAction ( ICON ( "browse-controls/remove.png" ), TR_FUNC ( "Remove (CRTL+R)" ),
							 this, [&] () { return execRecordAction ( Qt::Key_R ); } );
	actionSave = mActionsToolBar->addAction ( ICON ( "document-save" ), TR_FUNC ( "Save (CRTL+S)" ), this,
							 [&] () { return execRecordAction ( Qt::Key_S ); } );
	actionCancel = mActionsToolBar->addAction ( ICON ( "cancel.png" ), TR_FUNC ( "Cancel (Esc key)" ),
							 this, [&] () { return execRecordAction ( Qt::Key_Escape ); } );
	
	mActionsToolBar->setFloatable ( true );
	mActionsToolBar->setMovable ( true );
	mActionsToolBar->setOrientation ( Qt::Vertical );
	mActionsToolBar->setToolButtonStyle ( Qt::ToolButtonIconOnly );
	mActionsToolBar->setIconSize ( QSize ( 30, 30 ) );
	mActionsToolBar->setGeometry ( 20, 50, mActionsToolBar->sizeHint ().width (), mActionsToolBar->sizeHint ().height () );
	addToolBar ( Qt::LeftToolBarArea, mActionsToolBar );
	
	auto utilitiesToolBar ( new QToolBar ( this ) );
	utilitiesToolBar->addWidget ( ui->btnReportGenerator );
	utilitiesToolBar->addWidget ( ui->btnBackupRestore );
	utilitiesToolBar->addWidget ( ui->btnSearch );
	utilitiesToolBar->addWidget ( ui->btnCalculator );
	utilitiesToolBar->addWidget ( ui->btnServicesPrices );
	utilitiesToolBar->addWidget ( ui->btnEstimates );
	utilitiesToolBar->addWidget ( ui->btnCompanyPurchases );
	utilitiesToolBar->addWidget ( ui->btnConfiguration );
	utilitiesToolBar->addWidget ( ui->btnExitProgram );
	addToolBar ( Qt::TopToolBarArea, utilitiesToolBar );
	
	auto sectionsToolBar ( new QToolBar ( this ) );
	sectionsToolBar->addWidget ( ui->lblCurInfoClient );
	sectionsToolBar->addWidget ( ui->line_2 );
	sectionsToolBar->addWidget ( ui->lblCurInfoJob );
	sectionsToolBar->addWidget ( ui->line_3 );
	sectionsToolBar->addWidget ( ui->lblCurInfoPay );
	sectionsToolBar->addWidget ( ui->line_4 );
	sectionsToolBar->addWidget ( ui->lblCurInfoBuy );
	addToolBar ( Qt::TopToolBarArea, sectionsToolBar );
	addToolBarBreak ( Qt::TopToolBarArea );
}

void MainWindow::trayMenuTriggered ( QAction* action )
{
	const int idx ( dynamic_cast<vmAction*>( action )->id () );

	if ( idx == 100 )
	{
		btnExitProgram_clicked ();
		return;
	}
	if ( idx == 101 )
		trayActivated ( QSystemTrayIcon::Trigger );
}

void MainWindow::trayActivated ( QSystemTrayIcon::ActivationReason reason )
{
	switch ( reason )
	{
		case QSystemTrayIcon::Trigger:
		case QSystemTrayIcon::DoubleClick:
			if ( isMinimized () || isHidden () )
			{
				trayIconMenu->actions ().at ( 0 )->setText ( TR_FUNC ( "Hide" ) );
				showNormal ();
				activateWindow ();
				raise ();
				if ( ui->tabMain->currentIndex () == TI_MAIN )
					mActionsToolBar->show ();
				if ( Sys_Init::isEditorStarted () )
				{
					if ( EDITOR ()->isHidden () )
						EDITOR ()->showNormal ();
				}
			}
			else
			{
				trayIconMenu->actions ().at ( 0 )->setText ( TR_FUNC ( "Restore" ) );
				hide ();
				mActionsToolBar->hide ();
				if ( Sys_Init::isEditorStarted () )
				{
					if ( EDITOR ()->isVisible () )
						EDITOR ()->hide ();
				}
			}
		break;
		case QSystemTrayIcon::MiddleClick:
		case QSystemTrayIcon::Unknown:
		case QSystemTrayIcon::Context:
		break;
	}
}

//-------------------------------------------ITEM-SHARED---------------------------------------------------------
void MainWindow::notifyExternalChange ( const uint id, const uint table_id, const RECORD_ACTION action )
{
	switch ( table_id )
	{
		case CLIENT_TABLE:		clientExternalChange ( id, action );	break;
		case JOB_TABLE:			jobExternalChange ( id, action );		break;
		case PAYMENT_TABLE:		payExternalChange ( id, action );		break;
		case PURCHASE_TABLE:	buyExternalChange ( id, action );		break;
		case SUPPLIES_TABLE:	dbSupplies::notifyDBChange ( id );		break;
	}
}

void MainWindow::removeListItem ( dbListItem* item, const bool b_delete_item, const bool b_auto_select_another )
{
	vmListWidget* list ( item->listWidget () );
	list->setIgnoreChanges ( true );
	removeEditItem ( item ); // maybe item is in EDIT_ or ADD_ ACTION
	
	// do not propagate the status to DBRecord so that it might use the info from the previous 
	// state to perform some actions if needed
	item->setAction ( ACTION_DEL, true );
	if ( item->dbRec () && item->relation () == RLI_CLIENTITEM )
		item->dbRec ()->deleteRecord ();
	if ( b_auto_select_another )
		list->setIgnoreChanges ( false );
	list->removeItem ( item, b_delete_item );
}

void MainWindow::postFieldAlterationActions ( dbListItem* item )
{
	//item->saveCrashInfo ( m_crash ); TODO TOOOOOOOOOO SLOOOOOOOOOOWWWWWWW
	const bool bCanSave ( true );//item->isGoodToSave () );
	curSectionAction->b_canSave = bCanSave;
	updateActionButtonsState ();
	item->update ();
}

// Trigger a focus change to force an update in case there are any pending changes to be processed
void MainWindow::updateWindowBeforeSave ()
{
	QWidget* wdgFocus ( this->focusWidget () );
	ui->tabStatistics->setFocus ();
	qApp->sendPostedEvents();
	wdgFocus->setFocus ();
	qApp->sendPostedEvents();
}
//-------------------------------------------ITEM-SHARED---------------------------------------------------------

//--------------------------------------------JOB---------------------------------------------------------------
void MainWindow::addJobPictures ()
{
	if ( mJobCurItem )
	{
		QString strPicturePath ( recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_PICTURE_PATH ) );
		if ( strPicturePath.isEmpty () )
		{
			strPicturePath = recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_PROJECT_ID );
			if ( !strPicturePath.isEmpty () )
				strPicturePath.append ( QStringLiteral ( " - " ) + recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_TYPE ) );
			else
			{
				strPicturePath = mJobCurItem->jobRecord ()->date ( FLD_JOB_STARTDATE ).toDate ( vmNumber::VDF_FILE_DATE ) +
						QStringLiteral ( " - " ) + recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_TYPE ) + CHR_F_SLASH;
			}
			
			const QString simpleJobBaseDir ( CONFIG ()->getProjectBasePath ( recStrValue ( mClientCurItem->clientRecord (), FLD_CLIENT_NAME ) )
											 + QStringLiteral ( u"Serviços simples/" ) );
			if ( fileOps::exists ( simpleJobBaseDir ).isOff () )
				fileOps::createDir ( simpleJobBaseDir );
			strPicturePath.prepend ( simpleJobBaseDir );
			fileOps::createDir ( strPicturePath );
			if ( static_cast<TRI_STATE>( mJobCurItem->action () >= ACTION_READ ) )
			{
				mJobCurItem->setAction ( ACTION_EDIT, true );
				ui->txtJobPicturesPath->setText ( strPicturePath, true );
				mJobCurItem->jobRecord ()->saveRecord ();
				mJobCurItem->setAction ( ACTION_READ );
			}
			else
				ui->txtJobPicturesPath->setText ( strPicturePath );
		}
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Waiting for " ) + CONFIG ()->fileManager () + TR_FUNC ( " to finish" ),
									  TR_FUNC ( "Add some pictures to this job info data" ) );

		QStringList args ( strPicturePath );
		fileOps::executeWait ( args, CONFIG ()->fileManager () );
		// useless, because I cannot block the main thread wating for the filemanager to return
		// One possible solution would be to connect the QProcess inside fileOps::executeWait to the finished signal
		// but fileOps is not an objectfiable class, therefore it cannot inherit QObject
		//btnJobReloadPictures_clicked ();
	}
}

void MainWindow::btnJobReloadPictures_clicked ()
{
	jobImageViewer->reload ( ui->txtJobPicturesPath->text () );
	ui->cboJobPictures->setIgnoreChanges ( true );
	ui->cboJobPictures->clear ();
	jobImageViewer->addImagesList ( ui->cboJobPictures );
	controlJobPictureControls ();
	ui->cboJobPictures->setIgnoreChanges ( false );
}

void MainWindow::showClientsYearPictures ( QAction* action )
{
	if ( action == jobsPicturesMenuAction )
		return;

	QString picturePath, str_lbljobpicture;
	const QString lastDir ( QString::number ( dynamic_cast<vmAction*>( action )->id () ) );
	if ( lastDir == CHR_ZERO )
	{
		picturePath = recStrValue ( mJobCurItem->jobRecord (), FLD_JOB_PICTURE_PATH );
		str_lbljobpicture = TR_FUNC ( "Job pictures path:" );
	}
	else
	{
		picturePath = CONFIG ()->getProjectBasePath ( ui->txtClientName->text () );
		picturePath += jobPicturesSubDir + lastDir + CHR_F_SLASH;
		fileOps::createDir ( picturePath );
		str_lbljobpicture = QString ( TR_FUNC ( "Viewing clients pictures for year %1" ) ).arg ( lastDir );
	}
	ui->btnJobNextPicture->setEnabled ( true );
	ui->btnJobPrevPicture->setEnabled ( true );
	jobImageViewer->prepareToShowImages ( -1, picturePath );
	ui->cboJobPictures->setIgnoreChanges ( true );
	ui->cboJobPictures->clear ();
	ui->cboJobPictures->clearEditText ();
	ui->cboJobPictures->setEditText ( jobImageViewer->imageFileName () );
	jobImageViewer->addImagesList ( ui->cboJobPictures );
	ui->cboJobPictures->setIgnoreChanges ( false );
	ui->lblJobPictures->setText ( str_lbljobpicture );
	ui->txtJobPicturesPath->setText ( picturePath );
	jobsPicturesMenuAction->setChecked ( false );
	jobsPicturesMenuAction = action;
	action->setChecked ( true );
}

void MainWindow::showDayPictures ( const vmNumber& date )
{
	const int npics ( ui->cboJobPictures->count () );
	if ( npics > 0 )
	{
		vmNumber::VM_DATE_FORMAT format ( vmNumber::VDF_DROPBOX_DATE );
		QString strDate;
		do
		{
			strDate = date.toDate ( format );
			for ( int i ( 0 ); i < npics; ++i )
			{
				if ( ui->cboJobPictures->itemText ( i ).startsWith ( strDate, Qt::CaseInsensitive ) )
				{
					ui->cboJobPictures->setCurrentIndex ( i );
					return;
				}
			}
			// Dropbox filename format did not yield any result. Try file format
			format = static_cast<vmNumber::VM_DATE_FORMAT>( static_cast<int>( format ) - 1 );
		} while ( format >= vmNumber::VDF_FILE_DATE );
		
		// Date by filename failed. Now we try another method: look into the file properties and query its modification date.
		// Note: dropbox file format is preferred and is the default method since late 2013, save a few periods of exception
		// This last method will probably fail.
		QString filename;
		filename.reserve ( ui->txtJobPicturesPath->text ().count () + 20 );
		vmNumber modDate, modTime;
		for ( int i ( 0 ); i < npics; ++i )
		{
			filename = ui->txtJobPicturesPath->text () + ui->cboJobPictures->itemText ( i );
			if ( fileOps::modifiedDateTime ( filename, modDate, modTime ) )
			{
				if ( date == modDate )
				{
					ui->cboJobPictures->setCurrentIndex ( i );
					return;
				}
			}
		}
		ui->cboJobPictures->setCurrentIndex ( ui->cboJobPictures->count () - 1 ); //when everything else fails
	}
}

void MainWindow::showJobImageRequested ( const int index )
{
	ui->cboJobPictures->setCurrentText ( ui->cboJobPictures->itemText ( index ) );
	ui->btnJobPrevPicture->setEnabled ( index > 0 );
	ui->btnJobNextPicture->setEnabled ( index < ( ui->cboJobPictures->count () - 1 ) );
	if ( sepWin_JobPictures && index != -1 )
	{
		if ( !sepWin_JobPictures->isHidden () )
			sepWin_JobPictures->setWindowTitle ( jobImageViewer->imageFileName () );
	}
}

void MainWindow::btnJobRenamePicture_clicked ( const bool checked )
{
	ui->cboJobPictures->vmComboBox::setEditable ( checked );
	if ( checked )
	{
		if ( !ui->cboJobPictures->text ().isEmpty () )
		{
			ui->cboJobPictures->vmComboBox::setEditable ( true );
			ui->cboJobPictures->editor ()->selectAll ();
			ui->cboJobPictures->setFocus ();
		}
	}
	else
	{
		const int new_index ( jobImageViewer->rename ( ui->cboJobPictures->text () ) );
		if ( new_index != -1 )
		{
			ui->cboJobPictures->removeItem ( ui->cboJobPictures->currentIndex () );
			ui->cboJobPictures->QComboBox::insertItem ( new_index, ui->cboJobPictures->text () );
			ui->cboJobPictures->setCurrentIndex ( new_index );
			ui->btnJobNextPicture->setEnabled ( new_index < ( ui->cboJobPictures->count () - 1 ) );
			ui->btnJobPrevPicture->setEnabled ( new_index >= 1 );
		}
	}
}

void MainWindow::showJobImageInWindow ( const bool maximized )
{
	if ( sepWin_JobPictures == nullptr )
	{
		sepWin_JobPictures = new separateWindow ( ui->frmJobPicturesControls );
		sepWin_JobPictures->setCallbackForReturningToParent ( [&] ( QWidget* widget ) { return receiveWidgetBack ( widget ); } );
	}
	sepWin_JobPictures->showSeparate ( jobImageViewer->imageFileName (), false, maximized ? Qt::WindowMaximized : Qt::WindowNoState );
	if ( !ui->btnJobSeparatePicture->isChecked () )
		ui->btnJobSeparatePicture->setChecked ( true );
}

void MainWindow::btnJobSeparateReportWindow_clicked ( const bool checked )
{
	if ( sepWin_JobReport == nullptr )
	{
		sepWin_JobReport = new separateWindow ( ui->frmJobReport );
		sepWin_JobReport->setCallbackForReturningToParent ( [&] ( QWidget* widget ) { return receiveWidgetBack ( widget ); } );
	}
	if ( checked )
	{
		QString title ( TR_FUNC ( "Job report " ) );
		if ( !ui->txtJobProjectID->text ().isEmpty () )
			title += ui->txtJobProjectID->text ();
		else
			title += TR_FUNC ( "for the client " ) + ui->txtClientName->text ();
		sepWin_JobReport->showSeparate ( title );
	}
	else
		sepWin_JobReport->returnToParent ();
}
//--------------------------------------------JOB------------------------------------------------------------
