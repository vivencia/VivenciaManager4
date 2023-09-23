#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <dbRecords/dblistitem.h>
#include <vmTaskPanel/vmtaskpanel.h>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSystemTrayIcon>

#include <functional>

class crashRestore;
class clientListItem;
class jobListItem;
class schedListItem;
class payListItem;
class buyListItem;
class separateWindow;
class machinesDlg;
class vmAction;
class vmNotify;
class vmListWidget;
class vmLineEdit;
class vmTableWidget;
class vmActionGroup;
class dbCalendar;
class dbListItem;
class dbTableView;
class dbStatistics;
class calendarViewUI;
class imageViewer;

class QEvent;
class QKeyEvent;
class QMenu;
class QSystemTrayIcon;
class QToolButton;
class QTabWidget;
class QDialog;

namespace Ui
{

static const QString mw_configSectionName ( QStringLiteral ( "MAIN_WINDOW" ) );
static const QString mw_configCategoryWindowGeometry ( QStringLiteral ( "WINDOW_GEOMETRY" ) );
static const QString mw_configCategoryAppScheme ( QStringLiteral ( "APP_SCHEME" ) );

class MainWindow;
}

class MainWindow : public QMainWindow
{

friend class searchUI;

public:

	enum TAB_INDEXES { TI_MAIN = 0, TI_CALENDAR = 1, TI_STATISTICS = 2, TI_TABLEWIEW = 3 };

	explicit MainWindow ();
	virtual ~MainWindow () override;

	void continueStartUp ();
	void exitRequested ();

//--------------------------------------------CLIENT------------------------------------------------------------
	void saveClientWidget ( vmWidget* widget, const int id );
	void showClientSearchResult ( dbListItem* item, const bool bshow );
	void setupClientPanel ();
	void clientsListWidget_currentItemChanged ( dbListItem* item );
	void controlClientForms ( const clientListItem* const client_item );
	bool saveClient ( clientListItem* client_item, const bool b_dbsave = true );
	clientListItem* addClient ();
	bool editClient ( clientListItem* client_item, const bool b_dogui = true );
	bool delClient ( clientListItem* client_item, const bool b_ask = true );
	bool cancelClient ( clientListItem* client_item );
	bool displayClient ( clientListItem* client_item, const bool b_select = false, jobListItem* job_item = nullptr, buyListItem* buy_item = nullptr, const uint jobid = 0, const uint buyid = 0 );
	void loadClientInfo ( const Client* const client );
	clientListItem* getClientItem ( const uint id ) const;
	void loadClients ( clientListItem* &item );
	dbListItem* loadItem( const clientListItem* client_parent, const uint id, const uint table_id );
	void fillAllLists (const clientListItem* client_item, jobListItem* &job_item, buyListItem* &buy_item, const uint jobid = 0, const uint buyid = 0 );
	void alterClientEndDate ( const jobListItem* const job_item );
	void clientExternalChange ( const uint id, const RECORD_ACTION action );
//--------------------------------------------CLIENT------------------------------------------------------------

//--------------------------------------------EDITING-FINISHED-CLIENT-----------------------------------------------------------
	void txtClientName_textAltered ( const QString& text );
	void txtClient_textAltered ( const vmWidget* const sender );
	void txtClientZipCode_textAltered ( const QString& text );
	void dteClient_dateAltered ( const vmWidget* const sender );
	void contactsClientAdd ( const QString& info, const vmWidget* const sender );
	void contactsClientDel ( const int idx, const vmWidget* const sender );
//--------------------------------------------EDITING-FINISHED-CLIENT-----------------------------------------------------------

//--------------------------------------------JOB------------------------------------------------------------
	void saveJobWidget ( vmWidget* widget, const int id );
	void showJobSearchResult ( dbListItem* item, const bool bshow );
	void setupJobPanel ();
	void setUpJobButtons ( const QString& path );
	void setupJobPictureControl ();
	void displayJobFromCalendar ( dbListItem* cal_item );
	void jobsListWidget_currentItemChanged ( dbListItem* item );
	void lstJobDayReport_currentItemChanged ( vmListItem* item );
	void controlJobForms ( const jobListItem* const job_item );
	void controlJobDaySection ( const jobListItem* const job_item );
	void controlJobDayForms ( const bool b_editable );
	void controlJobPictureControls (); //must be called from within loadJobInfo or after because it relies on having read the database for info
	bool saveJob ( jobListItem* job_item, const bool b_dbsave = true );
	void postSaveJobActions ( jobListItem* job_item );
	jobListItem* addJob ( clientListItem* parent_client );
	bool editJob ( jobListItem* job_item, const bool b_dogui = true );
	bool delJob ( jobListItem* job_item, const bool b_ask = true );
	bool cancelJob ( jobListItem* job_item );
	void displayJob ( jobListItem* job_item, const bool b_select = false, buyListItem* buy_item = nullptr );
	void loadJobInfo ( const Job* const job );
	jobListItem* getJobItem ( const clientListItem* const parent_client, const uint id ) const;
	void scanJobImages ();
	void decodeJobReportInfo ( const Job* const job );
	void fillJobKeyWordsList ( const Job* const job );
	void fixJobDaysList ( jobListItem* const job_item );
	void revertDayItem ( vmListItem* day_item );
	void updateJobInfo ( const QString& text, const uint user_role, vmListItem* const item = nullptr );
	void addJobPayment ( jobListItem* const job_item );
	void saveJobPayment ( jobListItem* const job_item );
	void removeJobPayment ( payListItem* pay_item );
	void alterJobPayPrice ( jobListItem* const job_item );
	bool jobRemoveKeyWord ( const uint row );
	void jobAddKeyWord ( const QString& words );
	void calcJobTime ();
	void showJobTotalTime ( const vmNumber& time ) const;
	triStateType dateIsInDaysList ( const QString& str_date );
	void rescanJobDaysList ( jobListItem* const job_item );
	void controlFormsForJobSelection ( const bool bStartSelection );
	void selectJob ();
	void btnJobSelect_clicked ();
	void btnJobAddDay_clicked ();
	void btnJobEditDay_clicked ();
	void btnJobDelDay_clicked ();
	void btnJobCancelDelDay_clicked ();
	void btnJobPrevDay_clicked ();
	void btnJobNextDay_clicked ();
	void btnJobMachines_clicked ();
	void jobOpenProjectFile ( QAction* action );
	void quickProjectClosed ();
	void on_btnQuickProject_clicked ();
	void jobExternalChange ( const uint id, const RECORD_ACTION action );
	inline void setTempCallbackForJobSelect ( const std::function<void ( const uint )>& func ) { selJob_callback = func; }
//--------------------------------------------JOB------------------------------------------------------------

//--------------------------------------------EDITING-FINISHED-JOB-----------------------------------------------------------
	void cboJobType_textAltered ( const vmWidget* const sender );
	void txtJobProjectPath_textAltered ( const vmWidget* const sender );
	void txtJob_textAltered ( const vmWidget* const sender );
	void txtJobWheather_textAltered ( const vmWidget* const sender );
	void txtJobTotalDayTime_textAltered ( const vmWidget* const sender );
	void timeJobStart_timeAltered ();
	void timeJobEnd_timeAltered ();
	void dteJob_dateAltered ( const vmWidget* const sender );
	void dteJobAddDate_dateAltered ();
//--------------------------------------------EDITING-FINISHED-JOB-----------------------------------------------------------

//--------------------------------------------PAY------------------------------------------------------------
	void savePayWidget ( vmWidget* widget, const int id );
	void showPaySearchResult ( dbListItem* item, const bool bshow );
	void setupPayPanel ();
	void displayPayFromCalendar ( dbListItem* cal_item );
	void paysListWidget_currentItemChanged ( dbListItem* item );
	void paysOverdueListWidget_currentItemChanged ( dbListItem* item );
	void paysOverdueClientListWidget_currentItemChanged ( dbListItem* item );
	void addPaymentOverdueItems ( payListItem* pay_item );
	void removePaymentOverdueItems ( payListItem* pay_item );
	void updateTotalPayValue (const vmNumber& nAdd, const vmNumber& nSub );
	void updatePayOverdueTotals ( const vmNumber& nAdd, const vmNumber& nSub );
	void payOverdueGUIActions ( Payment* const pay, const RECORD_ACTION new_action );
	void controlPayForms ( const payListItem* const pay_item );
	bool savePay ( payListItem* pay_item );
	bool editPay ( payListItem* pay_item, const bool b_dogui = true );
	bool delPay ( payListItem* pay_item );
	bool cancelPay ( payListItem* pay_item );
	void displayPay ( payListItem* pay_item, const bool b_select = false );
	void loadPayInfo ( const Payment* const pay );
	payListItem* getPayItem ( const clientListItem* const parent_client, const uint id ) const;
	void loadClientOverduesList ();
	void loadAllOverduesList ();
	void interceptPaymentCellChange ( const vmTableItem* const item );
	void updatePayTotalPaidValue ();
	void tabPaysLists_currentChanged ( const int index );
	void payExternalChange ( const uint id, const RECORD_ACTION action );
//--------------------------------------------PAY------------------------------------------------------------

//------------------------------------EDITING-FINISHED-PAY-----------------------------------------------------------
	void txtPayTotalPrice_textAltered ( const QString& text );
	void txtPay_textAltered ( const vmWidget* const sender );
	void chkPayOverdue_toggled ( const bool checked );
//------------------------------------EDITING-FINISHED-PAY-----------------------------------------------------------

//--------------------------------------------BUY------------------------------------------------------------
	void saveBuyWidget ( vmWidget* widget, const int id );
	void showBuySearchResult ( dbListItem* item, const bool bshow );
	void setupBuyPanel ();
	void displayBuyFromCalendar ( dbListItem* cal_item );
	void buysListWidget_currentItemChanged ( dbListItem* item );
	void buysJobListWidget_currentItemChanged ( dbListItem* item );
	void buySuppliersListWidget_currentItemChanged ( dbListItem* item );
	void updateBuyTotalPriceWidgets ( const buyListItem* const buy_item );
	void controlBuyForms ( const buyListItem* const buy_item );
	bool saveBuy ( buyListItem* buy_item, const bool b_dbsave = true );
	buyListItem* addBuy ( clientListItem* client_parent, jobListItem* job_parent );
	bool editBuy ( buyListItem* buy_item, const bool b_dogui = true );
	bool delBuy ( buyListItem* buy_item, const bool b_ask = true );
	bool cancelBuy ( buyListItem* buy_item );
	void displayBuy ( buyListItem* buy_item, const bool b_select = false );
	void loadBuyInfo ( const Buy* const buy );
	buyListItem* getBuyItem ( const clientListItem* const parent_client, const uint id ) const;
	void fillJobBuyList ( const jobListItem* parent_job );
	void interceptBuyItemsCellChange ( const vmTableItem* const item );
	void interceptBuyPaymentCellChange ( const vmTableItem* const item );
	void updateBuyTotalPaidValue ();
	void updateBuySelectionTotals ();
	void updateBuyFilteredViewTotals ();
	void getPurchasesForSuppliers ( const QString& supplier );
	void setBuyPayValueToBuyPrice ( const QString& price );
	void buyExternalChange ( const uint id, const RECORD_ACTION action );
//--------------------------------------------BUY------------------------------------------------------------

//--------------------------------------------EDITING-FINISHED-BUY-----------------------------------------------------------
	void txtBuy_textAltered ( const vmWidget* const sender );
	void dteBuy_dateAltered ( const vmWidget* const sender );
	void cboBuySuppliers_textAltered ( const QString& text );
	void cboBuySuppliers_indexChanged ( const int idx );
//--------------------------------------------EDITING-FINISHED-BUY-----------------------------------------------------------

//----------------------------------SETUP-CUSTOM-CONTROLS-NAVIGATION--------------------------------------
	void setupCustomControls ();
	void restoreCrashedItems ( crashRestore* const crash, clientListItem* &client_item, jobListItem* &job_item, buyListItem* &buy_item );
	void restoreLastSession ();
	void searchCallbackSelector ( const QKeyEvent* ke );
	void reOrderTabSequence ();
	void setupWorkFlow ();
	void setupSectionNavigation ();
	void findSectionByScrollPosition ( const int scrollBar_value );
	void changeFuncActionPointers ( vmActionGroup* grpActive );
	void updateActionButtonsState ();
	bool execRecordAction ( const int key );
	void setupTabNavigationButtons ();

	inline const QString& appMainStyle () const { return m_strStyle; }
	void changeSchemeStyle ( const QString& style, const bool b_save = false );
	
//----------------------------------SETUP-CUSTOM-CONTROLS-NAVIGATION--------------------------------------


//---------------------------------------------SESSION----------------------------------------------------------
	void showTab ( const TAB_INDEXES ti );
	void tabMain_currentTabChanged ( const int tab_idx );
	void alterLastViewedRecord ( dbListItem* old_item, dbListItem* new_item );
	uint getLastViewedRecord ( const TABLE_INFO* t_info, const uint client_id = 0, const uint job_id = 0 );
	void changeNavLabels ();
	void navigatePrev ();
	void navigateNext ();
	void insertNavItem ( dbListItem* item );
	void displayNav ();
	void insertEditItem ( dbListItem* item );
	void removeEditItem ( dbListItem* item );
	void saveEditItems ();
//---------------------------------------------SESSION----------------------------------------------------------

//-------------------------------------------ITEM-SHARED---------------------------------------------------------
	void notifyExternalChange ( const uint id, const uint table_id, const RECORD_ACTION action );
	
	inline void notifyExternalChange ( const dbListItem* const changed_item )
		{ notifyExternalChange ( changed_item->dbRec () ); }
	
	inline void notifyExternalChange ( const DBRecord* const dbrec )
		{ notifyExternalChange ( static_cast<uint>( recIntValue ( dbrec, 0 ) ), dbrec->tableInfo ()->table_id, dbrec->prevAction () ); }
	
	void removeListItem ( dbListItem* item, const bool b_delete_item = true, const bool b_auto_select_another = true );
	void postFieldAlterationActions ( dbListItem* item );
	void updateWindowBeforeSave ();
	inline clientListItem* currentClient () const { return mClientCurItem; }
	inline jobListItem* currentJob () const { return mJobCurItem; }
	inline payListItem* currentPay () const { return mPayCurItem; }
	inline buyListItem* currentBuy () const { return mBuyCurItem; }
//-------------------------------------------ITEM-SHARED---------------------------------------------------------

protected:
	void closeEvent ( QCloseEvent * ) override;
	bool eventFilter ( QObject* o, QEvent* e ) override;

private:
	Ui::MainWindow* ui;

	QSystemTrayIcon* trayIcon;
	QToolBar* mActionsToolBar;
	QAction* actionAdd, *actionEdit, *actionRemove, *actionSave, *actionCancel;
	QMenu* trayIconMenu;

	QMenu* menuJobDoc, *menuJobXls;
	QMenu* menuJobPdf, *subMenuJobPdfView;

	QString m_qpStrId;

	//---------------------------CUSTOM-WIDGETS-------------------------------------
	separateWindow* sepWin_JobPictures;
	separateWindow* sepWin_JobReport;

	crashRestore* m_crash;
	dbCalendar* mCal;
	dbTableView* mTableView;
	calendarViewUI* mCalendarView;
	dbStatistics* mStatistics;
	machinesDlg* mMachines;

	QAction* jobsPicturesMenuAction;
	QAction* actJobSelectJob;
	QAction* actPayReceipt, *actPayReport, *actPayUnPaidReport;
	vmAction* actCountDayHours, *actCountAllHours, *actAppendDayToday, *actAppendDayYesterday;

	vmTaskPanel* mainTaskPanel;

	vmActionGroup* grpActive;
	vmActionGroup* grpClients;
	vmActionGroup* grpJobs;
	vmActionGroup* grpPays;
	vmActionGroup* grpBuys;

	QString m_strStyle;

	int clientSectionPos, jobSectionPos, paySectionPos, buySectionPos;
	dbListItem* activeRecord;
	std::function<bool ( dbListItem* )> func_Save;
	std::function<dbListItem* ()> func_Add;
	std::function<bool ( dbListItem* )> func_Edit;
	std::function<bool ( dbListItem* )> func_Del;
	std::function<bool ( dbListItem* )> func_Cancel;
	
	std::function<void ( const uint )> selJob_callback;
	bool mb_jobPosActions;
	imageViewer* jobImageViewer;

	pointersList<vmWidget*> clientWidgetList;
	pointersList<vmWidget*> jobWidgetList;
	pointersList<vmWidget*> payWidgetList;
	pointersList<vmWidget*> buyWidgetList;
	pointersList<dbListItem*> navItems;
	pointersList<dbListItem*> editItems;

	clientListItem* mClientCurItem;
	jobListItem* mJobCurItem;
	payListItem* mPayCurItem;
	buyListItem* mBuyCurItem;

	QToolButton* mBtnNavPrev, *mBtnNavNext;
	QDialog* dlgSaveEditItems;

	struct actionsAvailability
	{
		bool b_canAdd;
		bool b_canEdit;
		bool b_canRemove;
		bool b_canSave;
		bool b_canCancel;
		bool b_canExtra[3];
		
		actionsAvailability () : b_canAdd ( false ), b_canEdit ( false ), b_canRemove ( false ), b_canSave ( false ),
			b_canCancel ( false ), b_canExtra { false } {}
	} sectionActions[4], *curSectionAction;

	//---------------------------CUSTOM-WIDGETS-------------------------------------

//--------------------------------------------JOB-----------------------------------------------------------
	void addJobPictures ();
	void btnJobReloadPictures_clicked ();
	void showClientsYearPictures ( QAction* action );
	void showDayPictures ( const vmNumber& date );
	void showJobImage ( const int index );
	void showJobImageRequested ( const int index );
	void btnJobRenamePicture_clicked ( const bool checked );
	void showJobImageInWindow ( const bool maximized );
	void btnJobSeparateReportWindow_clicked ( const bool checked );
//--------------------------------------------JOB-----------------------------------------------------------

//--------------------------------------------SLOTS---------------------------------------------------------
	void receiveWidgetBack ( QWidget* widget );
	void btnReportGenerator_clicked ();
	void btnBackupRestore_clicked ();
	void btnCalculator_clicked ();
	void btnEstimates_clicked ();
	void btnCompanyPurchases_clicked ();
	void btnConfiguration_clicked ();
	void btnExitProgram_clicked ();
//--------------------------------------------SLOTS-----------------------------------------------------------

//--------------------------------------------SEARCH-----------------------------------------------------------
	void on_txtSearch_textEdited ( const QString& text );
	void on_btnSearchStart_clicked ();
	void on_btnSearchCancel_clicked ();
//--------------------------------------------SEARCH-----------------------------------------------------------

//--------------------------------------------TRAY-----------------------------------------------------------
	void createTrayIcon ();
	void createFloatToolBar ();
	inline QSystemTrayIcon* appTrayIcon () const { return trayIcon; }
	void trayMenuTriggered ( QAction* );
	void trayActivated ( QSystemTrayIcon::ActivationReason );
//--------------------------------------------TRAY-----------------------------------------------------------
};

#endif // MAINWINDOW_H
