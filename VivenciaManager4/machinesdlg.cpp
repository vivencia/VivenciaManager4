#include "machinesdlg.h"
#include "system_init.h"
#include "mainwindow.h"

#include <heapmanager.h>
#include <vmStringRecord/stringrecord.h>
#include <vmNotify/vmnotify.h>
#include <dbRecords/machinesrecord.h>
#include <dbRecords/completers.h>
#include <vmWidgets/vmwidgets.h>
#include <vmWidgets/vmtablewidget.h>
#include <vmTaskPanel/vmactiongroup.h>
#include <vmUtils/fast_library_functions.h>

#include <QtWidgets/QToolButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

machinesDlg::machinesDlg ( QWidget* parent )
	: QDialog ( parent ), mMacRec ( new machinesRecord )
{
	setupUI ();
}

machinesDlg::~machinesDlg ()
{
	heap_del ( mMacRec );
	heap_del ( tableMachineHistory );
}

void machinesDlg::showJobMachineUse ( const QString& jobid )
{
	bool bFoundJod ( false );
	if ( !jobid.isEmpty () && jobid != QStringLiteral ( "-1" ) )
	{
		if ( mMacRec->action () != ACTION_READ )
		{
			if ( NOTIFY ()->questionBox ( tr ( "Save current information before showing the requested logs?" ),
											 tr ( "If you click NO, all changes will be lost." ) ) )
				btnSave_clicked ();
			else
			{
				mMacRec->setAction ( ACTION_READ );
				controlForms ();
			}
		}
		bFoundJod = loadData ( loadJobMachineEventsTable ( jobid ) );
	}
	if ( !bFoundJod )
	{
		txtJob->setInternalData ( QString ( CHR_QUESTION_MARK + jobid ) );
		txtJob->setText ( Job::jobSummary ( jobid ) );
	}
	grpJobMachineEvents->setTitle ( tr ( "Machine events for " ) + txtJob->text () );
	QDialog::show ();
}

void machinesDlg::setupUI ()
{
	setWindowFlags ( Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint );
	setWindowIcon ( ICON ( "job_machines" ) );
	setWindowTitle ( tr ( "Machines and events" ) );

	cboEvents = new vmComboBox;
	cboEvents->setID ( FLD_MACHINES_EVENTS_DESC );
	cboMachines = new vmComboBox;
	cboMachines->setMinimumWidth ( 250 );
	cboMachines->setID ( FLD_MACHINES_NAME );
	cboBrand = new vmComboBox;
	cboBrand->setID ( FLD_MACHINES_BRAND );
	cboBrand->setTextType ( vmWidget::TT_UPPERCASE );
	cboType = new vmComboBox;
	cboType->setID ( FLD_MACHINES_TYPE );
	btnAddEvent = new QToolButton;
	btnAddEvent->setIcon ( ICON ( "add" ) );
	btnAddEvent->setToolTip ( tr ( "Add an event to the selected job" ) );
	btnDelEvent = new QToolButton;
	btnDelEvent->setIcon ( ICON ( "remove" ) );
	btnDelEvent->setToolTip ( tr ( "Remove the current event for the job" ) );
	dteEventDate = new vmDateEdit;
	dteEventDate->setID ( FLD_MACHINES_EVENT_DATES );
	timeEventTime = new vmTimeEdit;
	timeEventTime->setID ( FLD_MACHINES_EVENT_TIMES );

	tableJobEvents = new vmTableWidget;
	txtJob = new vmLineEdit;
	txtJob->setID ( FLD_MACHINES_EVENT_JOBS );
	btnSelectJob = new QToolButton;
	btnSelectJob->setIcon ( ICON ( "job_info" ) );
	btnSelectJob->setToolTip ( tr ( "Choose job associated with this machine event" ) );
	QHBoxLayout* layoutJobMachineEvents ( new QHBoxLayout );
	grpJobMachineEvents = new vmActionGroup;
	grpJobMachineEvents->addQEntry ( new QLabel ( tr ( "Associated job: " ) ), layoutJobMachineEvents );
	grpJobMachineEvents->addEntry ( txtJob, layoutJobMachineEvents );
	grpJobMachineEvents->addQEntry ( btnSelectJob, layoutJobMachineEvents );
	grpJobMachineEvents->addEntry ( tableJobEvents, nullptr, true );

	tableMachineHistory = new vmTableWidget;
	grpHistory = new vmActionGroup ( tr ( "Machine event history" ) );
	//txtMachineTotalTime = new vmLineEdit;
	grpHistory->addEntry ( tableMachineHistory, new QHBoxLayout (), true );
	//grpJobMachineEvents->addQEntry ( new QLabel ( tr ( "Total use time: " ) ), layoutJobMachineEvents );

	btnEdit = new QPushButton ( ICON ( "browse-controls/edit" ), tr ( "Edit" ) );
	btnEdit->setToolTip ( tr ( "Manage machines information" ) );
	btnCancel = new QPushButton ( ICON ( "cancel" ), tr ( "Cancel" ) );
	btnCancel->setToolTip ( tr ( "Cancel alterations" ) );
	btnSave = new QPushButton ( ICON ( "document-save" ), tr ( "Save" ) );
	btnSave->setToolTip ( tr ( "Save alterations" ) );
	btnClose = new QPushButton ( ICON ( "exit_app" ), tr ( "Close" ) );
	btnClose->setToolTip ( tr ( "Close the dialog" ) );

	QHBoxLayout* layoutRow_0 ( new QHBoxLayout );
	layoutRow_0->addWidget ( new QLabel ( tr ( "Event description: " ) ) );
	layoutRow_0->addWidget ( cboEvents, 2 );
	layoutRow_0->addWidget ( btnAddEvent );
	layoutRow_0->addWidget ( btnDelEvent );

	QHBoxLayout* layoutRow_1 ( new QHBoxLayout );
	layoutRow_1->addWidget ( new QLabel ( tr ( "Machine: " ) ) );
	layoutRow_1->addWidget ( cboMachines, 2 );

	QHBoxLayout* layoutRow_2 ( new QHBoxLayout );
	layoutRow_2->addWidget ( new QLabel ( tr ( "Brand: " ) ) );
	layoutRow_2->addWidget ( cboBrand, 1 );
	layoutRow_2->addWidget ( new QLabel ( tr ( "Type: " ) ) );
	layoutRow_2->addWidget ( cboType, 1 );

	QHBoxLayout* layoutRow_3 ( new QHBoxLayout );
	layoutRow_3->addWidget ( new QLabel ( tr ( "Event date: " ) ) );
	layoutRow_3->addWidget ( dteEventDate, 1 );
	layoutRow_3->addWidget ( new QLabel ( tr ( "Event time: " ) ) );
	layoutRow_3->addWidget ( timeEventTime, 1 );

	QHBoxLayout* layoutRow_4 ( new QHBoxLayout );
	layoutRow_4->addWidget ( grpJobMachineEvents );

	QHBoxLayout* layoutRow_5 ( new QHBoxLayout );
	layoutRow_5->addWidget ( grpHistory );

	QHBoxLayout* layoutRow_6 ( new QHBoxLayout );
	layoutRow_6->addWidget ( btnEdit );
	layoutRow_6->addWidget ( btnSave );
	layoutRow_6->addWidget ( btnCancel );
	layoutRow_6->addStretch ( 2 );
	layoutRow_6->addWidget ( btnClose );

	QVBoxLayout* mainLayout ( new QVBoxLayout );
	mainLayout->addLayout ( layoutRow_0 );
	mainLayout->addLayout ( layoutRow_1 );
	mainLayout->addLayout ( layoutRow_2 );
	mainLayout->addLayout ( layoutRow_3 );
	mainLayout->addLayout ( layoutRow_4 );
	mainLayout->addLayout ( layoutRow_5 );
	mainLayout->addLayout ( layoutRow_6 );
	setLayout ( mainLayout );

	setupConnections ();
	initTableMachineEvents ();
	inittableMachineHistory ();
	fillComboBoxes ();
	loadData ( -1 );
	controlForms ();
}

void machinesDlg::setupConnections ()
{
	cboMachines->setCallbackForIndexChanged ( [&] ( const int idx ) {
		return cboMachines_IndexChanged ( idx );
	} );
	cboMachines->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dataAltered ( sender );
	} );

	cboBrand->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dataAltered ( sender );
	} );
	cboType->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dataAltered ( sender );
	} );
	cboEvents->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dataAltered ( sender );
	} );

	connect ( btnAddEvent, &QToolButton::clicked, this, [&] () {
		return btnAddEvent_clicked ();
	} );
	connect ( btnDelEvent, &QToolButton::clicked, this, [&] () {
		return btnDelEvent_clicked ();
	} );

	dteEventDate->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dataAltered ( sender );
	} );
	timeEventTime->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return dataAltered ( sender );
	} );

	txtJob->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return dataAltered ( sender ); } );
	connect ( btnSelectJob, &QToolButton::clicked, this, [&] () {
			return btnSelectJob_clicked (); } );

	connect ( btnEdit, &QPushButton::clicked, this, [&] () {
		return btnEdit_clicked ();
	} );
	connect ( btnCancel, &QPushButton::clicked, this, [&] () {
		return btnCancel_clicked ();
	} );
	connect ( btnSave, &QPushButton::clicked, this, [&] () {
		return btnSave_clicked ();
	} );
	connect ( btnClose, &QPushButton::clicked, this, [&] () {
		return canClose ();
	} );
}

void machinesDlg::initTableMachineEvents ()
{
	const uint n_cols ( 4 );
	vmTableColumn* cols ( tableJobEvents->createColumns( n_cols ) );
	uint i ( 0 );
	for ( ; i < n_cols - 1; ++i )
	{
		cols[i].editable = false;
		switch ( i )
		{
			case 0: // FLD_MACHINES_NAME
				cols[0].label = tr ( "Machine" );
				cols[0].width = 180;
			break;
			case 1: //FLD_MACHINES_EVENTS_DESC
				cols[1].label = tr ( "Event" );
				cols[1].width = 220;
			break;
			case 2: // FLD_MACHINES_EVENTS_DESC
				cols[2].wtype = WT_DATEEDIT;
				cols[2].label = tr ( "Event date" );
			break;
			case 3: //FLD_MACHINES_EVENT_TIMES
				cols[3].wtype = WT_TIMEEDIT;
				cols[3].label = tr ( "Event duration" );
			break;
		}
	}
	tableJobEvents->setIsPlainTable ();
	tableJobEvents->initTable ( 6 );
}

// Display info only
void machinesDlg::inittableMachineHistory ()
{
	tableMachineHistory->setIsPlainTable ();
	const uint n_cols ( MACHINES_FIELD_COUNT - 5 );
	vmTableColumn* cols ( tableMachineHistory->createColumns ( n_cols ) );
	uint i ( 0 );
	for ( ; i < n_cols; ++i )
	{
		cols[i].editable = false;
		switch ( i )
		{
			case FLD_MACHINES_EVENTS_DESC - 5:
				cols[FLD_MACHINES_EVENTS_DESC - 5].label = tr ( "Event" );
				cols[FLD_MACHINES_EVENTS_DESC - 5].width = 220;
			break;
			case FLD_MACHINES_EVENT_DATES - 5:
				cols[FLD_MACHINES_EVENT_DATES - 5].wtype = WT_DATEEDIT;
				cols[FLD_MACHINES_EVENT_DATES - 5].label = tr ( "Event date" );
			break;
			case FLD_MACHINES_EVENT_TIMES - 5:
				cols[FLD_MACHINES_EVENT_TIMES - 5].wtype = WT_TIMEEDIT;
				cols[FLD_MACHINES_EVENT_TIMES - 5].label = tr ( "Event duration" );
			break;
			case FLD_MACHINES_EVENT_JOBS - 5:
				cols[FLD_MACHINES_EVENT_JOBS - 5].label = tr ( "Associated job" );
				cols[FLD_MACHINES_EVENT_JOBS - 5].width = 200;
			break;
		}
	}
	tableMachineHistory->initTable ( 5 );
}

void machinesDlg::canClose ()
{
	close ();
}

void machinesDlg::controlForms ()
{
	const bool editing_action ( mMacRec->action () != ACTION_READ );
	const bool b_hasItems ( mMacRec->actualRecordInt ( FLD_MACHINES_ID ) >= 0 );
	cboMachines->setEditable ( editing_action );
	cboBrand->setEditable ( editing_action );
	cboType->setEditable ( editing_action );
	cboEvents->setEditable ( editing_action );
	cboEvents->setIgnoreChanges ( editing_action );
	btnAddEvent->setEnabled ( !editing_action );
	btnDelEvent->setEnabled ( !editing_action && b_hasItems );
	dteEventDate->setEditable ( editing_action );
	timeEventTime->setEditable ( editing_action );
	btnSelectJob->setEnabled ( editing_action );
	btnEdit->setEnabled ( mMacRec->action () == ACTION_READ && b_hasItems );
	btnCancel->setEnabled ( editing_action );
	btnSave->setEnabled ( editing_action );
}

void machinesDlg::clearForms ()
{
	cboMachines->clearEditText ();
	cboBrand->clearEditText ();
	cboType->clearEditText ();
	cboEvents->clearEditText ();
	tableMachineHistory->clear ();
	dteEventDate->setDate ( vmNumber () );
	timeEventTime->setTime ( vmNumber () );
	tableJobEvents->clear ();
	const QString jobid ( txtJob->internalData ().toString () );
	if ( jobid.isEmpty () || jobid == QStringLiteral ( "-1" ) )
		txtJob->clear ();
}

void machinesDlg::fillComboBoxes ()
{
	QStringList list;
	COMPLETERS ()->fillList ( MACHINE_EVENT, list );
	cboEvents->addItems ( list );
	COMPLETERS ()->fillList ( MACHINE_NAME, list );
	cboMachines->addItems ( list );
	COMPLETERS ()->fillList ( STOCK_TYPE, list );
	cboType->addItems ( list );
	COMPLETERS ()->fillList ( BRAND, list );
	cboBrand->addItems ( list );
}

int machinesDlg::loadJobMachineEventsTable ( const QString& jobid )
{
	int machine_id ( -1 );
	if ( !jobid.isEmpty () && jobid != QStringLiteral ( "-1" ) )
	{
		machinesRecord mrec;
		if ( mrec.readFirstRecord () )
		{
			tableJobEvents->clear ();
			stringRecord jobs;
			uint field ( 0 );
			uint row ( 0 );
			do
			{
				jobs.fromString ( recStrValue ( &mrec, FLD_MACHINES_EVENT_JOBS ) );
				if ( jobs.first () )
				{
					do
					{
						if ( jobs.curValue () == jobid )
						{
							tableJobEvents->setCellValue ( recStrValue ( &mrec, FLD_MACHINES_NAME ), row, 0 );
							tableJobEvents->setCellValue ( stringRecord::fieldValue ( recStrValue ( &mrec, FLD_MACHINES_EVENTS_DESC ), field ), row, 1 );
							tableJobEvents->setCellValue ( stringRecord::fieldValue ( recStrValue ( &mrec, FLD_MACHINES_EVENT_DATES ), field ), row, 2 );
							tableJobEvents->setCellValue ( stringRecord::fieldValue ( recStrValue ( &mrec, FLD_MACHINES_EVENT_TIMES ), field ), row, 3 );
						}
						++field;
					} while ( jobs.next () );
				}
				++row;
				machine_id = recIntValue ( &mrec, FLD_MACHINES_ID ); // return the id for the last (most recent) recorded event
			} while ( mrec.readNextRecord () );
		}
	}
	txtJob->setInternalData ( jobid );
	if ( machine_id == -1 )
		NOTIFY ()->notifyMessage ( tr ( "Invalid Job ID" ), tr ( "There were no records associated with this job ID (" ) + jobid + CHR_R_PARENTHESIS );
	return machine_id;
}

QString machinesDlg::decodeStringRecord ( const uint field )
{
	const stringRecord& strRec ( recStrValue ( mMacRec, field ) );
	if ( strRec.first () )
	{
		const uint column ( field - 5 );
		uint row ( 0 );
		do {
			tableJobEvents->setCellValue ( strRec.curValue (), row++, column );
		} while ( strRec.next () );
	}
	return strRec.curValue ();
}

bool machinesDlg::loadData ( const int id )
{
	bool ok ( false );
	if ( mMacRec->action () == ACTION_READ )
	{
		clearForms ();
		ok = ( id <= -1 ) ? mMacRec->readLastRecord () : mMacRec->readRecord ( static_cast<uint>( id ) );
		if ( ok )
		{
			cboEvents->setText ( decodeStringRecord ( FLD_MACHINES_EVENTS_DESC ) );
			cboMachines->setText ( decodeStringRecord ( FLD_MACHINES_NAME ) );
			//force cboMachines' index change signal call
			this->cboMachines_IndexChanged ( 0 );

			cboBrand->setText ( decodeStringRecord ( FLD_MACHINES_BRAND ) );
			cboType->setText ( decodeStringRecord ( FLD_MACHINES_TYPE ) );
			dteEventDate->setDate ( vmNumber ( decodeStringRecord ( FLD_MACHINES_EVENT_DATES ), VMNT_DATE, vmNumber::VDF_DB_DATE ) );
			timeEventTime->setTime ( vmNumber ( decodeStringRecord ( FLD_MACHINES_EVENT_TIMES ), VMNT_TIME, vmNumber::VTF_DAYS ) );
			txtJob->setText ( Job::jobSummary ( decodeStringRecord ( FLD_MACHINES_EVENT_JOBS ) ) );
		}
	}
	return ok;
}

void machinesDlg::getSelectedJobID ( const uint jobid )
{
	qApp->setActiveWindow ( this );
	setFocus ();
	if ( jobid >= 1 )
	{
		const QString strJobID ( QString::number ( jobid ) );
		txtJob->setText ( Job::jobSummary ( strJobID ) );
		txtJob->setInternalData ( strJobID );
		stringRecord curData ( recStrValue ( mMacRec, FLD_MACHINES_EVENT_JOBS ) );
		if ( cboEvents->currentIndex () >= 0 )
			curData.changeValue ( static_cast<uint>( cboEvents->currentIndex () ), strJobID );
		else
			curData.insertField ( 0, QString::number ( jobid ) );
		setRecValue ( mMacRec, FLD_MACHINES_EVENT_JOBS, curData.toString () );
	}
	else
		txtJob->setInternalData ( QStringLiteral ( "-1" ) );
}

void machinesDlg::cboMachines_IndexChanged ( const int idx )
{
	if ( idx < 0 ) return;

	machinesRecord mrec;
	if ( mrec.readFirstRecord () )
	{
		grpHistory->setTitle ( cboMachines->text () + tr ( " event history" ) );
		QString recValue;
		stringRecord machineName;
		tableMachineHistory->clear ();
		int index ( -1 );
		uint row ( 0 );
		do
		{
			recValue = recStrValue ( &mrec, FLD_MACHINES_NAME );
			if ( recValue.contains ( cboMachines->text (), Qt::CaseInsensitive ) )
			{
				machineName.fromString ( recValue );
				while ( ( index = machineName.field ( cboMachines->text (), index ) ) >= 0 )
				{
					tableMachineHistory->setCellValue ( stringRecord::fieldValue ( recStrValue ( &mrec, FLD_MACHINES_EVENTS_DESC ), static_cast<uint>( index ) ), row, 0 );
					tableMachineHistory->setCellValue ( stringRecord::fieldValue ( recStrValue ( &mrec, FLD_MACHINES_EVENT_DATES ), static_cast<uint>( index ) ), row, 1 );
					tableMachineHistory->setCellValue ( stringRecord::fieldValue ( recStrValue ( &mrec, FLD_MACHINES_EVENT_TIMES ), static_cast<uint>( index ) ), row, 2 );
					tableMachineHistory->setCellValue ( Job::jobSummary ( stringRecord::fieldValue ( recStrValue ( &mrec, FLD_MACHINES_EVENT_JOBS ), index ) ), row, 3 );
					row++;
				}
			}
		} while ( mrec.readNextRecord () );
	}
}

void machinesDlg::dataAltered ( const vmWidget* const sender )
{
	int eventIndex ( cboEvents->currentIndex () );
	QString data;
	stringRecord curData ( recStrValue ( mMacRec, static_cast<uint>( sender->id () ) ) );

	switch ( sender->id () )
	{
		default:
			data = sender->text ();
		break;
		case FLD_MACHINES_EVENT_JOBS:
			data = txtJob->internalData ().toString ();
		break;
		case FLD_MACHINES_EVENT_DATES:
			data = dteEventDate->date ().toString ( DATE_FORMAT_DB );
		break;
		case FLD_MACHINES_EVENT_TIMES:
			data = timeEventTime->time ().toString ( TIME_FORMAT_DEFAULT );
		break;
	}
	if ( eventIndex >= 0 )
		curData.changeValue ( static_cast<uint>( eventIndex ), data );
	else
		curData.insertField ( 0, data );
	setRecValue ( mMacRec, static_cast<uint>( sender->id () ), curData.toString () );
}

void machinesDlg::btnAddEvent_clicked ()
{
	mMacRec->setAction ( ACTION_ADD );
	controlForms ();
	clearForms ();
	cboEvents->setCurrentIndex ( -1 );
	cboEvents->setFocus ();
}

void machinesDlg::btnDelEvent_clicked ()
{
	stringRecord strRec ( recStrValue ( mMacRec, FLD_MACHINES_EVENTS_DESC ) );
	const int field ( strRec.field ( cboEvents->text () ) );
	if ( field >= 0 )
	{
		if ( NOTIFY ()->questionBox ( tr ( "Machines Events" ), tr ( "Confirm removing event " ) + cboEvents->text () + CHR_QUESTION_MARK ) )
		{
			strRec.removeField ( static_cast<uint>( field ) );
			setRecValue ( mMacRec, FLD_MACHINES_EVENTS_DESC, strRec.toString () );
			strRec.fromString ( recStrValue ( mMacRec, FLD_MACHINES_EVENT_DATES ) );
			strRec.removeField ( static_cast<uint>( field ) );
			setRecValue ( mMacRec, FLD_MACHINES_EVENT_DATES, strRec.toString () );
			strRec.fromString ( recStrValue ( mMacRec, FLD_MACHINES_EVENT_TIMES ) );
			strRec.removeField ( static_cast<uint>( field ) );
			setRecValue ( mMacRec, FLD_MACHINES_EVENT_TIMES, strRec.toString () );
			strRec.fromString ( recStrValue ( mMacRec, FLD_MACHINES_EVENT_JOBS ) );
			strRec.removeField ( static_cast<uint>( field ) );
			setRecValue ( mMacRec, FLD_MACHINES_EVENT_JOBS, strRec.toString () );
			if ( mMacRec->saveRecord () )
			{
				tableMachineHistory->removeRow ( static_cast<uint>( field ) );
				(void) loadData ( -1 );
				NOTIFY ()->notifyMessage ( tr ( "Machines Events" ), tr ( "Event info deleted!" ) );
			}
		}
	}
}

void machinesDlg::btnSelectJob_clicked ()
{
	MAINWINDOW ()->setTempCallbackForJobSelect ( [&] ( const uint jobid ) {
			return getSelectedJobID ( jobid ); } );
	MAINWINDOW ()->selectJob ();
}

void machinesDlg::btnEdit_clicked ()
{
	mMacRec->setAction ( ACTION_EDIT );
	controlForms ();
	cboEvents->setFocus ();
}

void machinesDlg::btnCancel_clicked ()
{
	mMacRec->setAction ( ACTION_READ );
	controlForms ();
	(void) loadData ( mMacRec->actualRecordInt ( FLD_MACHINES_ID ) );
}

void machinesDlg::btnSave_clicked ()
{
	if ( txtJob->internalData ().toString ().contains ( CHR_QUESTION_MARK ) )
	{
		txtJob->setInternalData ( txtJob->internalData ().toString ().remove ( CHR_QUESTION_MARK ) );
		dataAltered ( txtJob );
	}
	if ( mMacRec->saveRecord () )
	{
		if ( cboEvents->insertItemSorted ( cboEvents->text () ) != -1 )
			COMPLETERS ()->updateCompleter ( cboEvents->text (), MACHINE_EVENT );
		if ( cboEvents->insertItemSorted ( cboMachines->text () ) != -1 )
			COMPLETERS ()->updateCompleter ( cboMachines->text (), MACHINE_NAME );
		if ( cboBrand->insertItemSorted ( cboBrand->text () ) != -1 )
			COMPLETERS ()->updateCompleter ( cboBrand->text (), BRAND );
		if ( cboType->insertItemSorted ( cboType->text () ) != -1 )
			COMPLETERS ()->updateCompleter ( cboType->text (), STOCK_TYPE );
		
		auto row ( static_cast<uint>( tableMachineHistory->lastUsedRow () + 1 ) );
		tableMachineHistory->setCellValue ( cboEvents->text (), row, 0 );
		tableMachineHistory->setCellValue ( dteEventDate->text (), row, 1 );
		tableMachineHistory->setCellValue ( timeEventTime->text (), row, 2 );
		tableMachineHistory->setCellValue ( txtJob->text (), row, 3 );

		row = static_cast<uint>( tableJobEvents->lastUsedRow () + 1 );
		tableJobEvents->setCellValue ( cboMachines->text (), row, 0 );
		tableJobEvents->setCellValue ( cboEvents->text (), row, 1 );
		tableJobEvents->setCellValue ( dteEventDate->text (), row, 2 );
		tableJobEvents->setCellValue ( timeEventTime->text (), row, 3 );

		NOTIFY ()->notifyMessage ( tr ( "Machine Events" ), tr ( "Machine event info saved" ) );
		cboEvents->insertItemSorted ( cboMachines->text () );
		controlForms ();
	}
}
