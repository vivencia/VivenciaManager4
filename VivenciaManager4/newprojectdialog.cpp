#include "newprojectdialog.h"
#include "system_init.h"
#include "mainwindow.h"

#include <vmUtils/fast_library_functions.h>
#include <vmUtils/configops.h>
#include <vmWidgets/vmwidgets.h>
#include <vmWidgets/vmlistwidget.h>
#include <dbRecords/vivenciadb.h>
#include <dbRecords/dblistitem.h>
#include <dbRecords/job.h>
#include <vmNotify/vmnotify.h>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

static inline void fillClientsNamesList ( vmComboBox* combo )
{
	for ( uint i ( VDB ()->getLowestID ( TABLE_CLIENT_ORDER ) ); i <= VDB ()->getHighestID ( TABLE_CLIENT_ORDER ); ++i )
		combo->insertItemSorted ( Client::clientName ( i ) );
}

static inline void fillJobTypeList ( QStringList& list, const QString& clientid )
{
	Job job;
	if ( job.readFirstRecord ( FLD_JOB_CLIENTID, clientid ) )
	{
		do
		{
			list.append ( job.jobTypeWithDate () );
		} while ( job.readNextRecord ( true ) );
	}
}

newProjectDialog::newProjectDialog ( QWidget *parent )
	: QDialog ( parent, Qt::Tool ), mJobItem ( nullptr ), bresult ( false )
{
	auto lblClient ( new QLabel ( TR_FUNC ( "Client:" ) ) );
	cboClients = new vmComboBox;
	cboClients->setCallbackForIndexChanged ( [&] ( const int idx ) { return loadJobsList ( Client::clientID ( cboClients->itemText ( idx ) ) ); } );
	
	auto lblJobType ( new QLabel ( TR_FUNC ( "Select target job ..." ) ) );
	lstJobTypes = new vmListWidget;
	lstJobTypes->setCallbackForCurrentItemChanged ( [&] ( vmListItem* current ) {
		return jobTypeItemSelected ( static_cast<dbListItem*>( current ) ); } );

	auto lblProjectName ( new QLabel ( TR_FUNC ( "Project Name:" ) ) );
	txtProjectName = new vmLineEdit;
	txtProjectName->setCallbackForContentsAltered ( [&] ( const vmWidget* const widget ) {
		return txtProjectNameAltered ( widget ); } );

	btnChooseExistingDir = new QToolButton;
	btnChooseExistingDir->setIcon( ICON ( "folder-brown" ) );
	connect ( btnChooseExistingDir, &QToolButton::clicked, this, [&] () { return btnChooseExistingDir_clicked (); } );

	chkUseDefaultName = new vmCheckBox ( TR_FUNC ( "Use automatically generated name" ) );
	chkUseDefaultName->setChecked ( true );
	chkUseDefaultName->setCallbackForContentsAltered ( [&] ( const vmWidget* const ) { return chkUseDefaultName_checked (); } );
	chkUseDefaultName->setEditable ( true );

	btnOK = new QPushButton ( TR_FUNC ( "Accept" ) );
	connect ( btnOK, &QPushButton::clicked, this, [&] () { return btnOK_clicked (); } );
	btnCancel = new QPushButton ( TR_FUNC ( "Cancel" ) );
	connect ( btnCancel, &QPushButton::clicked, this, [&] () { return btnCancel_clicked (); } );

	auto vbLayout ( new QVBoxLayout );
	vbLayout->setMargin ( 2 );
	vbLayout->setSpacing ( 2 );
	vbLayout->addWidget ( lblClient );
	vbLayout->addWidget ( cboClients, 2 );
	vbLayout->addWidget( lblJobType );
	vbLayout->addWidget( lstJobTypes, 2 );

	auto gLayout ( new QGridLayout );
	gLayout->setMargin ( 2 );
	gLayout->setSpacing ( 2 );
	gLayout->setColumnStretch ( 0, 3 );
	gLayout->addWidget ( lblProjectName, 0, 0, 1, 3 );
	gLayout->addWidget ( txtProjectName, 1, 0, 1, 3 );
	gLayout->addWidget ( btnChooseExistingDir, 1, 4, 1, 1 );
	gLayout->addWidget ( chkUseDefaultName, 2, 0, 2, 4 );
	gLayout->addItem ( new QSpacerItem ( 0, 0 ), 3, 0, 5, 4 );

	auto btnsLayout ( new QHBoxLayout );
	btnsLayout->addWidget ( btnOK );
	btnsLayout->addWidget ( btnCancel );
	gLayout->addLayout ( btnsLayout, 6, 0, 6, 5 );

	auto mainLayout ( new QHBoxLayout );
	mainLayout->setMargin ( 0 );
	mainLayout->setSpacing ( 2 );
	mainLayout->addLayout ( vbLayout, 1 );
	mainLayout->addLayout ( gLayout, 1 );
	setLayout ( mainLayout );
}

void newProjectDialog::showDialog ( const QString& clientname, const bool b_allow_other_client, const bool b_allow_name_change )
{
	if ( cboClients->count () == 0 )
		fillClientsNamesList ( cboClients );

	if ( !clientname.isEmpty () ) {
		cboClients->setCurrentIndex ( cboClients->findText ( clientname ) );
		cboClients->setCurrentText ( clientname );
		loadJobsList ( Client::clientID ( clientname ) );
	}
	cboClients->setEditable ( b_allow_other_client );
	cboClients->setIgnoreChanges ( !b_allow_other_client );
	txtProjectName->setEnabled ( b_allow_name_change );
	btnChooseExistingDir->setEnabled ( b_allow_name_change );
	chkUseDefaultName->setEnabled ( b_allow_name_change );
	exec ();
}

void newProjectDialog::loadJobsList ( const uint clientid )
{
	lstJobTypes->setIgnoreChanges ( true );
	lstJobTypes->clear ();
	txtProjectName->clear ();
	mProjectID.clear ();
	mProjectPath.clear ();

	QStringList jobTypesList;
	fillJobTypeList ( jobTypesList, QString::number ( clientid ) );

	QString jobid;
	mClientItem = MAINWINDOW ()->getClientItem ( clientid );
	jobListItem* job_parent ( nullptr );
	QStringList::const_iterator itr ( jobTypesList.constBegin () );
	const QStringList::const_iterator& itr_end ( jobTypesList.constEnd () );
	for ( ; itr != itr_end; ++itr )
	{
		jobid = (*itr).mid ( 1, (*itr).indexOf ( CHR_R_PARENTHESIS, 2 ) - 1 );
		job_parent = MAINWINDOW ()->getJobItem ( mClientItem, jobid.toUInt () );
		if ( job_parent )
		{
			mJobItem = static_cast<jobListItem*>( job_parent->relatedItem ( RLI_EXTRAITEMS ) );
			if ( mJobItem == nullptr )
			{
				mJobItem = new jobListItem;
				mJobItem->setRelation ( RLI_EXTRAITEMS );
				job_parent->syncSiblingWithThis ( mJobItem );
				mJobItem->setText ( *itr, false, false, false );
			}
			mJobItem->addToList ( lstJobTypes );
		}
	}
	lstJobTypes->setIgnoreChanges ( false );
	lstJobTypes->setCurrentItem ( mJobItem, true, true );
}

void newProjectDialog::jobTypeItemSelected ( dbListItem* item )
{
	if ( item != nullptr )
	{
		mJobItem = static_cast<jobListItem*> ( item );
		if ( mJobItem->loadData ( true ) )
		{
			if ( chkUseDefaultName->isChecked () )
			{
				txtProjectName->setText ( mJobItem->jobRecord ()->date ( FLD_JOB_STARTDATE ).toDate ( vmNumber::VDF_FILE_DATE ) +
							QStringLiteral ( " - " ) + recStrValue ( mJobItem->jobRecord (), FLD_JOB_TYPE ), true );
			}
			else
				txtProjectName->setText ( item->data ( Qt::UserRole ).toString (), true );
		}
	}
}

void newProjectDialog::txtProjectNameAltered ( const vmWidget* const )
{
	if ( !txtProjectName->text ().contains ( CHR_F_SLASH ) )
	{
		mProjectID = mJobItem->jobRecord ()->date ( FLD_JOB_STARTDATE ).toDate ( vmNumber::VDF_FILE_DATE );
		if ( !txtProjectName->text ().startsWith ( mProjectID ) )
			txtProjectName->setText ( mProjectID + QStringLiteral ( " - " ) + txtProjectName->text (), false );
		mProjectPath = CONFIG ()->getProjectBasePath ( recStrValue ( mClientItem->clientRecord (), FLD_CLIENT_NAME ) ) 
				+ txtProjectName->text () + CHR_F_SLASH;
	}
	else
	{
		mProjectID = fileOps::nthDirFromPath ( txtProjectName->text () );
		mProjectID = mProjectID.left ( mProjectID.indexOf ( CHR_SPACE ) );
		mProjectPath = txtProjectName->text ();
		if ( mProjectPath.at ( mProjectPath.count () - 1 ) != CHR_F_SLASH )
			mProjectPath += CHR_F_SLASH;
	}
	lstJobTypes->currentItem ()->setData ( Qt::UserRole, txtProjectName->text () );
}

void newProjectDialog::btnChooseExistingDir_clicked ()
{
	QString newProjectDir ( fileOps::getExistingDir ( CONFIG ()->getProjectBasePath (
								recStrValue ( static_cast<clientListItem*> ( mJobItem->relatedItem ( RLI_CLIENTPARENT ) )->clientRecord (), FLD_CLIENT_NAME ) ) ) );
	if ( !newProjectDir.isEmpty () )
	{
		newProjectDir += CHR_F_SLASH;
		txtProjectName->setText ( newProjectDir, true );
	}
}

void newProjectDialog::chkUseDefaultName_checked ()
{
	jobTypeItemSelected ( static_cast<dbListItem*>( lstJobTypes->currentItem () ) );
	txtProjectName->setEditable ( !chkUseDefaultName->isChecked () );
}

void newProjectDialog::btnOK_clicked ()
{
	if ( NOTIFY ()->questionBox ( TR_FUNC ( "Confirmation" ), TR_FUNC ( "Proceed with creating a project for the following job: " ) +
								mJobItem->jobRecord ()->jobSummary () + CHR_QUESTION_MARK ) )
	{
		bresult = true;
		close ();
	}
}

void newProjectDialog::btnCancel_clicked ()
{
	bresult = false;
	close ();
}
