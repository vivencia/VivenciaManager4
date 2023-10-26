#include "estimates.h"
#include "global.h"
#include "system_init.h"
#include "newprojectdialog.h"
#include "mainwindow.h"

#include <heapmanager.h>
#include <vmUtils/configops.h>
#include <vmNotify/vmnotify.h>
#include <vmWidgets/vmwidgets.h>
#include <dbRecords/vivenciadb.h>
#include <vmTaskPanel/vmtaskpanel.h>
#include <vmTaskPanel/vmactiongroup.h>
#include <vmDocumentEditor/documenteditor.h>
#include <vmDocumentEditor/reportgenerator.h>

#include <QtSql/QSqlQuery>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>

#ifdef USING_QT6
#include <QtGui/QAction>
#else
#include <QtWidgets/QAction>
#endif

static QToolButton* btnDoc ( nullptr );
static QToolButton* btnXls ( nullptr );
static QToolButton* btnPdf ( nullptr );
static QToolButton* btnVmr ( nullptr );
static QToolButton* btnFileManager ( nullptr );
static QToolButton* btnReload ( nullptr );
static QPushButton* btnProject ( nullptr );
static QPushButton* btnEstimate ( nullptr );
static QPushButton* btnReport ( nullptr );
static QPushButton* btnClose ( nullptr );
static QMenu* menuDoc ( nullptr );
static QMenu* menuVmr ( nullptr );
static QMenu* menuXls ( nullptr );
static QMenu* menuPdf ( nullptr );
static QMenu* menuFileMngr ( nullptr );
static QMenu* menuProject ( nullptr );
static QMenu* menuEstimate ( nullptr );
static QMenu* menuReport ( nullptr );
static QTreeWidget* treeView ( nullptr );
static QIcon iconsType[5];

static const uint TYPE_PROJECT_ITEM ( 0 );
static const uint TYPE_ESTIMATE_ITEM ( 1 );
static const uint TYPE_REPORT_ITEM ( 2 );
static const uint TYPE_PROJECT_DIR ( 3 );
static const uint TYPE_ESTIMATE_DIR ( 4 );
static const uint TYPE_REPORT_DIR ( 5 );
static const uint TYPE_CLIENT_TOPLEVEL ( 1000 );

const int ROLE_ITEM_TYPE ( Qt::UserRole );
const int ROLE_ITEM_FILENAME ( Qt::UserRole + 1 );
const int ROLE_ITEM_CLIENTNAME ( Qt::UserRole + 2 );

static bool b_inItemSelectionMode ( false );
static QTreeWidgetItem* targetItemInSelectionMode ( nullptr );

enum FILETYPES
{
	FILETYPE_DOC = 0, FILETYPE_XLS = 1, FILETYPE_PDF = 2, FILETYPE_VMR = 3, FILETYPE_UNKNOWN = 4
};

enum SINGLE_FILE_OPS
{
	SFO_OPEN_FILE = 0, SFO_REMOVE_FILE = 1
};

enum FILEMANAGER_ACTIONS
{
	OPEN_SELECTED_DIR = 10, OPEN_ESTIMATES_DIR = 11, OPEN_REPORTS_DIR = 12, OPEN_CLIENT_DIR = 13
};

enum PROJECT_ACTIONS
{
	PA_NEW_FULL = 0, PA_NEW_EMPTY = 1, PA_CONVERT_FROM_ESTIMATE = 2, PA_RENAME = 3, 
		PA_LINK_TO_JOB = 4, PA_REMOVE = 5, PA_ADD_DOC = 6, PA_ADD_XLS = 7
};

enum ESTIMATE_ACTIONS
{
	EA_NEW_EXTERNAL = 0, EA_NEW_VMR = 1, EA_CONVERT = 2, EA_REMOVE = 3, EA_ADD_DOC = 4, EA_ADD_XLS = 5
};

enum REPORT_ACTIONS
{
	RA_NEW_EXTERNAL = 0, RA_NEW_EXTERNAL_COMPLETE = 1, RA_NEW_VMR = 2, RA_REMOVE = 3
};

static const QString nonClientStr ( QStringLiteral ( "Não-clientes" ) );

// Not used anymore but kept so that we maintain compatibility with the old-style filename, since those files will not be renamed
static const QString nonClientEstimatesPrefix ( QStringLiteral ( "Orçamento - " ) );
static const QString nonClientReportsPrefix ( QStringLiteral ( "Relatório - " ) );
static const QString itemTypeStr[6] =
{
	QStringLiteral ( "Projeto" ), QStringLiteral ( "Orçamento" ), QStringLiteral ( "Relatório" ),
	QStringLiteral ( "Diretório de Projeto" ), QStringLiteral ( "Diretório de Orçamentos" ), QStringLiteral ( "Diretório de Relatórios" )
};

//------------------------------------ESTIMATES------------------------------------------

// This func () is execed by the actions initiated from the buttons, which, in turn, will only
// be enabled if it's guaranteed they will produce a valid item and, therefore, valid filenames
#define execMenuAction(action) \
	execAction ( treeView->selectedItems ().at ( 0 ), \
				 static_cast<vmAction*> ( action )->id () );

inline static QString getNameFromProjectFile ( const QString& filename )
{
	return filename.left ( filename.indexOf ( CHR_DOT, 1 ) );
}

static QString getDateFromProjectFile ( const QString& filename )
{
	bool b_digit ( false );
	QString strDate;
	QString::const_iterator itr ( filename.constBegin () );
	const QString::const_iterator& itr_end ( filename.constEnd () );
	for ( ; itr != itr_end; ++itr )
	{
		if ( b_digit )
		{
			b_digit = static_cast<QChar> ( *itr ).isDigit ();
			if ( b_digit )
				strDate += static_cast<QChar> ( *itr );
		}
		else
		{
			b_digit = static_cast<QChar> ( *itr ).isDigit ();
			if ( !b_digit )
			{
				if ( strDate.length () < 4 )
					strDate.clear ();
			}
			else
				strDate += static_cast<QChar> ( *itr );
		}
	}
	if ( strDate.length () >= 4 )
	{
		const vmNumber date ( strDate, VMNT_DATE );
		return date.toDate ( vmNumber::VDF_HUMAN_DATE );
	}
	return emptyString;
}

static QString getDateFromProjectDir ( const QString& filename )
{
	vmNumber date;
	date.fromStrDate ( filename.left ( filename.indexOf ( CHR_SPACE ) ) );
	return date.toDate ( vmNumber::VDF_HUMAN_DATE );
}

static QString getNameFromEstimateFile ( const QString& filename )
{
	const QString estimateFilePrefix ( filename.startsWith ( nonClientEstimatesPrefix ) ?
										 nonClientEstimatesPrefix : emptyString );
	const int idx ( filename.indexOf ( CHR_HYPHEN, estimateFilePrefix.count () + 1 ) );
	if ( idx != -1 )
		return filename.mid ( idx + 2, filename.indexOf ( CHR_DOT, idx + 2 ) - ( idx + 2 ) );
	return emptyString;
}

static QString getDateFromEstimateFile ( const QString& filename )
{
	const QString estimateFilePrefix ( filename.startsWith ( nonClientEstimatesPrefix ) ?
										 nonClientEstimatesPrefix : emptyString );

	const int idx ( filename.indexOf ( CHR_HYPHEN, estimateFilePrefix.count () + 1 ) );
	if ( idx != -1 )
	{
		const QString datestr ( filename.mid (
									estimateFilePrefix.count (), idx - estimateFilePrefix.count () - 1 ) );
		vmNumber date ( datestr, VMNT_DATE, vmNumber::VDF_FILE_DATE );
		return date.toDate( vmNumber::VDF_HUMAN_DATE );
	}
	return emptyString;
}

static QString getNameFromReportFile ( const QString& filename )
{
	const QString reportName ( fileOps::fileNameWithoutPath ( filename ) );
	if ( !reportName.isEmpty () )
	{
		return fileOps::filePathWithoutExtension ( reportName );
	}
	return reportName;
}

static QString getDateFromReportFile ( const QString& filename )
{
	vmNumber date, time;
	if ( fileOps::modifiedDateTime ( filename, date, time ) )
		return date.toString ();
	return emptyString;
}

/*static QString tryToGetClientAddress ( const QString& client_name )
{
	Client client;
	if ( client.readRecord ( FLD_CLIENT_NAME, client_name ) )
		return recStrValue ( &client, FLD_CLIENT_EMAIL );
	return emptyString;
}*/

estimateDlg::estimateDlg ( QWidget* parent )
	: QDialog ( parent ), m_npdlg ( nullptr )
{
	iconsType[FILETYPE_DOC] = ICON ( "x-office-document" );
	iconsType[FILETYPE_XLS] = ICON ( "x-office-spreadsheet" );
	iconsType[FILETYPE_PDF] = ICON ( "okular" );
	iconsType[FILETYPE_VMR] = ICON ( "x-office-address-book" );
	iconsType[FILETYPE_UNKNOWN] = ICON ( "unknown" );

	setWindowTitle ( TR_FUNC ( "Projects, Estimates and Reports" ) );
	setWindowIcon ( iconsType[FILETYPE_VMR] );
	
	btnDoc = new QToolButton;
	btnDoc->setIcon ( iconsType[FILETYPE_DOC] );
	connect ( btnDoc, &QToolButton::pressed, btnDoc, [&] () { return btnDoc->showMenu (); } );
	btnVmr = new QToolButton;
	btnVmr->setIcon ( iconsType[FILETYPE_VMR] );
	connect ( btnVmr, &QToolButton::pressed, btnVmr, [&] () { return btnVmr->showMenu (); } );
	btnXls = new QToolButton;
	btnXls->setIcon ( iconsType[FILETYPE_XLS] );
	connect ( btnXls, &QToolButton::pressed, btnXls, [&] () { return btnXls->showMenu (); } );
	btnPdf = new QToolButton;
	btnPdf->setIcon ( iconsType[FILETYPE_PDF] );
	connect ( btnPdf, &QToolButton::pressed, btnPdf, [&] () { return btnPdf->showMenu (); } );
	btnFileManager = new QToolButton;
	btnFileManager->setIcon ( QIcon::fromTheme ( QStringLiteral ( "system-file-manager" ) ) );
	connect ( btnFileManager, &QToolButton::pressed, btnFileManager, [&] () { return btnFileManager->showMenu (); } );
	btnReload = new QToolButton;
	btnReload->setIcon ( QIcon::fromTheme ( QStringLiteral ( "view-refresh" ) ) );
	connect ( btnReload, &QToolButton::pressed, this, [&] () { return scanDir (); } );

	auto vLayout1 ( new QVBoxLayout );
	vLayout1->setContentsMargins ( 1, 1, 1, 1 );
	vLayout1->setSpacing ( 1 );
	vLayout1->addWidget ( btnDoc );
	vLayout1->addWidget ( btnVmr );
	vLayout1->addWidget ( btnXls );
	vLayout1->addWidget ( btnPdf );
	vLayout1->addWidget ( btnFileManager );
	vLayout1->addWidget ( btnReload );

	treeView = new QTreeWidget;
	treeView->setAlternatingRowColors ( true );
	treeView->setMinimumSize ( 500, 300 );
	treeView->setSortingEnabled ( true );
	treeView->sortByColumn ( 0, Qt::AscendingOrder );
	treeView->setColumnCount ( 3 );
	treeView->setHeaderLabels ( QStringList () << TR_FUNC ( "File name" ) << TR_FUNC ( "Document type" ) << TR_FUNC ( "Date" ) );
	treeView->setColumnWidth ( 0, 350 );
	treeView->setColumnWidth ( 1, 150 );
	treeView->setSelectionMode ( QAbstractItemView::SingleSelection );
	connect ( treeView, &QTreeWidget::itemSelectionChanged, this, [&] () { updateButtons (); } );
	connect ( treeView, &QTreeWidget::itemDoubleClicked, this, [&] ( QTreeWidgetItem* item, int ) { return openWithDoubleClick ( item ); } );

	auto hLayout2 ( new QHBoxLayout );
	hLayout2->setContentsMargins ( 2, 2, 2, 2 );
	hLayout2->setSpacing ( 2 );
	hLayout2->addWidget ( treeView, 2 );
	hLayout2->addLayout ( vLayout1, 0 );

	btnEstimate = new QPushButton ( TR_FUNC ( "Estimate" ) );
	connect ( btnEstimate, &QPushButton::clicked, btnEstimate, [&] () { return btnEstimate->showMenu (); } );

	btnProject = new QPushButton ( TR_FUNC ( "Project" ) );
	connect ( btnProject, &QPushButton::clicked, btnProject, [&] () { return btnProject->showMenu (); } );

	btnReport = new QPushButton ( TR_FUNC ( "Report" ) );
	connect ( btnReport, &QPushButton::clicked, btnReport, [&] () { return btnReport->showMenu (); } );

	btnClose = new QPushButton ( TR_FUNC ( "Close" ) );
	connect ( btnClose, &QPushButton::clicked, this, [&] () { b_inItemSelectionMode ? cancelItemSelection () : static_cast<void>( showMinimized () ); } );

	auto hLayout3 ( new QHBoxLayout );
	hLayout3->setContentsMargins ( 2, 2, 2, 2 );
	hLayout3->setSpacing ( 2 );
	hLayout3->addWidget ( btnProject, 1, Qt::AlignCenter );
	hLayout3->addWidget ( btnEstimate, 1, Qt::AlignCenter );
	hLayout3->addWidget ( btnReport, 1, Qt::AlignCenter );
	hLayout3->addWidget ( btnClose, 1, Qt::AlignCenter );

	vmTaskPanel* panel ( new vmTaskPanel ( emptyString, this ) );
	auto mainLayout ( new QVBoxLayout );
	mainLayout->setContentsMargins ( 0, 0, 0, 0 );
	mainLayout->setSpacing ( 0 );
	mainLayout->addWidget ( panel, 1 );
	setLayout ( mainLayout );
	vmActionGroup* group ( panel->createGroup ( emptyString, false, true, false ) );
	group->addLayout ( hLayout2, 2 );
	group->addLayout ( hLayout3, 0 );
	MAINWINDOW ()->appMainStyle ( panel );

	setupActions ();
	scanDir ();
	updateButtons ();
}

estimateDlg::~estimateDlg () 
{
	/* All the list items created on heap initiated by newProjectDialog will be deleted once m_npdlg is deleted.
	 * The parent list will not delete the items, the heap destruction will. So we defer its destruction when we
	 * are destructed */
	heap_del ( m_npdlg );
}

void estimateDlg::showWindow ( const QString& client_name )
{
	QTreeWidgetItem* item ( nullptr );
	for ( int i ( 0 ); i < treeView->topLevelItemCount (); ++i )
	{
		if ( treeView->topLevelItem ( i )->text ( 0 ) == client_name )
		{
			item = treeView->topLevelItem ( i );
			treeView->setCurrentItem ( item, 0 );
			treeView->expandItem ( item );
			break;
		}
	}
	showNormal ();
}

void estimateDlg::setupActions ()
{
	menuDoc = new QMenu;
	menuDoc->addAction ( new vmAction ( SFO_OPEN_FILE, TR_FUNC ( "Open document for editing" ), this ) );
	menuDoc->addAction ( new vmAction ( SFO_REMOVE_FILE, TR_FUNC ( "Remove document file" ), this ) );
	connect ( menuDoc, &QMenu::triggered, this, [&] ( QAction* action ) { return execMenuAction ( action ); } );
	btnDoc->setMenu ( menuDoc );

	menuVmr = new QMenu;
	menuVmr->addAction ( new vmAction ( SFO_OPEN_FILE, TR_FUNC ( "Open report for editing" ), this ) );
	menuVmr->addAction ( new vmAction ( SFO_REMOVE_FILE, TR_FUNC ( "Remove document file" ), this ) );
	connect ( menuVmr, &QMenu::triggered, this, [&] ( QAction* action ) { return execMenuAction ( action ); } );
	btnVmr->setMenu ( menuVmr );

	menuXls = new QMenu;
	menuXls->addAction ( new vmAction ( SFO_OPEN_FILE, TR_FUNC ( "Open spreadsheet for editing" ), this ) );
	menuXls->addAction ( new vmAction ( SFO_REMOVE_FILE, TR_FUNC ( "Remove spreadsheet file" ), this ) );
	connect ( menuXls, &QMenu::triggered, this, [&] ( QAction* action ) { return execMenuAction ( action ); } );
	btnXls->setMenu ( menuXls );

	menuPdf = new QMenu;
	menuPdf->addAction ( new vmAction ( SFO_OPEN_FILE, TR_FUNC ( "Open PDF for viewing" ), this ) );
	menuPdf->addAction ( new vmAction ( SFO_REMOVE_FILE, TR_FUNC ( "Remove PDF file" ), this ) );
	connect ( menuPdf, &QMenu::triggered, this, [&] ( QAction* action ) { return execMenuAction ( action ); } );
	btnPdf->setMenu ( menuPdf );

	menuFileMngr = new QMenu;
	menuFileMngr->addAction ( new vmAction ( OPEN_SELECTED_DIR, TR_FUNC ( "Open selected directory" ), this ) );
	menuFileMngr->addSeparator ();
	menuFileMngr->addAction ( new vmAction ( OPEN_ESTIMATES_DIR, TR_FUNC ( "Open client's estimates dir" ), this ) );
	menuFileMngr->addAction ( new vmAction ( OPEN_REPORTS_DIR, TR_FUNC ( "Open client's reports dir" ), this ) );
	menuFileMngr->addAction ( new vmAction ( OPEN_CLIENT_DIR, TR_FUNC ( "Open client's projects dir" ), this ) );
	connect ( menuFileMngr, &QMenu::triggered, this, [&] ( QAction* action ) { return execMenuAction ( action ); } );
	btnFileManager->setMenu ( menuFileMngr );

	menuProject = new QMenu;
	menuProject->addAction ( new vmAction ( PA_NEW_FULL, TR_FUNC ( "New project folder" ), this ) );
	menuProject->addAction ( new vmAction ( PA_NEW_EMPTY, TR_FUNC ( "New empty project folder" ), this ) );
	menuProject->addAction ( new vmAction ( PA_CONVERT_FROM_ESTIMATE, TR_FUNC ( "New from estimate" ), this ) );
	menuProject->addAction ( new vmAction ( PA_LINK_TO_JOB, TR_FUNC ( "Link this project folder to job" ), this ) );
	menuProject->addAction ( new vmAction ( PA_RENAME, TR_FUNC ( "Rename project" ), this ) );
	menuProject->addAction ( new vmAction ( PA_REMOVE, TR_FUNC ( "Delete project folder" ), this ) );
	menuProject->addAction ( new vmAction ( PA_ADD_DOC, TR_FUNC ( "Add new DOC" ), this ) );
	menuProject->addAction ( new vmAction ( PA_ADD_XLS, TR_FUNC ( "Add new XLS" ), this ) );
	connect ( menuProject, &QMenu::triggered, this, [&] ( QAction* action ) { return projectActions ( action ); } );
	btnProject->setMenu ( menuProject );

	menuEstimate = new QMenu;
	menuEstimate->addAction ( new vmAction ( EA_NEW_EXTERNAL, TR_FUNC ( "New estimage using external document editor" ), this ) );
	menuEstimate->addAction ( new vmAction ( EA_NEW_VMR, TR_FUNC ( "New estimate using internal document editor" ), this ) );
	menuEstimate->addAction ( new vmAction ( EA_CONVERT, TR_FUNC ( "Convert to project" ), this ) );
	menuEstimate->addAction ( new vmAction ( EA_REMOVE, TR_FUNC ( "Delete estimate" ), this ) );
	menuEstimate->addAction ( new vmAction ( EA_ADD_DOC, TR_FUNC ( "Add new DOC" ), this ) );
	menuEstimate->addAction ( new vmAction ( EA_ADD_XLS, TR_FUNC ( "Add new XLS" ), this ) );
	connect ( menuEstimate, &QMenu::triggered, this, [&] ( QAction* action ) { return estimateActions ( action ); } );
	btnEstimate->setMenu ( menuEstimate );

	menuReport = new QMenu;
	menuReport->addAction ( new vmAction ( RA_NEW_EXTERNAL, TR_FUNC ( "New report using external document editor" ), this ) );
	menuReport->addAction ( new vmAction ( RA_NEW_EXTERNAL_COMPLETE, TR_FUNC ( "New complete report, using external spreadsheet and document editor" ), this ) );
	menuReport->addAction ( new vmAction ( RA_NEW_VMR, TR_FUNC ( "New report using internal document editor" ), this ) );
	menuReport->addAction ( new vmAction ( RA_REMOVE, TR_FUNC ( "Delete this report" ), this ) );
	connect ( menuReport, &QMenu::triggered, this, [&] ( QAction* action ) { return reportActions ( action ); } );
	btnReport->setMenu ( menuReport );
}

void estimateDlg::scanDir ()
{
	pointersList<fileOps::st_fileInfo*> filesFound;

	const QStringList nameFilter ( QStringList () <<
								   configOps::projectDocumentFormerExtension () <<
								   configOps::projectSpreadSheetFormerExtension () <<
								   configOps::projectPDFExtension () <<
								   configOps::projectReportExtension () );

	const QStringList exclude_dirs ( QStringList () <<
									 QStringLiteral ( u"Pictures" ) <<
									 QStringLiteral ( u"Serviços simples" ) );
	
	treeView->clear ();
	QString clientName;
	QString dirName;
	const uint lastClientID ( VDB ()->getHighestID ( TABLE_CLIENT_ORDER, VDB () ) );
	
	for ( uint c_id ( 1 ); c_id <= lastClientID; ++c_id )
	{
		clientName = Client::clientName ( c_id );
		if ( !clientName.isEmpty () )
		{
			dirName = CONFIG ()->getProjectBasePath ( clientName );
			fileOps::lsDir ( filesFound, dirName, nameFilter, exclude_dirs, fileOps::LS_ALL, 2, false );
			addToTree ( filesFound, clientName );
		}
	}
	// Scan estimates done for non-clients
	clientName = nonClientStr;
	dirName = CONFIG ()->projectsBaseDir () + nonClientStr + CHR_F_SLASH;
	if ( fileOps::exists ( dirName ).isOn () )
	{
		fileOps::lsDir ( filesFound, dirName, nameFilter, exclude_dirs, fileOps::LS_ALL, 1, false );
		addToTree ( filesFound, clientName, TYPE_ESTIMATE_ITEM );
	}
	if ( treeView->topLevelItemCount () > 0 )
		treeView->setCurrentItem ( treeView->topLevelItem ( 0 ) );
}

void estimateDlg::addToTree ( pointersList<fileOps::st_fileInfo*>& files, const QString& clientName, const int newItemType )
{
	QTreeWidgetItem* topLevelItem ( nullptr ), *child_dir ( nullptr ), *child_file ( nullptr );
	for ( int i ( 0 ); i < treeView->topLevelItemCount (); ++i )
	{
		if ( treeView->topLevelItem ( i )->text ( 0 ) == clientName )
		{
			topLevelItem = treeView->topLevelItem ( i );
			break;
		}
	}
	if ( topLevelItem == nullptr )
	{
		topLevelItem = new QTreeWidgetItem;
		topLevelItem->setText ( 0, clientName );
		topLevelItem->setBackground ( 0, QBrush ( Qt::darkCyan ) );
		topLevelItem->setForeground ( 0, QBrush ( Qt::white ) );
		topLevelItem->setData ( 0, ROLE_ITEM_TYPE, TYPE_CLIENT_TOPLEVEL );
		topLevelItem->setData ( 0, ROLE_ITEM_FILENAME, CONFIG ()->getProjectBasePath ( clientName ) );
		topLevelItem->setData ( 0, ROLE_ITEM_CLIENTNAME, clientName );
		treeView->addTopLevelItem ( topLevelItem );
	}

	if ( !files.isEmpty () )
	{
		uint i ( 0 ), itemType ( newItemType == -1 ? TYPE_REPORT_ITEM : static_cast<uint>(newItemType) );
		uint dirType ( 0 );
		QString filename;

		do
		{
			filename = files.at ( i )->filename;
			if ( files.at ( i )->is_dir )
			{
				child_dir = new QTreeWidgetItem ( topLevelItem );
				child_dir->setText ( 0, filename );
				if ( filename == CONFIG ()->estimatesDirSuffix () )
					dirType = TYPE_ESTIMATE_DIR;
				else if ( filename == CONFIG ()->reportsDirSuffix () )
					dirType = TYPE_REPORT_DIR;
				else
				{
					dirType = TYPE_PROJECT_DIR;
					child_dir->setText ( 2, getDateFromProjectDir ( filename ) );
				}
				if ( newItemType == -1 )
					itemType = dirType - 3;
				child_dir->setText ( 1, itemTypeStr[dirType] );
				child_dir->setData ( 0, ROLE_ITEM_TYPE, dirType );
				child_dir->setData ( 0, ROLE_ITEM_FILENAME, files.at ( i )->fullpath );
				child_dir->setData ( 0, ROLE_ITEM_CLIENTNAME, clientName );
			}
			else
			{
				if ( child_dir == nullptr ) // not used in the first pass, only when updating
				{
					QString child_dir_name;
					if ( itemType == TYPE_PROJECT_ITEM )
						child_dir_name = fileOps::nthDirFromPath ( filename );
					if ( itemType == TYPE_ESTIMATE_ITEM )
						child_dir_name = CONFIG ()->estimatesDirSuffix ();
					else // itemType == TYPE_REPORT_ITEM
						child_dir_name = CONFIG ()->reportsDirSuffix ();
					
					for ( int i ( 0 ); i < topLevelItem->childCount (); ++i )
					{
						if ( topLevelItem->child ( i )->text ( 0 ) == child_dir_name )
						{
							child_dir = topLevelItem->child ( i );
							break;
						}
					}
					if ( child_dir == nullptr )
						continue;
				}
				child_file = new QTreeWidgetItem ( child_dir );
				switch ( itemType )
				{
					case TYPE_PROJECT_ITEM:
						child_file->setText ( 0, getNameFromProjectFile ( filename ) + QStringLiteral ( " (" ) + fileOps::fileExtension ( filename ) + CHR_R_PARENTHESIS );
						child_file->setText ( 2, getDateFromProjectFile ( filename ) );
					break;
					case TYPE_ESTIMATE_ITEM:
						child_file->setText ( 0, getNameFromEstimateFile ( filename ) + QStringLiteral ( " (" ) + fileOps::fileExtension ( filename ) + CHR_R_PARENTHESIS );
						child_file->setText ( 2, getDateFromEstimateFile ( filename ) );
					break;
					case TYPE_REPORT_ITEM:
						child_file->setText ( 0, getNameFromReportFile ( filename ) + QStringLiteral ( " (" ) + fileOps::fileExtension ( filename ) + CHR_R_PARENTHESIS );
						child_file->setText ( 2, getDateFromReportFile ( CONFIG ()->reportsDir ( clientName ) + filename ) );
					break;
				}
				child_file->setText ( 1, itemTypeStr[itemType] );
				child_file->setIcon ( 0, iconsType[actionIndex ( filename )] );
				child_file->setData ( 0, ROLE_ITEM_TYPE, itemType );
				child_file->setData ( 0, ROLE_ITEM_FILENAME, files.at ( i )->fullpath );
				child_file->setData ( 0, ROLE_ITEM_CLIENTNAME, clientName != nonClientStr ? clientName : emptyString );
			}
		} while ( ++i < files.count () );
		files.clearButKeepMemory ( true );
	}
}

uint estimateDlg::actionIndex ( const QString& filename ) const
{
	if ( filename.contains ( CONFIG ()->projectDocumentFormerExtension () ) )
		return FILETYPE_DOC;
	if ( filename.contains ( CONFIG ()->projectSpreadSheetFormerExtension () ) )
		return FILETYPE_XLS;
	if ( filename.contains ( CONFIG ()->projectPDFExtension () ) )
		return FILETYPE_PDF;
	if ( filename.contains ( CONFIG ()->projectReportExtension () ) )
		return FILETYPE_VMR;
	return FILETYPE_UNKNOWN;
}

void estimateDlg::updateButtons ()
{
	if ( !b_inItemSelectionMode && !treeView->selectedItems ().isEmpty () )
	{
		const QTreeWidgetItem* sel_item ( treeView->selectedItems ().at ( 0 ) );
		const uint type ( sel_item->data ( 0, ROLE_ITEM_TYPE ).toUInt () );
		const uint fileTypeIdx ( actionIndex ( sel_item->data ( 0, ROLE_ITEM_FILENAME ).toString () ) );
		btnDoc->setEnabled ( fileTypeIdx == FILETYPE_DOC );
		btnXls->setEnabled ( fileTypeIdx == FILETYPE_XLS );
		btnPdf->setEnabled ( fileTypeIdx == FILETYPE_PDF );
		btnVmr->setEnabled ( fileTypeIdx == FILETYPE_VMR );
		btnProject->setEnabled ( type == TYPE_PROJECT_ITEM || type == TYPE_CLIENT_TOPLEVEL || type == TYPE_PROJECT_DIR );
		if ( btnProject->isEnabled () )
		{
			menuProject->actions ().at ( PA_NEW_FULL )->setEnabled ( type == TYPE_CLIENT_TOPLEVEL );
			menuProject->actions ().at ( PA_NEW_EMPTY )->setEnabled ( type == TYPE_CLIENT_TOPLEVEL );
			menuProject->actions ().at ( PA_CONVERT_FROM_ESTIMATE )->setEnabled ( type == TYPE_CLIENT_TOPLEVEL );
			menuProject->actions ().at ( PA_LINK_TO_JOB )->setEnabled ( type == TYPE_PROJECT_DIR );
			menuProject->actions ().at ( PA_RENAME )->setEnabled ( type == TYPE_PROJECT_DIR );
			menuProject->actions ().at ( PA_REMOVE )->setEnabled ( type == TYPE_PROJECT_DIR );
			menuProject->actions ().at ( PA_ADD_DOC )->setEnabled ( type == TYPE_PROJECT_DIR );
			menuProject->actions ().at ( PA_ADD_XLS )->setEnabled ( type == TYPE_PROJECT_DIR );
		}
		btnReport->setEnabled ( type == TYPE_REPORT_ITEM || type == TYPE_CLIENT_TOPLEVEL || type == TYPE_REPORT_DIR );
		if ( btnReport->isEnabled () )
		{
			menuReport->actions ().at ( RA_NEW_EXTERNAL )->setEnabled ( type != TYPE_REPORT_ITEM );
			menuReport->actions ().at ( RA_NEW_EXTERNAL_COMPLETE )->setEnabled ( type != TYPE_REPORT_ITEM );
			menuReport->actions ().at ( RA_NEW_VMR )->setEnabled ( type != TYPE_REPORT_ITEM );
			menuReport->actions ().at ( RA_REMOVE )->setEnabled ( type == TYPE_REPORT_ITEM );
		}
		btnEstimate->setEnabled ( type == TYPE_ESTIMATE_ITEM || type == TYPE_CLIENT_TOPLEVEL || type == TYPE_ESTIMATE_DIR );
		if ( btnEstimate->isEnabled () )
		{
			menuEstimate->actions ().at ( EA_NEW_EXTERNAL )->setEnabled ( type != TYPE_ESTIMATE_ITEM );
			menuEstimate->actions ().at ( EA_NEW_VMR )->setEnabled ( type != TYPE_ESTIMATE_ITEM );
			menuEstimate->actions ().at ( EA_CONVERT )->setEnabled ( type == TYPE_ESTIMATE_ITEM );
			menuEstimate->actions ().at ( EA_REMOVE )->setEnabled ( type == TYPE_ESTIMATE_ITEM );
			menuEstimate->actions ().at ( EA_ADD_DOC )->setEnabled ( type == TYPE_ESTIMATE_DIR );
			menuEstimate->actions ().at ( EA_ADD_XLS )->setEnabled ( type == TYPE_ESTIMATE_DIR );
		}
		btnFileManager->menu ()->actions ().at ( 0 )->setEnabled ( true );
	}
	else
	{
		btnDoc->setEnabled ( false );
		btnXls->setEnabled ( false );
		btnPdf->setEnabled ( false );
		btnVmr->setEnabled ( false );
		btnProject->setEnabled ( false );
		btnReport->setEnabled (	false );
		btnEstimate->setEnabled ( false );
		btnFileManager->setEnabled ( false );
	}
}

void estimateDlg::openWithDoubleClick ( QTreeWidgetItem* item )
{
	const uint type ( item->data ( 0, ROLE_ITEM_TYPE ).toUInt () );
	if ( type <= TYPE_REPORT_ITEM )
		execAction ( item, SFO_OPEN_FILE );
}

void estimateDlg::cancelItemSelection ()
{
	b_inItemSelectionMode = false;
	for ( int i ( 0 ); i < treeView->topLevelItemCount (); i++ )
		treeView->topLevelItem ( i )->setDisabled ( false );
	treeView->setCurrentItem ( targetItemInSelectionMode, 0, QItemSelectionModel::ClearAndSelect );
	treeView->scrollToItem ( targetItemInSelectionMode, QAbstractItemView::PositionAtTop );
	treeView->expandItem ( targetItemInSelectionMode );
	treeView->expandItem ( targetItemInSelectionMode->child ( 0 ) ? targetItemInSelectionMode->child ( 0 ) : nullptr );
	btnClose->setText ( TR_FUNC ( "Close" ) );
	disconnect ( treeView, &QTreeWidget::itemSelectionChanged, nullptr, nullptr );
	connect ( treeView, &QTreeWidget::itemSelectionChanged, this, [&] () { updateButtons (); } );
	updateButtons ();
}

void estimateDlg::selectEstimateFromFile ( QTreeWidgetItem* targetClient )
{
	NOTIFY ()->notifyMessage ( TR_FUNC ( "Select estimate file" ), TR_FUNC ( "Select one estimate file to be converted into "
																			"a new project. Or press cancel to abort this operation." ), 5000 );
	for ( int i ( 0 ); i < treeView->topLevelItemCount (); i++ )
	{
		if ( treeView->topLevelItem ( i )->text ( 0 ) != nonClientStr )
			treeView->topLevelItem ( i )->setDisabled ( true );
		else {
			treeView->setCurrentItem ( treeView->topLevelItem ( i ), 0, QItemSelectionModel::ClearAndSelect );
			treeView->scrollToItem ( treeView->topLevelItem ( i ), QAbstractItemView::PositionAtTop );
			treeView->expandItem ( treeView->topLevelItem ( i ) );
			treeView->expandItem ( treeView->topLevelItem ( i )->child ( 0 ) );
		}
	}
	targetClient->setDisabled ( false );
	targetItemInSelectionMode = targetClient;
	b_inItemSelectionMode = true;
	updateButtons ();
	btnClose->setText ( TR_FUNC ( "Cancel" ) );	
	disconnect ( treeView, &QTreeWidget::itemSelectionChanged, nullptr, nullptr );
	connect ( treeView, &QTreeWidget::itemSelectionChanged, this, [&] () { convertToProject ( treeView->currentItem () ) ; } );
}

void estimateDlg::convertToProject ( QTreeWidgetItem* item )
{	
	NOTIFY ()->notifyMessage ( TR_FUNC ( "Choose destination job" ),
							   TR_FUNC ( "Please select the client and the job to which to convert this estimate." ), 5000 );
	if ( !m_npdlg )
		m_npdlg = new newProjectDialog ( this );
	m_npdlg->showDialog ( emptyString, true );
	if ( !m_npdlg->resultAccepted () )
		return;

	QString project_name;	
	bool bProceed ( false );
	fileOps::st_fileInfo* f_info ( nullptr );
	pointersList<fileOps::st_fileInfo*> files;
	files.setAutoDeleteItem ( true );
	jobListItem* jobItem ( nullptr );
	
	const QString targetPath ( m_npdlg->projectPath () );
	if ( fileOps::exists ( targetPath ).isOn () )
	{
		f_info = new fileOps::st_fileInfo;
		bProceed = fileOps::createDir ( targetPath ).isOn ();
		if ( bProceed )
		{
			project_name = fileOps::nthDirFromPath ( targetPath );
			f_info->filename = project_name;
			if ( f_info->filename.endsWith ( CHR_F_SLASH ) )
				f_info->filename.chop ( 1 );
			f_info->fullpath = targetPath;
			f_info->is_dir = true;
			f_info->is_file = false;
			files.append ( f_info );
			(void) fileOps::createDir ( targetPath + QStringLiteral ( "Pictures/" ) );
			jobItem = m_npdlg->jobItem ();
		}
	}
	else
		return;

	const QString srcPath ( fileOps::dirFromPath ( item->data ( 0, ROLE_ITEM_FILENAME ).toString () ) );
	
	if ( getDateFromProjectDir ( project_name ).isEmpty () )
	{
		vmNumber date ( getDateFromEstimateFile ( fileOps::nthDirFromPath ( item->data ( 0, ROLE_ITEM_FILENAME ).toString () ) ), VMNT_DATE );
		project_name.prepend ( date.toDate ( vmNumber::VDF_FILE_DATE )  + CHR_SPACE + CHR_HYPHEN + CHR_SPACE );
	}
	
	//const QString baseFilename ( fileOps::fileNameWithoutPath ( fileOps::filePathWithoutExtension ( item->data ( 0, ROLE_ITEM_FILENAME ).toString () ) ) );
	fileOps::lsDir ( files, srcPath, QStringList () << item->text ( 0 ).left ( item->text ( 0 ).indexOf ( CHR_L_PARENTHESIS ) - 1 ) );
	for ( uint i ( 1 ); i < files.count (); ++i )
	{
		auto final_name = [] ( const QString& source ) -> QString { return ( source.endsWith ( CONFIG ()->projectSpreadSheetExtension () ) ? QStringLiteral ( "Planilhas-" ) : QStringLiteral ( "Projeto-" ) ) + source; };
		static_cast<void> ( fileOps::copyFile ( targetPath + final_name ( files.at ( i )->filename ), files.at ( i )->fullpath ) );
	}

	changeJobData ( jobItem, targetPath, m_npdlg->projectID () );
	
	addToTree ( files, Client::clientName ( static_cast<uint>( recIntValue ( jobItem->jobRecord (), FLD_JOB_CLIENTID ) ) ) );
	NOTIFY ()->notifyMessage ( TR_FUNC ( "Success!" ), project_name + TR_FUNC ( " was converted!" ) );
	if ( b_inItemSelectionMode )
	{
		cancelItemSelection ();
	}
}

void estimateDlg::projectActions ( QAction *action )
{
	QTreeWidgetItem* item ( treeView->selectedItems ().at ( 0 ) );
	bool bAddDoc ( false ), bAddXls ( false ), bUseDialog ( false ), bProceed ( false ), bGetJobOnly ( false );
	QString strProjectPath, strProjectID, msgTitle, msgBody[2];
	switch ( static_cast<vmAction*>( action )->id () )
	{
		case PA_NEW_FULL:
			bAddDoc = bAddXls = true;
			bUseDialog = true;
			msgTitle = TR_FUNC ( "New Full Project Creation - " );
			msgBody[0] = TR_FUNC ( "Project %1 could not be created! Check the the error logs." );
			msgBody[1] = TR_FUNC ( "Project %1 was created!" );
		break;
		case PA_NEW_EMPTY:
			bUseDialog = true;
			msgTitle = TR_FUNC ( "New Empty Project Creation - " );
			msgBody[0] = TR_FUNC ( "Project %1 could not be created! Check the the error logs." );
			msgBody[1] = TR_FUNC ( "Project %1 was created!" );
		break;
		case PA_CONVERT_FROM_ESTIMATE:
			selectEstimateFromFile ( item );
		return;
		case PA_LINK_TO_JOB:
			bUseDialog = true;
			bGetJobOnly = true;
			msgTitle = TR_FUNC ( "New Full Project Creation - " );
			msgBody[0] = TR_FUNC ( "Project " ) + item->text ( 0 ) + TR_FUNC ( " was not linked to any job. Check the error logs." );
			msgBody[1] = TR_FUNC ( "Project " ) + item->text ( 0 ) + TR_FUNC ( " was linked to job %1" );
		break;
		case PA_RENAME:
			bProceed = renameDir ( item, strProjectPath );
			msgTitle = TR_FUNC ( "Project Renaming - ");
			msgBody[0] = tr ( "Project " ) + item->text ( 0 ) + tr ( " could not be renamed! Check the the error logs." );
			msgBody[1] = tr ( "Project " ) + item->text ( 0 ) + tr ( " was renamed to " ) + strProjectPath;
		break;
		case PA_REMOVE:
			msgTitle = TR_FUNC ( "Project Removal - ");
			msgBody[0] = tr ( "Project " ) + item->text ( 0 ) + tr ( " could not be removed! Check the the error logs." );
			msgBody[1] = tr ( "Project " ) + item->text ( 0 ) + tr ( " was removed." );
			bProceed = removeDir ( item, true );
		break;
		case PA_ADD_DOC:
			bAddDoc = true;
			msgTitle = TR_FUNC ( "Adding document file - ");
			msgBody[0] = tr ( "Could not add document file to project " ) + item->text( 0 ) + tr ( " Check the the error logs." );
			msgBody[1] = tr ( "A new document file was added to project " ) + item->text ( 0 );
		break;
		case PA_ADD_XLS:
			bAddXls = true;
			msgTitle = TR_FUNC ( "Adding spreadshett file - ");
			msgBody[0] = tr ( "Could not add spreadsheet file to project " ) + item->text( 0 ) + tr ( " Check the the error logs." );
			msgBody[1] = tr ( "A new spreadsheet file was added to project " ) + item->text ( 0 );
		break;
	}
	pointersList<fileOps::st_fileInfo*> files;
	files.setAutoDeleteItem ( true );
	jobListItem* jobItem ( nullptr );

	if ( bUseDialog )
	{
		if ( !m_npdlg )
			m_npdlg = new newProjectDialog ( this );
		m_npdlg->showDialog ( item->data ( 0, ROLE_ITEM_CLIENTNAME ).toString (), !bGetJobOnly, !bGetJobOnly );
		bProceed = m_npdlg->resultAccepted ();
		strProjectID = m_npdlg->projectID ();
		if ( bProceed && !bGetJobOnly )
		{
			strProjectPath = m_npdlg->projectPath ();
			fileOps::st_fileInfo* f_info ( nullptr );
			if ( ( bProceed = !fileOps::exists ( strProjectPath ).isUndefined () ) )
			{
				f_info = new fileOps::st_fileInfo;
				bProceed = fileOps::createDir ( strProjectPath ).isOn ();
				if ( bProceed )
				{
					f_info->filename = fileOps::nthDirFromPath ( strProjectPath );
					if ( f_info->filename.endsWith ( CHR_F_SLASH ) )
						f_info->filename.chop ( 1 );
					f_info->fullpath = strProjectPath;
					f_info->is_dir = true;
					f_info->is_file = false;
					files.append ( f_info );
					static_cast<void>( fileOps::createDir ( strProjectPath + QStringLiteral ( "Pictures" ) ) );
					addFilesToDir ( bAddDoc, bAddXls, strProjectPath, strProjectID, files );
					bAddDoc = bAddXls = false;
				}
				else
					delete f_info;
			}
		}
		else if ( bGetJobOnly )
		{
			strProjectPath = item->data ( 0, ROLE_ITEM_FILENAME ).toString ();
		}
		jobItem = m_npdlg->jobItem ();
	}
	if ( bProceed )
	{
		if ( jobItem == nullptr ) // Only when renaming, removing, add_doc or add_xls to existing directory
			jobItem = findJobByPath ( item );
		if ( jobItem != nullptr )
		{
			if ( bAddDoc || bAddXls )
			{ // Last case to handle. There is no change to the database
				strProjectPath = recStrValue ( jobItem->jobRecord (), FLD_JOB_PROJECT_PATH );
				strProjectID = recStrValue ( jobItem->jobRecord (), FLD_JOB_PROJECT_ID );
				addFilesToDir ( bAddDoc, bAddXls, strProjectPath, strProjectID, files );
			}
			else
				changeJobData ( jobItem, strProjectPath, strProjectID );
			if ( !bGetJobOnly )
				addToTree ( files, item->text ( 0 ) );
		}
	}
	msgTitle += bProceed ? TR_FUNC ( "Succeeded!" ) : TR_FUNC ( "Failed!" );
	if ( msgBody[0].contains ( CHR_PERCENT ) )
	{
		if ( bProceed )
			msgBody[1] = msgBody[1].arg ( strProjectPath );
		else
			msgBody[0] = msgBody[0].arg ( strProjectPath );
	}
	NOTIFY ()->notifyMessage ( msgTitle, msgBody[static_cast<uint>( bProceed )] );
}

jobListItem* estimateDlg::findJobByPath ( QTreeWidgetItem* const item )
{
	jobListItem* jobItem ( nullptr );
#ifdef USING_QT6
    const QString clientID ( QString::number ( Client::clientID ( item->text ( 0 ) ) ) );
#else
    const QString clientID ( Client::clientID ( item->text ( 0 ) ) );
#endif
	clientListItem* clientItem ( MAINWINDOW ()->getClientItem ( clientID.toUInt () ) );
	if ( clientItem )
	{
		const QString strPath ( item->data ( 0, ROLE_ITEM_FILENAME ).toString () );
		const QString strQuery ( QStringLiteral ( "SELECT ID FROM JOBS WHERE CLIENTID=%1 AND PROJECTPATH=%2" ) );
		QSqlQuery queryRes;
		if ( VDB ()->runSelectLikeQuery ( strQuery.arg ( clientID, strPath ), queryRes ) )
			jobItem = MAINWINDOW ()->getJobItem ( clientItem, queryRes.value ( 0 ).toUInt () );
	}
	return jobItem;
}

void estimateDlg::changeJobData ( jobListItem* const jobItem, const QString& strProjectPath, const QString& strProjectID )
{
	if ( jobItem != nullptr ) 
	{	
		jobItem->loadData ( true );
		jobItem->setAction ( ACTION_EDIT, true );
		Job* job ( jobItem->jobRecord () );
		setRecValue ( job, FLD_JOB_PROJECT_PATH, strProjectPath );
		setRecValue ( job, FLD_JOB_PICTURE_PATH, strProjectPath + QStringLiteral ( "Pictures/" ) );
		setRecValue ( job, FLD_JOB_PROJECT_ID, strProjectID );
		job->saveRecord ();
		jobItem->setAction ( job->action () );
		MAINWINDOW ()->jobExternalChange ( jobItem->dbRecID (), job->prevAction () );
	}
}

void estimateDlg::estimateActions ( QAction* action )
{
	QTreeWidgetItem* item ( treeView->selectedItems ().at ( 0 ) );
	if ( !item )
		return;
	const QString clientName ( item->parent () ? item->parent ()->text ( 0 ) : item->text ( 0 ) );
	const QString basePath ( clientName != nonClientStr ?
							 CONFIG ()->estimatesDir ( clientName ) :
							 CONFIG ()->projectsBaseDir () + configOps::estimatesDirSuffix () + CHR_F_SLASH );

	switch ( static_cast<vmAction*> ( action )->id () )
	{
		case EA_CONVERT:
			convertToProject ( item );
		break;
		case EA_REMOVE:
			removeFiles ( item, false, true );
		break;
		default:
		{
			QString estimateName;
			vmNotify::inputBox ( estimateName, this, TR_FUNC ( "Please write the estimate's name" ),
								 TR_FUNC ( "Name: " ) );
			if ( estimateName.isEmpty () )
				return;

			pointersList<fileOps::st_fileInfo*> files;
			fileOps::st_fileInfo* f_info ( nullptr );
			
			if ( fileOps::exists ( basePath ).isOff () )
			{
				if ( !fileOps::createDir ( basePath ).isOn () ) //maybe first estimate for client, create dir
					return;
				f_info = new fileOps::st_fileInfo;
				f_info->filename = CONFIG ()->estimatesDirSuffix ();
				f_info->fullpath = basePath;
				f_info->is_dir = true;
				f_info->is_file = false;
				files.append ( f_info );
			}
			
			estimateName.prepend ( vmNumber::currentDate ().toDate ( vmNumber::VDF_FILE_DATE ) + QStringLiteral ( " - " ) );
			if ( clientName == nonClientStr )
				estimateName.prepend ( nonClientEstimatesPrefix );

			f_info = new fileOps::st_fileInfo;
			if ( static_cast<vmAction*> ( action )->id () == EA_NEW_VMR )
			{
				f_info->filename = estimateName + CONFIG ()->projectReportExtension ();
				f_info->fullpath = basePath + f_info->filename;

				EDITOR ()->startNewReport ( true )->createJobReport ( Client::clientID ( clientName ),
					false, f_info->fullpath, false );
				files.append ( f_info );
			}
			else
			{
				f_info->filename = estimateName + CONFIG ()->projectDocumentExtension ();
				f_info->fullpath = basePath + f_info->filename;

				if ( fileOps::copyFile ( f_info->fullpath, CONFIG ()->projectDocumentFile () ) )
					files.append ( f_info );

				f_info = new fileOps::st_fileInfo;
				f_info->filename = estimateName + CONFIG ()->projectSpreadSheetExtension ();
				f_info->fullpath = basePath + f_info->filename;
				if ( fileOps::copyFile ( f_info->fullpath, CONFIG ()->projectSpreadSheetFile () ) )
					files.append ( f_info );
			}
			addToTree ( files, clientName, TYPE_ESTIMATE_ITEM );
		}
		break;
	}
}

void estimateDlg::reportActions ( QAction* action )
{
	QTreeWidgetItem* item ( treeView->selectedItems ().at ( 0 ) );
	const QString clientName ( item->parent () ? item->parent ()->text ( 0 ) : item->text ( 0 ) );

	const QString basePath ( clientName != nonClientStr ?
							 CONFIG ()->reportsDir ( clientName ) :
							 CONFIG ()->projectsBaseDir () + configOps::reportsDirSuffix () + CHR_F_SLASH
						   );
	if ( static_cast<vmAction*> ( action )->id () == RA_REMOVE )
			removeFiles ( item, true, true );
	else
	{
		QString reportName;
		vmNotify::inputBox ( reportName, nullptr, TR_FUNC ( "Please write the report's name" ),
						 TR_FUNC ( "Name: " ) );
		if ( reportName.isEmpty () )
			return;

		pointersList<fileOps::st_fileInfo*> files;
		fileOps::st_fileInfo* f_info ( nullptr );
		
		if ( fileOps::exists ( basePath ).isOff () )
		{
			if ( !fileOps::createDir ( basePath ).isOn () ) //maybe first report for user, create dir
				return;
			f_info = new fileOps::st_fileInfo;
			f_info->filename = CONFIG ()->reportsDirSuffix ();
			f_info->fullpath = basePath;
			f_info->is_dir = true;
			f_info->is_file = false;
			files.append ( f_info );
		}

		reportName.prepend ( vmNumber::currentDate ().toDate ( vmNumber::VDF_FILE_DATE ) + QStringLiteral ( " - " ) );
		if ( clientName == nonClientStr )
			reportName.prepend ( nonClientReportsPrefix );

		f_info = new fileOps::st_fileInfo;
		if ( static_cast<vmAction*> ( action )->id () == RA_NEW_VMR )
		{
			f_info->filename = reportName + CONFIG ()->projectReportExtension ();
			f_info->fullpath = basePath + f_info->filename;
			EDITOR ()->startNewReport ( true )->createJobReport (
				Client::clientID ( clientName ), false , f_info->fullpath, false );
			files.append ( f_info );
		}
		else
		{
			f_info->filename = reportName + CONFIG ()->projectDocumentExtension ();
			f_info->fullpath = basePath + f_info->filename;
			if ( fileOps::copyFile ( f_info->fullpath, CONFIG ()->projectDocumentFile () ) )
				files.append ( f_info );

			if ( static_cast<vmAction*> ( action )->id () == RA_NEW_EXTERNAL_COMPLETE )
			{
				f_info = new fileOps::st_fileInfo;
				f_info->filename = reportName + CONFIG ()->projectSpreadSheetExtension ();
				f_info->fullpath = basePath + f_info->filename;
				if ( fileOps::copyFile ( f_info->fullpath, CONFIG ()->projectSpreadSheetFile () ) )
					files.append ( f_info );
			}
		}
		addToTree ( files, clientName, TYPE_REPORT_ITEM );
	}
}

void estimateDlg::removeFiles ( QTreeWidgetItem* item, const bool bSingleFile, const bool bAsk )
{
	if ( bAsk && !NOTIFY ()->questionBox ( TR_FUNC ( "Atention!" ), TR_FUNC ( "Are you sure you want to remove %1?" ).arg ( item->text ( 0 ) ) ) )
		return;

	const QString fileName ( item->data ( 0, ROLE_ITEM_FILENAME ).toString () );
	if ( bSingleFile )
	{
		if ( fileOps::removeFile ( fileName ).isOn () )
		{
			if ( item->parent () )
				item->parent ()->removeChild ( item );
			delete item;
		}
	}
	else
	{
		const QString baseFileName ( fileName.left ( fileName.lastIndexOf ( CHR_DOT ) + 1 ) );
		QTreeWidgetItem* parentItem ( item->parent () );
		QTreeWidgetItem* child ( nullptr );
		int i ( 0 );
		if ( parentItem->childCount () > 0 )
		{
			do
			{
				child = parentItem->child ( i );
				if ( child )
				{
					if ( child->text ( 0 ).startsWith ( baseFileName, Qt::CaseInsensitive ) )
					{
						if ( fileOps::removeFile ( child->text ( 0 ) ).isOn () )
						{
							parentItem->removeChild ( child );
							delete child;
						}
					}
					++i;
				}
			} while ( child != nullptr );
		}
	}
}

bool estimateDlg::removeDir ( QTreeWidgetItem *item, const bool bAsk )
{
	if ( bAsk )
	{
		if ( NOTIFY ()->questionBox ( TR_FUNC ( "Question" ), TR_FUNC ( "Are you sure you want to delete project " )
					+ item->text ( 0 ) + TR_FUNC ( " and all its contents?" ) ) )
		{

			if ( fileOps::rmDir ( item->data ( 0, ROLE_ITEM_FILENAME ).toString (), QStringList (), fileOps::LS_ALL, -1 ) )
			{
				NOTIFY ()->notifyMessage ( TR_FUNC ( "Success" ), TR_FUNC ( "Project " ) + item->text ( 0 ) + tr (" is now removed" ) );
				item->parent ()->removeChild ( item );
				delete item;
				return true;
			}
		}
	}
	return false;
}

bool estimateDlg::renameDir ( QTreeWidgetItem *item, QString &strNewName )
{
	if ( strNewName.isEmpty () )
	{
		if ( !vmNotify::inputBox ( strNewName, this, TR_FUNC ( "Rename Project" ), TR_FUNC ( "Enter new name for the project" ) ) )
			return false;
	}
	QString strNewFilePath ( item->data ( 0, ROLE_ITEM_FILENAME ).toString () );
	if ( fileOps::rename ( item->data ( 0, ROLE_ITEM_FILENAME ).toString (),
						   strNewFilePath.replace ( item->text ( 0 ), strNewName ) ).isOn () )
	{
		item->setText ( 0, strNewName );
		item->setData ( 0, ROLE_ITEM_FILENAME, strNewFilePath );
		return true;
	}
	return false;
}

void estimateDlg::addFilesToDir ( const bool bAddDoc, const bool bAddXls, const QString& projectpath, QString& projectid,
								  pointersList<fileOps::st_fileInfo*>& files )
{
	QString fileName, fileNameComplete;
	QString sub_version ( QLatin1Char ( 'A' ) );
	uint i ( 1 );
	bool b_ok ( false );
	if ( bAddDoc )
	{
		fileName = QStringLiteral ( "Projeto-" ) + projectid + QStringLiteral ( "-%1" ) + CONFIG ()->projectDocumentExtension ();
		do
		{
			fileNameComplete = projectpath + fileName.arg ( sub_version );
			if ( fileOps::exists ( fileNameComplete ).isOn () )
				sub_version = QChar ( 'A' + i++ );
			else
				break;
		} while ( true );
		if ( fileOps::copyFile ( fileNameComplete, CONFIG ()->projectDocumentFile () ) )
		{
			auto f_info ( new fileOps::st_fileInfo );
			f_info->filename = fileName.arg ( sub_version );
			f_info->fullpath = fileNameComplete;
			files.append ( f_info );
			b_ok = true;
		}
	}
	if ( bAddXls )
	{
		fileName = QStringLiteral ( "Planilhas-" ) + projectid + QStringLiteral ( "-%1") + CONFIG ()->projectSpreadSheetExtension ();
		do
		{
			fileNameComplete = projectpath + fileName.arg ( sub_version );
			if ( fileOps::exists ( fileNameComplete ).isOn () )
				sub_version = QChar ( 'A' + i++ );
			else
				break;
		} while ( true );
		if ( fileOps::copyFile ( fileNameComplete, CONFIG ()->projectSpreadSheetFile () ) )
		{
			auto f_info ( new fileOps::st_fileInfo );
			f_info->filename = fileName.arg ( sub_version );
			f_info->fullpath = fileNameComplete;
			files.append ( f_info );
			b_ok = true;
		}
	}
	if ( b_ok )
		projectid += CHR_HYPHEN + sub_version;
}

void estimateDlg::execAction ( const QTreeWidgetItem* item, const int action_id )
{
	const QString filename ( item->data ( 0, ROLE_ITEM_FILENAME ).toString () );
	const uint fileTypeIdx ( actionIndex ( filename ) );
	const QString clientName ( item->data ( 0, ROLE_ITEM_CLIENTNAME ).toString () );
	QString program;
	QStringList args;

	switch ( action_id )
	{
		case SFO_OPEN_FILE:
		{
			switch ( fileTypeIdx )
			{
				case FILETYPE_DOC:		program = CONFIG ()->docEditor (); break;
				case FILETYPE_XLS:		program = CONFIG ()->xlsEditor (); break;
				case FILETYPE_PDF:
				case FILETYPE_UNKNOWN:
										program = CONFIG ()->universalViewer (); break;
				case FILETYPE_VMR:
					EDITOR ()->startNewReport ()->load ( filename, true );
					EDITOR ()->show ();
					return;
				default:
					return;
			}
			args << filename;
			fileOps::executeFork ( args, program );
		}
		break;
		case SFO_REMOVE_FILE:
			removeFiles ( const_cast<QTreeWidgetItem*>( item ), true );
		break;
		default:
			program = CONFIG ()->fileManager ();
			switch ( action_id )
			{
				case OPEN_SELECTED_DIR:
					args << filename;
					fileOps::executeFork ( args, program );
				break;
				case OPEN_ESTIMATES_DIR:
					args << CONFIG ()->estimatesDir ( clientName );
					fileOps::executeFork ( args, program );
				break;
				case OPEN_REPORTS_DIR:
					args << CONFIG ()->reportsDir ( clientName );
					fileOps::executeFork ( args, program );
				break;
				case OPEN_CLIENT_DIR:
					args << CONFIG ()->getProjectBasePath ( clientName );
					fileOps::executeFork ( args, program );
				break;
			}
		break;
	}
}
