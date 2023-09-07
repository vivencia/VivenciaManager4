#include "searchui.h"
#include "system_init.h"
#include "companypurchases.h"
#include "suppliersdlg.h"
#include "mainwindow.h"

#include <heapmanager.h>
#include <vmUtils/fast_library_functions.h>
#include <vmWidgets/vmtablewidget.h>
#include <dbRecords/dblistitem.h>
#include <dbRecords/dbsupplies.h>
#include <dbRecords/supplierrecord.h>
#include <dbRecords/inventory.h>
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

searchUI* searchUI::s_instance ( nullptr );

enum listColumns { COL_TYPE = 0, COL_CLIENT, COL_JOB, COL_DATE, COL_FIELD };

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

void deleteSearchUIInstance ()
{
	heap_del ( searchUI::s_instance );
}

searchUI::searchUI ()
	: QDialog (), mbShow ( true ), mSearchFields ( 0 ), mWidget ( nullptr ),
	  mFoundItems ( 10 ), mPreviusItemActivated ( nullptr ), displayFunc ( nullptr )
{
	setupUI ();
	setWindowTitle ( PROGRAM_NAME + TR_FUNC ( " Search results" ) );
}

searchUI::~searchUI ()
{
	dbListItem* item ( mFoundItems.first () );
	while ( item != nullptr )
	{
		if ( item->id () >= SUPPLIES_TABLE )
			delete item; //delete the temporary item and its temporary DBRecord
		item = mFoundItems.next ();
	}
	mFoundItems.clear ();
}

void searchUI::searchCancel ()
{
	if ( displayFunc )
		displayFunc ( mFoundItems.current (), false );

	mFoundList->setIgnoreChanges ( true );
	dbListItem* item ( mFoundItems.first () );
	uint i ( 0 ), max ( 0 );
	while ( item != nullptr )
	{
		if ( item->id () >= SUPPLIES_TABLE )
			delete item; //delete the temporary item and its temporary DBRecord
		else
		{
			max = item->dbRec ()->tableInfo ()->field_count;
			for ( i = 0; i < max; ++i )
				item->setSearchFieldStatus ( i, SS_NOT_SEARCHING );
		}
		item = mFoundItems.next ();
	}
	mFoundItems.clearButKeepMemory ();
	mFoundList->clear ();
	mSearchFields = 0;
	displayFunc = nullptr;
	mSearchTerm.clear ();
	searchStatus.setState ( SS_NOT_SEARCHING );
}

void searchUI::prepareSearch ( const QString& searchTerm, QWidget* widget )
{
	if ( static_cast<SEARCH_STATUS>( searchStatus.state () ) != SS_SEARCH_FOUND )
	{
		if ( searchTerm.length () >= 2 && mSearchTerm != searchTerm )
		{
			searchCancel ();
			parseSearchTerm ( searchTerm );
			mbShow = widget != nullptr;
			mWidget = widget;
			searchStatus.setState ( SS_NOT_FOUND ); //Undefined is synonym to can start search
		}
	}
}

void searchUI::parseSearchTerm ( const QString& searchTerm )
{
	uint char_pos ( 0 );

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
				case 'e': // client address
				case 'E':
					setBit ( mSearchFields, SC_ADDRESS_1 );
					setBit ( mSearchFields, SC_ADDRESS_2 );
					setBit ( mSearchFields, SC_ADDRESS_3 );
					setBit ( mSearchFields, SC_ADDRESS_4 );
					setBit ( mSearchFields, SC_ADDRESS_5 );
					setBit ( mSearchFields, SC_EXTRA_1 );
				break;
				case 'f': // client phones
				case 'F':
					setBit ( mSearchFields, SC_PHONE_1 );
					setBit ( mSearchFields, SC_PHONE_2 );
					setBit ( mSearchFields, SC_PHONE_3 );
				break;
				case 'i': // client, job, sched, pay, buy ids
				case 'I':
					setBit ( mSearchFields, SC_ID );
				break;
				case '#': // project number
					setBit ( mSearchFields, SC_EXTRA_1 );
					setBit ( mSearchFields, SC_EXTRA_2 );
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

void searchUI::search ( const uint search_start, const uint search_end )
{
	if ( !canSearch () ) return; // only in canSearch mode we proceed

	uint i ( 0 ), id ( 0 ), c_id ( 0 );
	int recordcategory ( -1 );

	DBRecord* dbrec ( nullptr );
	Client client_rec ( false );
	Job job_rec ( false );
	Payment pay_rec ( false );
	Buy buy_rec ( false );
	Inventory inv_rec ( false );
	dbSupplies sup_rec ( false );
	supplierRecord supplier_rec ( false );
	dbListItem* item ( nullptr );
	clientListItem* client_parent ( nullptr );
	uint search_step ( search_start ), fld ( 0 );
	QSqlQuery query;
	bool bhas_clientid ( false ), bhas_result ( false );

	do {
		bhas_clientid = false;
		switch ( search_step )
		{
			case SS_CLIENT:
				dbrec = &client_rec;
			break;
			case SS_JOB:
				dbrec = &job_rec;
				bhas_clientid = true;
			break;
			case SS_PAY:
				dbrec = &pay_rec;
				bhas_clientid = true;
			break;
			case SS_BUY:
				dbrec = &buy_rec;
				bhas_clientid = true;
			break;
			case SS_INVENTORY:
				dbrec = &inv_rec;
			break;
			case SS_SUPPLIES:
				dbrec = &sup_rec;
			break;
			case SS_SUPPLIERS:
				dbrec = &supplier_rec;
			break;
			default:
			continue;
		}

		QString colsList;
		for ( i = SC_REPORT_1; i <= SC_EXTRA_2; ++i )
		{
			recordcategory = dbrec->searchCategoryTranslate ( static_cast<SEARCH_CATEGORIES> ( i ) );
			if ( recordcategory >= 0 )
			{
				if ( isBitSet ( mSearchFields, static_cast<uchar>( recordcategory ) ) && dbrec->fieldType ( static_cast<uint>( recordcategory ) ) != DBTYPE_ID )
					colsList += VDB ()->getTableColumnName ( dbrec->tableInfo (), static_cast<uint>(recordcategory) ) + CHR_COMMA;
			}
		}
		if ( !colsList.isEmpty () )
		{
			colsList.chop ( 1 );
			const QString query_cmd ( (bhas_clientid ? QStringLiteral ( "SELECT ID,CLIENTID FROM " ) : QStringLiteral ( "SELECT ID FROM " )) + dbrec->tableInfo ()->table_name + QStringLiteral ( " WHERE MATCH (" ) +
					colsList + QStringLiteral ( ") AGAINST (\"" ) + mSearchTerm + QStringLiteral ( "\" IN BOOLEAN MODE)" ) );
			if ( VDB ()->runSelectLikeQuery ( query_cmd, query ) )
			{
				const uint fld_max ( dbrec->tableInfo ()->field_count );

				do
				{
					id = query.value ( 0 ).toUInt ();
					if ( bhas_clientid )
					{
						c_id = query.value ( 1 ).toUInt ();
						client_parent = MAINWINDOW ()->getClientItem ( c_id );
					}

					switch ( search_step )
					{
						case SS_CLIENT:
							item = MAINWINDOW ()->getClientItem ( id );
						break;
						case SS_JOB:
							item = MAINWINDOW ()->getJobItem ( client_parent, id );
							if ( item == nullptr )
							{
								item = MAINWINDOW ()->loadItem ( client_parent, id, JOB_TABLE );
							}
						break;
						case SS_PAY:
							item = MAINWINDOW ()->getPayItem ( client_parent, id );
							if ( item == nullptr )
							{
								item = MAINWINDOW ()->loadItem ( client_parent, id, PAYMENT_TABLE );
							}
						break;
						case SS_BUY:
							item = MAINWINDOW ()->getBuyItem ( client_parent, id );
							if ( item == nullptr )
							{
								item = MAINWINDOW ()->loadItem ( client_parent, id, PURCHASE_TABLE );
							}
						break;
						case SS_INVENTORY:
						case SS_SUPPLIES:
						case SS_SUPPLIERS:
							item = getOtherItem ( dbrec->typeID (), id ); break;
						default:
							return;
					}
					if ( item )
					{
						if ( mFoundItems.contains ( item ) == -1 )
						{
							// Sometimes, MySQL does not find exactly what we searched for, and we need to
							// filter its results
							if ( item->loadData ( true ) )
							{
								for ( fld = 0; fld < fld_max; ++fld )
								{
									if ( recStrValue ( item->dbRec (), fld ).contains ( mSearchTerm, Qt::CaseInsensitive ) )
									{
										item->setSearchArray (); // for the found item - only -, initialize the array
										item->setSearchFieldStatus ( fld, SS_SEARCH_FOUND );
										bhas_result = true;
									}
								}
							}
							if ( bhas_result )
							{
								mFoundItems.append ( item );
								bhas_result = false;
							}
						}
						item = nullptr;
					}
				} while ( query.next () );
			}
		}
	} while ( ++search_step <= search_end );
	NOTIFY ()->notifyMessage ( TR_FUNC ( "Search panel notification" ), TR_FUNC ( "Search completed!" ) );
	if ( !mFoundItems.isEmpty () )
	{
		fillList ();
		searchStatus.setState ( searchFirst () ? SS_SEARCH_FOUND : SS_NOT_FOUND );
		if ( mbShow )
		{
			move ( vmWidget::getGlobalWidgetPosition ( mWidget ) );
			show ();
		}
	}
	else
	{
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Search panel notification" ), TR_FUNC ( "Nothing found!" ) );
		searchStatus.setState ( SS_NOT_SEARCHING );
	}
}

void searchUI::fillList ()
{
	mFoundList->setIgnoreChanges ( true );

	dbListItem* item ( mFoundItems.first () );
	vmList<QString> strInfo;
	uint row ( 0 );

	while ( item != nullptr )
	{
		switch ( item->subType () )
		{
			case CLIENT_TABLE:
				getClientInfo ( static_cast<clientListItem*> ( item ), strInfo );
			break;
			case JOB_TABLE:
				getJobInfo ( static_cast<jobListItem*> ( item ), strInfo );
			break;
			case PAYMENT_TABLE:
				getPayInfo ( static_cast<payListItem*> ( item ), strInfo );
			break;
			case PURCHASE_TABLE:
				getBuyInfo ( static_cast<buyListItem*> ( item ), strInfo );
			break;
			case INVENTORY_TABLE:
			case SUPPLIES_TABLE:
			case SUPPLIER_TABLE:
				getOtherInfo ( item, strInfo );
			break;
			default:
			return;
		}
		mFoundList->setCellValue ( TYPE_NAMES[item->dbRec ()->tableInfo ()->table_order], row, COL_TYPE );
		//mFoundList->sheetItem ( row, COL_TYPE )->setInternalData ( QVariant::fromValue ( item ) );
		mFoundList->setCellValue ( strInfo[COL_CLIENT-1], row, COL_CLIENT );
		mFoundList->setCellValue ( strInfo[COL_JOB-1], row, COL_JOB );
		mFoundList->setCellValue ( strInfo[COL_DATE-1], row, COL_DATE );

		strInfo[COL_FIELD-1].chop ( 2 );
		mFoundList->setCellValue ( strInfo[COL_FIELD-1], row, COL_FIELD );
		item = mFoundItems.next ();
		++row;
		strInfo[COL_FIELD-1].clear ();
	}
	mFoundList->setIgnoreChanges ( false );
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

void searchUI::setupUI ()
{
	createTable ();
	mBtnPrev = new QToolButton;
	mBtnPrev->setIcon ( ICON ( "arrow-left" ) );
	connect ( mBtnPrev,  &QPushButton::clicked, this, [&] () { return btnPrevClicked (); } );
	mBtnNext = new QToolButton;
	mBtnNext->setIcon ( ICON ( "arrow-right" ) );
	connect ( mBtnNext,  &QPushButton::clicked, this, [&] () { return btnNextClicked (); } );
	mBtnClose = new QPushButton ( APP_TR_FUNC ( "Close" ) );
	connect ( mBtnClose, &QPushButton::clicked, this, [&] () { return listRowSelected ( mFoundList->currentRow () ); } );
	
	auto hLayout ( new QHBoxLayout );
	hLayout->setSpacing ( 1 );
	hLayout->setContentsMargins ( 0, 0, 0, 0 );
	hLayout->addWidget ( mBtnPrev, 0, Qt::AlignLeft );
	hLayout->addWidget ( mBtnNext, 0, Qt::AlignLeft );
	hLayout->addStretch ( 2 );
	hLayout->addWidget ( mBtnClose, 1, Qt::AlignRight );

	auto vLayout ( new QVBoxLayout );
	vLayout->setContentsMargins ( 2, 2, 2, 2 );
	vLayout->setSpacing ( 2 );
	vLayout->addWidget ( mFoundList, 2 );
	vLayout->addLayout ( hLayout );
	setLayout ( vLayout );
	setMinimumSize ( mFoundList->size () );
}

void searchUI::createTable ()
{
	mFoundList = new vmTableWidget;
	mFoundList->setIsPlainTable ( false );
	mFoundList->setColumnsAutoResize ( true );
	mFoundList->setCallbackForRowActivated ( [&] ( const int row ) { return listRowSelected ( row ); } );
	mFoundList->setMinimumWidth ( 600 );

	vmTableColumn* __restrict cols ( mFoundList->createColumns ( 5 ) );
	for ( uint i ( 0 ); i < 5; ++i )
	{
		switch ( i )
		{
			case COL_TYPE:
				cols[COL_TYPE].label = TR_FUNC ( "Type" );
				cols[COL_TYPE].width = 100;
			break;
			case COL_CLIENT:
				cols[COL_CLIENT].label = TR_FUNC( "Client" );
				cols[COL_CLIENT].width = 130;
			break;
			case COL_JOB:
				cols[COL_JOB].label = TR_FUNC ( "Job" );
				cols[COL_JOB].width = 180;
			break;
			case COL_DATE:
				cols[COL_DATE].label = TR_FUNC ( "Date" );
				cols[COL_DATE].wtype = WT_DATEEDIT;
			break;
			case COL_FIELD:
				cols[COL_FIELD].label = TR_FUNC ( "Found Field(s)" );
				cols[COL_JOB].width = 150;
			break;
		}
	}

	mFoundList->initTable ( 5 );
	mFoundList->setEditable ( false );
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
	if ( displayFunc && mPreviusItemActivated )
		displayFunc ( mFoundItems.at ( mPreviusItemActivated->row () ), false );

	dbListItem* curItem ( row != -1 ? mFoundItems.at ( row ) : nullptr );
	if ( curItem )
	{
		switch ( curItem->subType () )
		{
			case CLIENT_TABLE:
				displayFunc = [&] ( dbListItem* item, const bool bshow ) {
						return MAINWINDOW ()->showClientSearchResult ( item, bshow ); };
			break;
			case JOB_TABLE:
				displayFunc = [&] ( dbListItem* item, const bool bshow ) {
						return MAINWINDOW ()->showJobSearchResult ( item, bshow ); };
			break;
			case PAYMENT_TABLE:
				displayFunc = [&] ( dbListItem* item, const bool bshow ) {
						return MAINWINDOW ()->showPaySearchResult ( item, bshow ); };
			break;
			case PURCHASE_TABLE:
				displayFunc = [&] ( dbListItem* item, const bool bshow ) {
						return MAINWINDOW ()->showBuySearchResult ( item, bshow ); };
			break;
			case INVENTORY_TABLE:
				displayFunc = [&] ( dbListItem* item, const bool bshow ) {
						return COMPANY_PURCHASES ()->showSearchResult ( item, bshow ); };
			break;
			case SUPPLIER_TABLE:
				displayFunc = [&] ( dbListItem* item, const bool bshow ) {
						return SUPPLIERS ()->showSearchResult ( item, bshow ); };
			break;
			default:
			return;
		}
		mFoundItems.setCurrent ( row );
		if ( displayFunc )
			displayFunc ( curItem, true );
		mPreviusItemActivated = static_cast<vmTableItem*>( mFoundList->currentItem () );
	}
}

dbListItem* searchUI::getOtherItem ( const uint typeID, const uint id ) const
{
	DBRecord* dbrec ( nullptr );
	switch ( typeID )
	{
		case SUPPLIES_TABLE:
			dbrec = new dbSupplies; break;
		case INVENTORY_TABLE:
			dbrec = new Inventory; break;
		case SUPPLIER_TABLE:
			dbrec = new supplierRecord; break;
		default:
			return nullptr;
	}
	dbrec->readRecord ( id );
	auto item ( new dbListItem ( typeID ) );
	item->setDBRecID ( id );
	item->setRelation ( RLI_CLIENTITEM );
	item->setDBRec ( dbrec, true );
	return item;
}

void searchUI::getClientInfo ( const clientListItem* const client_item, vmList<QString>& cellData )
{
	cellData[COL_CLIENT-1] = recStrValue ( client_item->clientRecord (), FLD_CLIENT_NAME );
	//cellData[COL_JOB-1] = emptyString;
	cellData[COL_DATE-1] = recStrValue ( client_item->clientRecord (), FLD_CLIENT_STARTDATE );
	for ( uint i ( 0 ); i < CLIENT_FIELD_COUNT; ++i ) {
		if ( client_item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
			cellData[COL_FIELD-1] += VivenciaDB::getTableColumnLabel ( &( client_item->clientRecord ()->t_info ), i ) + CHR_COMMA + CHR_SPACE;
	}
}

bool searchUI::getJobInfo ( jobListItem* job_item, vmList<QString>& cellData )
{
	if ( job_item->loadData ( true ) )
	{
		const clientListItem* client ( static_cast<clientListItem*> ( job_item->relatedItem ( RLI_CLIENTPARENT ) ) );
		if ( client )
		{
			cellData[COL_CLIENT - 1] = recStrValue ( client->clientRecord (), FLD_CLIENT_NAME );
			cellData[COL_JOB - 1] = recStrValue ( job_item->dbRec (), FLD_JOB_ID ) + CHR_HYPHEN + recStrValue ( job_item->dbRec (), FLD_JOB_TYPE );
			job_item->searchSubFields ()->clearButKeepMemory (); //erase (possible) previous search results

			const stringTable str_report ( recStrValue ( job_item->dbRec (), FLD_JOB_REPORT ) );
			int day ( -1 );

			for ( uint i ( 0 ); i < job_item->dbRec ()->fieldCount (); ++i )
			{
				if ( job_item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
				{
					cellData[COL_FIELD - 1] += VivenciaDB::getTableColumnLabel ( job_item->dbRec ()->tableInfo (), i ) + CHR_COMMA + CHR_SPACE;
					if ( i == FLD_JOB_REPORT )
					{
						day = str_report.findRecordRowThatContainsWord ( mSearchTerm, job_item->searchSubFields () );
						if ( day >= 0 )
							cellData[COL_DATE - 1] = str_report.readRecord ( static_cast<uint>(day) ).fieldValue ( Job::JRF_DATE ); // only list one day, the last, for simplicity
					}
				}
			}
			if ( cellData.at ( COL_DATE - 1 ).isEmpty () )
				cellData[COL_DATE - 1] = recStrValue ( job_item->dbRec (), FLD_JOB_STARTDATE );
			return true;
		}
	}
	return false;
}

bool searchUI::getPayInfo ( payListItem* pay_item, vmList<QString>& cellData )
{
	if ( pay_item->loadData ( true ) )
	{
		const clientListItem* client ( static_cast<clientListItem*> ( pay_item->relatedItem ( RLI_CLIENTPARENT ) ) );
		if ( client )
		{
			cellData[COL_CLIENT-1] = recStrValue ( client->clientRecord (), FLD_CLIENT_NAME );

			auto job ( static_cast<jobListItem*>( pay_item->relatedItem ( RLI_JOBPARENT ) ) );
			if ( job != nullptr && job->loadData ( true ) )
			{
				cellData[COL_JOB-1] = recStrValue ( job->jobRecord (), FLD_JOB_ID ) + CHR_HYPHEN + recStrValue ( job->jobRecord (), FLD_JOB_TYPE );
				const stringTable str_payinfo ( recStrValue ( pay_item->dbRec (), FLD_PAY_INFO ) );
				int day ( -1 );

				for ( uint i ( 0 ); i < pay_item->dbRec ()->fieldCount (); ++i )
				{
					if ( pay_item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
					{
						cellData[COL_FIELD-1] += VivenciaDB::getTableColumnLabel ( pay_item->dbRec ()->tableInfo (), i ) + CHR_COMMA + CHR_SPACE;
						if ( i == FLD_PAY_INFO )
						{
							day = str_payinfo.findRecordRowThatContainsWord ( mSearchTerm ); // only first date, for simplicity
							cellData[COL_DATE-1] = str_payinfo.readRecord ( day == -1 ? 0 : static_cast<uint>(day) ).fieldValue ( PHR_DATE );
						}
					}
				}
				return true;
			}
		}
	}
	return false;
}

bool searchUI::getBuyInfo ( buyListItem* buy_item, vmList<QString>& cellData )
{
	if ( buy_item->loadData ( true ) )
	{
		const clientListItem* client ( static_cast<clientListItem*> ( buy_item->relatedItem ( RLI_CLIENTPARENT ) ) );
		if ( client ) {
			cellData[COL_CLIENT-1] = recStrValue ( client->clientRecord (), FLD_CLIENT_NAME );

			auto job ( static_cast<jobListItem*>( buy_item->relatedItem ( RLI_JOBPARENT ) ) );
			if ( job != nullptr && job->loadData ( true ) )
			{
				cellData[COL_JOB-1] = recStrValue ( job->jobRecord (), FLD_JOB_ID ) + CHR_HYPHEN + recStrValue ( job->jobRecord (), FLD_JOB_TYPE );
				const stringTable str_buypayinfo ( recStrValue ( buy_item->dbRec (), FLD_BUY_PAYINFO ) );
				int day ( -1 );

				for ( uint i ( 0 ); i < buy_item->dbRec ()->fieldCount (); ++i )
				{
					if ( buy_item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
						cellData[COL_FIELD-1] += VivenciaDB::getTableColumnLabel ( buy_item->dbRec ()->tableInfo (), i ) + CHR_COMMA + CHR_SPACE;
					if ( i == FLD_BUY_PAYINFO )
					{
						day = str_buypayinfo.findRecordRowThatContainsWord ( mSearchTerm ); // only first date, for simplicity
						cellData[COL_DATE-1] = str_buypayinfo.readRecord ( day == -1 ? 0 : static_cast<uint>(day) ).fieldValue ( PHR_DATE );
					}
				}
				return true;
			}
		}
	}
	return false;
}

bool searchUI::getOtherInfo ( dbListItem* item, vmList<QString>& cellData )
{
	QSqlQuery query;
	switch ( item->subType () )
	{
		case INVENTORY_TABLE:
			( void )VDB ()->runSelectLikeQuery ( QStringLiteral ( "SELECT DATE_IN FROM INVENTORY WHERE ID='" ) +
										QString::number ( item->dbRecID () ) + CHR_CHRMARK, query );
		break;
		case SUPPLIES_TABLE:
			( void )VDB ()->runSelectLikeQuery ( QStringLiteral ( "SELECT DATE_IN FROM SUPPLIES WHERE ID='" ) +
										QString::number ( item->dbRecID () ) + CHR_CHRMARK, query );
		break;
		case SUPPLIER_TABLE:
			cellData[COL_DATE-1] = QStringLiteral ( "N/A" );
		break;
		default:
		break;
	}

	if ( query.isValid () )
	{
		cellData[COL_CLIENT-1] = QStringLiteral ( "N/A" );
		cellData[COL_JOB-1] = QStringLiteral ( "N/A" );
		cellData[COL_DATE-1] = query.value ( 0 ).toString ();

		for ( uint i ( 0 ); i < item->dbRec ()->fieldCount (); ++i )
		{
			if ( item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
				cellData[COL_FIELD-1] += VivenciaDB::getTableColumnLabel ( item->dbRec ()->tableInfo (), i ) + CHR_COMMA + CHR_SPACE;
		}
		return true;
	}
	return false;
}
