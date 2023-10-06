#include "searchui.h"
#include "system_init.h"
#include "companypurchases.h"
#include "suppliersdlg.h"
#include "mainwindow.h"

#include <heapmanager.h>
#include <vmTaskPanel/vmtaskpanel.h>
#include <vmTaskPanel/vmactiongroup.h>
#include <vmUtils/fast_library_functions.h>
#include <vmWidgets/vmtablewidget.h>
#include <dbRecords/dblistitem.h>
#include <dbRecords/dbsupplies.h>
#include <dbRecords/supplierrecord.h>
#include <dbRecords/companypurchases.h>
#include <dbRecords/vivenciadb.h>
#include <vmNotify/vmnotify.h>

#include <QtCore/QModelIndex>
#include <QtSql/QSqlQuery>
#include <QtWidgets/QApplication>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>
#include <QtGui/QKeyEvent>

static const QString searchQueryTemplate ( QStringLiteral ( "SELECT ID#1# FROM #2# WHERE MATCH (#3#) AGAINST (\"#4#\" IN BOOLEAN MODE)" ) );

enum listColumns { COL_ID = 0, COL_INFO, COL_TABLE, COL_DATE, COL_FIELD };

static const QString TYPE_NAMES[9] = {
	emptyString, emptyString, //table orders TABLE_GENERAL_ORDER and TABLE_USERS_ORDER are not used
	APP_TR_FUNC ( "Client" ),
	APP_TR_FUNC ( "Job" ),
	APP_TR_FUNC ( "Payment" ),
	APP_TR_FUNC ( "Purchase" ),
	APP_TR_FUNC ( "Inventory" ),
	APP_TR_FUNC ( "Supply" ),
	APP_TR_FUNC ( "Supplier" ),
};

searchUI::searchUI ()
	: QDialog (), mSearchFields ( 0 ), mCurRow ( -1 ), mFoundList ( nullptr ), mCurrentDisplayedRec ( nullptr )
{}

searchUI::~searchUI ()
{
	if ( mCurrentDisplayedRec )
		mCurrentDisplayedRec->clearSearchStatus (); //Otherwise the information will be left on the database
}

void searchUI::setupUI ()
{
	if ( mFoundList != nullptr )
		return;

	setWindowTitle ( PROGRAM_NAME + TR_FUNC ( " Search results" ) );
	setWindowIcon ( ICON ( "search" ) );
	setWindowModality ( Qt::WindowModal );

	createTable ();

	mtxtSearch = new vmLineEdit;
	mtxtSearch->setEditable ( true );
	static_cast<void>( connect ( mtxtSearch, &QLineEdit::textEdited, this, [&] ( const QString& text ) { return on_txtSearch_textEdited ( text ); } ) );
	mtxtSearch->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) { return searchCallbackSelector ( ke ); } );

	mBtnSearch = new QToolButton;
	mBtnSearch->setIcon ( ICON ( "search" ) );
	connect ( mBtnSearch,  &QPushButton::clicked, this, [&] () { return btnSearchClicked (); } );
	mBtnPrev = new QToolButton;
	mBtnPrev->setIcon ( ICON ( "arrow-left" ) );
	connect ( mBtnPrev,  &QPushButton::clicked, this, [&] () { return btnPrevClicked (); } );
	mBtnNext = new QToolButton;
	mBtnNext->setIcon ( ICON ( "arrow-right" ) );
	connect ( mBtnNext,  &QPushButton::clicked, this, [&] () { return btnNextClicked (); } );
	mBtnClose = new QPushButton ( APP_TR_FUNC ( "Hide" ) );
	connect ( mBtnClose, &QPushButton::clicked, this, [&] () { hide (); } );

	vmTaskPanel* panel ( new vmTaskPanel ( emptyString, this ) );
	auto mainLayout ( new QVBoxLayout );
	mainLayout->setContentsMargins ( 0, 0, 0, 0 );
	mainLayout->setSpacing ( 0 );
	mainLayout->addWidget ( panel, 1 );
	setLayout ( mainLayout );
	vmActionGroup* group = panel->createGroup ( ICON ( "search"), APP_TR_FUNC ( "Database search" ), false, true );
	group->addEntry ( mFoundList, nullptr, true );

	auto hLayout ( new QHBoxLayout );
	hLayout->setSpacing ( 1 );
	hLayout->setContentsMargins ( 0, 0, 0, 0 );
	hLayout->addWidget ( mtxtSearch, 1, Qt::AlignLeft );
	hLayout->addWidget ( mBtnSearch, 0, Qt::AlignLeft );
	hLayout->addWidget ( mBtnPrev, 0, Qt::AlignLeft );
	hLayout->addWidget ( mBtnNext, 0, Qt::AlignLeft );
	hLayout->addStretch ( 2 );
	hLayout->addWidget ( mBtnClose, 1, Qt::AlignRight );
	group->addLayout ( hLayout );

	panel->setScheme ( MAINWINDOW ()->appMainStyle () );
	resize ( mFoundList->preferredSize () );
	mtxtSearch->setFocus ();
}

void searchUI::searchCancel ()
{
	if ( mCurrentDisplayedRec )
		mCurrentDisplayedRec->clearSearchStatus ();
	mFoundItems.clear ();
	mFoundList->clear ();
	mCurRow = -1;
	mSearchTerm.clear ();
	searchStatus.setState ( SS_NOT_SEARCHING );
}

void searchUI::prepareSearch ( const QString& searchTerm )
{
	searchCancel ();
	parseSearchTerm ( searchTerm );
	searchStatus.setState ( SS_NOT_FOUND ); //Undefined is synonym to can start search
}

void searchUI::parseSearchTerm ( const QString& searchTerm )
{
	uint char_pos ( 0 );
	mSearchFields = 0;

	if ( searchTerm.at ( 0 ) == CHR_PERCENT )
	{
		while ( searchTerm.at ( static_cast<int>( ++char_pos ) ) != CHR_SPACE )
		{
			if ( char_pos >= static_cast<uint>( searchTerm.count () - 1 ) )
				break;

			switch ( searchTerm.at ( static_cast<int>( char_pos ) ).toLatin1 () )
			{
				case 'r': // job, buy reports
				case 'R':
					setBit ( mSearchFields, SC_REPORT_1 );
					setBit ( mSearchFields, SC_REPORT_2 );
				break;
				case '$': // pay total, pay value, buy total, buy value
					setBit ( mSearchFields, SC_PRICE_1 );
					setBit ( mSearchFields, SC_PRICE_2 );
				break;
				case 'n': // client name
				case 'N':
					setBit ( mSearchFields, SC_TYPE );
				break;
				case 'e': // addresses, including street, number, district, city
				case 'E':
					setBit ( mSearchFields, SC_ADDRESS_1 );
					setBit ( mSearchFields, SC_ADDRESS_2 );
					setBit ( mSearchFields, SC_ADDRESS_3 );
					setBit ( mSearchFields, SC_ADDRESS_4 );
					setBit ( mSearchFields, SC_ADDRESS_5 );
				break;
				case 'f': // phone numbers
				case 'F':
					setBit ( mSearchFields, SC_PHONES );
				break;
				case '@':
					setBit ( mSearchFields, SC_ONLINE_ADDRESS );
				break;
				case 'i': // client, job, sched, pay, buy ids
				case 'I':
					setBit ( mSearchFields, SC_ID );
				break;
				case '#': // project number
					setBit ( mSearchFields, SC_EXTRA );
				break;
				case 'd': // all dates
				case 'D':
					setBit ( mSearchFields, SC_DATE_1 );
					setBit ( mSearchFields, SC_DATE_2 );
					setBit ( mSearchFields, SC_DATE_3 );
				break;
				case 'h': // job mower time, job time
				case 'H':
					setBit ( mSearchFields, SC_TIME_1 );
					setBit ( mSearchFields, SC_TIME_2 );
				break;
				case 't': // job type, sched type
				case 'T':
					setBit ( mSearchFields, SC_TYPE );
				break;
				default:
				break;
			}
		}
	}
	else // search all
		setAllBits ( mSearchFields );

	if ( char_pos > 0 )
		mSearchTerm = searchTerm.right ( searchTerm.count () - static_cast<int>(char_pos + 1) );
	else
		mSearchTerm = searchTerm;
}

void searchUI::searchClients ()
{
	Client client;
	int recordcategory ( -1 );
	QSqlQuery query;
	stringRecord itemInfo;
	QString strColName;

	QString strQuery ( searchQueryTemplate );
	strQuery.replace ( QStringLiteral( "#1#" ), ",NAME,BEGINDATE" ); // Fields (columns) to display in table
	strQuery.replace ( QStringLiteral( "#2#" ), client.tableInfo ()->table_name ); // Table name
	strQuery.replace ( QStringLiteral( "#4#" ), mSearchTerm ); //The searched string
	QString strQuery2 ( strQuery );

	for ( uint i ( SC_REPORT_1 ); i <= SC_EXTRA; ++i )
	{
		if ( isBitSet ( mSearchFields, i ) )
		{
			recordcategory = client.searchCategoryTranslate ( static_cast<SEARCH_CATEGORIES> ( i ) );
			if ( recordcategory <= 0 )
				continue;

			strColName = VDB ()->getTableColumnName ( client.tableInfo (), static_cast<uint>(recordcategory) );
			strQuery.replace ( QStringLiteral( "#3#" ), strColName ); //Field (column) in which to search for the search term
			if ( VDB ()->runSelectLikeQuery ( strQuery, query ) )
			{
				do
				{
					itemInfo.insertField ( COL_ID, query.value ( 0 ).toString () );
					itemInfo.insertField ( COL_INFO, query.value ( 1 ).toString () );
					itemInfo.insertField ( COL_TABLE, client.tableInfo ()->table_name );
					itemInfo.insertField ( COL_DATE, query.value ( 2 ).toString () );
					itemInfo.insertField ( COL_FIELD, strColName );
					mFoundItems.fastAppendRecord ( itemInfo );
					itemInfo.clear ();
				} while ( query.next () );
			}
			strQuery = strQuery2;
		}
	}
}

void searchUI::searchJobs ()
{
	Job job;
	int recordcategory ( -1 );
	QSqlQuery query;
	stringRecord itemInfo;
	QString strColName;

	QString strQuery ( searchQueryTemplate );
	strQuery.replace ( QStringLiteral( "#1#" ), ",CLIENTID,TYPE,STARTDATE" ); // Fields (columns) to display in table
	strQuery.replace ( QStringLiteral( "#2#" ), job.tableInfo ()->table_name ); // Table name
	strQuery.replace ( QStringLiteral( "#4#" ), mSearchTerm ); //The searched string
	QString strQuery2 ( strQuery );

	for ( uint i ( SC_REPORT_1 ); i <= SC_EXTRA; ++i )
	{
		if ( isBitSet ( mSearchFields, i ) )
		{
			recordcategory = job.searchCategoryTranslate ( static_cast<SEARCH_CATEGORIES> ( i ) );
			if ( recordcategory <= 0 )
				continue;

			strColName = VDB ()->getTableColumnName ( job.tableInfo (), static_cast<uint>(recordcategory) );
			strQuery.replace ( QStringLiteral( "#3#" ), strColName ); //Field (column) in which to search for the search term
			if ( VDB ()->runSelectLikeQuery ( strQuery, query ) )
			{
				do
				{
					itemInfo.insertField ( COL_ID, query.value ( 0 ).toString () + CHR_COMMA + query.value ( 1 ).toString () );
					itemInfo.insertField ( COL_INFO, query.value ( 2 ).toString () + CHR_HYPHEN + Client::clientName ( query.value ( 1 ).toString () ) );
					itemInfo.insertField ( COL_TABLE, job.tableInfo ()->table_name );
					itemInfo.insertField ( COL_DATE, query.value ( 3 ).toString () );
					itemInfo.insertField ( COL_FIELD, strColName );
					mFoundItems.fastAppendRecord ( itemInfo );
					itemInfo.clear ();
				} while ( query.next () );
			}
			strQuery = strQuery2;
		}
	}
}

void searchUI::searchPayments ()
{
	Payment pay;
	int recordcategory ( -1 );
	QSqlQuery query;
	stringRecord itemInfo;
	QString strColName;

	QString strQuery ( searchQueryTemplate );
	strQuery.replace ( QStringLiteral( "#1#" ), ",CLIENTID,PRICE" ); // Fields (columns) to display in table
	strQuery.replace ( QStringLiteral( "#2#" ), pay.tableInfo ()->table_name ); // Table name
	strQuery.replace ( QStringLiteral( "#4#" ), mSearchTerm ); //The searched string
	QString strQuery2 ( strQuery );

	for ( uint i ( SC_REPORT_1 ); i <= SC_EXTRA; ++i )
	{
		if ( isBitSet ( mSearchFields, i ) )
		{
			recordcategory = pay.searchCategoryTranslate ( static_cast<SEARCH_CATEGORIES> ( i ) );
			if ( recordcategory <= 0 )
				continue;

			strColName = VDB ()->getTableColumnName ( pay.tableInfo (), static_cast<uint>(recordcategory) );
			strQuery.replace ( QStringLiteral( "#3#" ), strColName ); //Field (column) in which to search for the search term
			if ( VDB ()->runSelectLikeQuery ( strQuery, query ) )
			{
				do
				{
					itemInfo.insertField ( COL_ID, query.value ( 0 ).toString () + CHR_COMMA + query.value ( 1 ).toString () );
					itemInfo.insertField ( COL_INFO, Client::clientName ( query.value ( 1 ).toString () ) + CHR_HYPHEN + query.value ( 2 ).toString () );
					itemInfo.insertField ( COL_TABLE, pay.tableInfo ()->table_name );
					itemInfo.insertField ( COL_DATE, emptyString );
					itemInfo.insertField ( COL_FIELD, strColName );
					mFoundItems.fastAppendRecord ( itemInfo );
					itemInfo.clear ();
				} while ( query.next () );
			}
			strQuery = strQuery2;
		}
	}
}

void searchUI::searchPurchases ()
{
	Buy buy;
	int recordcategory ( -1 );
	QSqlQuery query;
	stringRecord itemInfo;
	QString strColName;

	QString strQuery ( searchQueryTemplate );
	strQuery.replace ( QStringLiteral( "#1#" ), ",CLIENTID,PRICE,DATE" ); // Fields (columns) to display in table
	strQuery.replace ( QStringLiteral( "#2#" ), buy.tableInfo ()->table_name ); // Table name
	strQuery.replace ( QStringLiteral( "#4#" ), mSearchTerm ); //The searched string
	QString strQuery2 ( strQuery );

	for ( uint i ( SC_REPORT_1 ); i <= SC_EXTRA; ++i )
	{
		if ( isBitSet ( mSearchFields, i ) )
		{
			recordcategory = buy.searchCategoryTranslate ( static_cast<SEARCH_CATEGORIES> ( i ) );
			if ( recordcategory <= 0 )
				continue;

			strColName = VDB ()->getTableColumnName ( buy.tableInfo (), static_cast<uint>(recordcategory) );
			strQuery.replace ( QStringLiteral( "#3#" ), strColName ); //Field (column) in which to search for the search term
			if ( VDB ()->runSelectLikeQuery ( strQuery, query ) )
			{
				do
				{
					itemInfo.insertField ( COL_ID, query.value ( 0 ).toString () + CHR_COMMA + query.value ( 1 ).toString () );
					itemInfo.insertField ( COL_INFO, Client::clientName ( query.value ( 1 ).toString () ) + CHR_HYPHEN + query.value ( 2 ).toString () );
					itemInfo.insertField ( COL_TABLE, buy.tableInfo ()->table_name );
					itemInfo.insertField ( COL_DATE, query.value ( 3 ).toString () );
					itemInfo.insertField ( COL_FIELD, strColName );
					mFoundItems.fastAppendRecord ( itemInfo );
					itemInfo.clear ();
				} while ( query.next () );
			}
			strQuery = strQuery2;
		}
	}
}

void searchUI::searchInventory ()
{
	companyPurchases cp_rec;
	int recordcategory ( -1 );
	QSqlQuery query;
	stringRecord itemInfo;
	QString strColName;

	QString strQuery ( searchQueryTemplate );
	strQuery.replace ( QStringLiteral( "#1#" ), ",SUPPLIER,DELIVERY_METHOD,TOTAL_PRICE,DATE" ); // Fields (columns) to display in table
	strQuery.replace ( QStringLiteral( "#2#" ), cp_rec.tableInfo ()->table_name ); // Table name
	strQuery.replace ( QStringLiteral( "#4#" ), mSearchTerm ); //The searched string
	QString strQuery2 ( strQuery );

	for ( uint i ( SC_REPORT_1 ); i <= SC_EXTRA; ++i )
	{
		if ( isBitSet ( mSearchFields, i ) )
		{
			recordcategory = cp_rec.searchCategoryTranslate ( static_cast<SEARCH_CATEGORIES> ( i ) );
			if ( recordcategory <= 0 )
				continue;

			strColName = VDB ()->getTableColumnName ( cp_rec.tableInfo (), static_cast<uint>(recordcategory) );
			strQuery.replace ( QStringLiteral( "#3#" ), strColName ); //Field (column) in which to search for the search term
			if ( VDB ()->runSelectLikeQuery ( strQuery, query ) )
			{
				do
				{
					itemInfo.insertField ( COL_ID, query.value ( 0 ).toString () );
					itemInfo.insertField ( COL_INFO, query.value ( 1 ).toString () + CHR_HYPHEN + query.value ( 2 ).toString () + query.value ( 3 ).toString () );
					itemInfo.insertField ( COL_TABLE, cp_rec.tableInfo ()->table_name );
					itemInfo.insertField ( COL_DATE, query.value ( 4 ).toString () );
					itemInfo.insertField ( COL_FIELD, strColName );
					mFoundItems.fastAppendRecord ( itemInfo );
					itemInfo.clear ();
				} while ( query.next () );
			}
			strQuery = strQuery2;
		}
	}
}

void searchUI::searchSupplies ()
{

}

void searchUI::searchSuppliers ()
{
	supplierRecord sup_rec;
	int recordcategory ( -1 );
	QSqlQuery query;
	stringRecord itemInfo;
	QString strColName;

	QString strQuery ( searchQueryTemplate );
	strQuery.replace ( QStringLiteral( "#1#" ), ",NAME" ); // Fields (columns) to display in table
	strQuery.replace ( QStringLiteral( "#2#" ), sup_rec.tableInfo ()->table_name ); // Table name
	strQuery.replace ( QStringLiteral( "#4#" ), mSearchTerm ); //The searched string
	QString strQuery2 ( strQuery );

	for ( uint i ( SC_REPORT_1 ); i <= SC_EXTRA; ++i )
	{
		if ( isBitSet ( mSearchFields, i ) )
		{
			recordcategory = sup_rec.searchCategoryTranslate ( static_cast<SEARCH_CATEGORIES> ( i ) );
			if ( recordcategory <= 0 )
				continue;

			strColName = VDB ()->getTableColumnName ( sup_rec.tableInfo (), static_cast<uint>(recordcategory) );
			strQuery.replace ( QStringLiteral( "#3#" ), strColName ); //Field (column) in which to search for the search term
			if ( VDB ()->runSelectLikeQuery ( strQuery, query ) )
			{
				do
				{
					itemInfo.insertField ( COL_ID, query.value ( 0 ).toString () );
					itemInfo.insertField ( COL_INFO, query.value ( 1 ).toString () );
					itemInfo.insertField ( COL_TABLE, sup_rec.tableInfo ()->table_name );
					itemInfo.insertField ( COL_DATE, emptyString );
					itemInfo.insertField ( COL_FIELD, strColName );
					mFoundItems.fastAppendRecord ( itemInfo );
					itemInfo.clear ();
				} while ( query.next () );
			}
			strQuery = strQuery2;
		}
	}
}

void searchUI::search ( const uint search_start, const uint search_end )
{
	if ( !canSearch () ) return; // only in canSearch mode we proceed

	uint search_step ( search_start );

	do {
		switch ( search_step )
		{
			case SS_CLIENT:
				searchClients ();
			break;
			case SS_JOB:
				searchJobs ();
			break;
			case SS_PAY:
				searchPayments ();
			break;
			case SS_BUY:
				searchPurchases ();
			break;
			case SS_INVENTORY:
				searchInventory ();
			break;
			case SS_SUPPLIES:
				searchSupplies ();
			break;
			case SS_SUPPLIERS:
				searchSuppliers ();
			break;
			default:
			continue;
		}
	} while ( ++search_step <= search_end );

	NOTIFY ()->notifyMessage ( TR_FUNC ( "Search panel notification" ), TR_FUNC ( "Search completed!" ) );
	if ( mFoundItems.countRecords () > 0 )
	{
		mFoundList->setIgnoreChanges ( true );
		static_cast<void>( disconnect ( mFoundList, &QTableWidget::itemSelectionChanged, nullptr, nullptr ) );
		mFoundList->loadFromStringTable ( mFoundItems );
		mFoundList->setIgnoreChanges ( false );
		static_cast<void>( connect ( mFoundList, &QTableWidget::itemSelectionChanged, this, [&] {
											listRowSelected ( mFoundList->currentRow () ); } ) );
		searchStatus.setState ( searchFirst () ? SS_SEARCH_FOUND : SS_NOT_FOUND );
	}
	else
	{
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Search panel notification" ), TR_FUNC ( "Nothing found!" ) );
		searchStatus.setState ( SS_NOT_SEARCHING );
	}
}

bool searchUI::searchFirst ()
{
	mFoundList->setCurrentCell ( 0, 0, QItemSelectionModel::ClearAndSelect );
	return ( mFoundList->currentRow () == 0 );
}

bool searchUI::searchPrev ()
{
	if ( mFoundList->currentRow () > 0 )
	{
		mFoundList->setCurrentCell ( mFoundList->currentRow () - 1, 0, QItemSelectionModel::ClearAndSelect );
		return true;
	}
	return false;
}

bool searchUI::searchNext ()
{
	if ( mFoundList->currentRow () < mFoundList->rowCount () - 1 )
	{
		mFoundList->setCurrentCell ( mFoundList->currentRow () + 1, 0, QItemSelectionModel::ClearAndSelect );
		return true;
	}
	return false;
}

bool searchUI::searchLast ()
{
	mFoundList->setCurrentCell ( mFoundList->rowCount () - 1, 0, QItemSelectionModel::ClearAndSelect );
	return !( mFoundList->isEmpty () );
}

bool searchUI::isLast () const
{
	return mFoundList->currentRow () == mFoundList->rowCount () - 1;
}

bool searchUI::isFirst () const
{
	return mFoundList->currentRow () == 0;
}

void searchUI::createTable ()
{
	mFoundList = new vmTableWidget;
	mFoundList->setIsPlainTable ( true );
	mFoundList->setColumnsAutoResize ( true );

	vmTableColumn* __restrict cols ( mFoundList->createColumns ( 5 ) );
	for ( uint i ( 0 ); i < 5; ++i )
	{
		switch ( i )
		{
			case COL_ID:
				cols[COL_ID].label = TR_FUNC( "ID" );
				cols[COL_ID].width = 130;
			break;
			case COL_INFO:
				cols[COL_INFO].label = TR_FUNC( "Info" );
				cols[COL_INFO].width = 130;
			break;
			case COL_TABLE:
				cols[COL_TABLE].label = TR_FUNC ( "Table" );
				cols[COL_TABLE].width = 180;
			break;
			case COL_DATE:
				cols[COL_DATE].label = TR_FUNC ( "Date" );
				cols[COL_DATE].wtype = WT_DATEEDIT;
			break;
			case COL_FIELD:
				cols[COL_FIELD].label = TR_FUNC ( "Found Field(s)" );
				cols[COL_FIELD].width = 150;
			break;
		}
	}
	mFoundList->initTable ( 5 );
	mFoundList->setEditable ( false );
}

void searchUI::on_txtSearch_textEdited ( const QString& text )
{
	mBtnSearch->setEnabled ( false );
	if ( text.length () >= 3 )
	{
		if ( mSearchTerm != text )
			mBtnSearch->setEnabled ( true );
	}
	else if ( text.length () == 0 )
		searchCancel ();
	mBtnPrev->setEnabled ( false );
	mBtnNext->setEnabled ( false );
}

void searchUI::searchCallbackSelector ( const QKeyEvent* const ke )
{
	switch ( ke->key () )
	{
		case Qt::Key_Enter:
		case Qt::Key_Return:
			if ( mBtnSearch->isEnabled () )
				btnSearchClicked ();
		break;
		case Qt::Key_Escape:
			mtxtSearch->setText ( mSearchTerm );
			mBtnSearch->setEnabled ( false );
			mBtnPrev->setEnabled ( mFoundList->currentRow () > 0 );
			mBtnNext->setEnabled ( mFoundList->currentRow () < mFoundList->rowCount () - 1 );
		break;
		default:
		break;
	}
}

void searchUI::btnSearchClicked ()
{
	prepareSearch ( mtxtSearch->text () );
	search ();
	mBtnSearch->setEnabled ( false );
	mBtnPrev->setEnabled ( false );
	mBtnNext->setEnabled ( searchFirst () );
}

void searchUI::btnPrevClicked ()
{
	const int row ( mFoundList->currentRow () );
	if ( row > 0 )
	{
		mBtnNext->setEnabled ( true );
		mFoundList->selectRow ( row - 1 );
		mBtnPrev->setEnabled ( row > 1 );
	}
}

void searchUI::btnNextClicked ()
{
	const int row ( mFoundList->currentRow () );
	if ( row < mFoundList->rowCount () - 1 )
	{
		mBtnPrev->setEnabled ( true );
		mFoundList->selectRow ( row + 1 );
		mBtnNext->setEnabled ( row < mFoundList->rowCount () - 1 );
	}
}

void searchUI::listRowSelected ( const int row )
{
	if ( mCurRow == row )
		return;
	mCurRow = row;
	uint table_id ( VivenciaDB::tableIDFromTableName ( mFoundList->sheetItem ( row, COL_TABLE )->text () ) );
	QString str_id ( mFoundList->sheetItem ( row, COL_ID )->text () );
	uint dbrec_id ( 0 );
	clientListItem* client_item ( nullptr );
	dbListItem* item ( nullptr );

	if ( str_id.contains ( CHR_COMMA ) )
	{
		dbrec_id = str_id.right ( str_id.length () - ( str_id.indexOf ( CHR_COMMA ) + 1 ) ).toUInt (); // client id, the number after the comma
		client_item = MAINWINDOW ()->getClientItem ( dbrec_id );
		MAINWINDOW ()->displayClient ( client_item, true );
	}
	dbrec_id = str_id.left ( str_id.indexOf ( CHR_COMMA ) ).toUInt (); //record id, the number before the comma
	if ( mCurrentDisplayedRec )
		mCurrentDisplayedRec->clearSearchStatus ();

	switch ( table_id )
	{
		case CLIENT_TABLE:
			item = MAINWINDOW ()->getClientItem ( dbrec_id );
			mCurrentDisplayedRec = static_cast<DBRecord*>(item->dbRec ());
			mCurrentDisplayedRec->setSearchStatus ( VivenciaDB::getTableColumnIndex ( mCurrentDisplayedRec->tableInfo (), mFoundList->sheetItem ( row, COL_FIELD )->text () ) , true );
			MAINWINDOW ()->displayClient ( static_cast<clientListItem*>(item), true );
			MAINWINDOW ()->navigateToClient ();
			MAINWINDOW ()->insertNavItem ( item );
		break;
		case JOB_TABLE:
			item = MAINWINDOW ()->getJobItem ( client_item, dbrec_id );
			mCurrentDisplayedRec = static_cast<DBRecord*>(item->dbRec ());
			mCurrentDisplayedRec->setSearchStatus ( VivenciaDB::getTableColumnIndex ( mCurrentDisplayedRec->tableInfo (), mFoundList->sheetItem ( row, COL_FIELD )->text () ) , true );
			MAINWINDOW ()->displayClient ( client_item, true, static_cast<jobListItem*>(item) );
			MAINWINDOW ()->navigateToJob ();
			MAINWINDOW ()->insertNavItem ( item );
		break;
		case PAYMENT_TABLE:
			item = MAINWINDOW ()->getPayItem ( client_item, dbrec_id );
			mCurrentDisplayedRec = static_cast<DBRecord*>(item->dbRec ());
			mCurrentDisplayedRec->setSearchStatus ( VivenciaDB::getTableColumnIndex ( mCurrentDisplayedRec->tableInfo (), mFoundList->sheetItem ( row, COL_FIELD )->text () ) , true );
			MAINWINDOW ()->displayClient ( client_item, true, static_cast<jobListItem*>(item->relatedItem(RLI_JOBPARENT) ) );
			MAINWINDOW ()->displayPay ( static_cast<payListItem*>(item), true );
			MAINWINDOW ()->navigateToPay ();
			MAINWINDOW ()->insertNavItem ( item );
		break;
		case PURCHASE_TABLE:
			item = MAINWINDOW ()->getBuyItem ( client_item, dbrec_id );
			mCurrentDisplayedRec = static_cast<DBRecord*>(item->dbRec ());
			mCurrentDisplayedRec->setSearchStatus ( VivenciaDB::getTableColumnIndex ( mCurrentDisplayedRec->tableInfo (), mFoundList->sheetItem ( row, COL_FIELD )->text () ) , true );
			MAINWINDOW ()->displayClient ( client_item, true, static_cast<jobListItem*>(item->relatedItem (RLI_JOBPARENT)), static_cast<buyListItem*>(item) );
			MAINWINDOW ()->navigateToBuy ();
			MAINWINDOW ()->insertNavItem ( item );
		break;
		case COMPANY_PURCHASES_TABLE:
			COMPANY_PURCHASES ()->showSearchResult ( dbrec_id, true );
		break;
		case SUPPLIER_TABLE:
			SUPPLIERS ()->displaySupplier ( mFoundList->sheetItem ( row, COL_INFO )->text (), true );
		break;
		//case SUPPLIES_TABLE:
		//	SUPPLIES ()->showSearchResult ( strDBRec_id.toUInt (), bshow );
		//break;
		default:
		break;
	}
}
