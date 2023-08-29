#include "dbtableview.h"
#include "system_init.h"
#include "mainwindow.h"

#include <heapmanager.h>
#include <dbRecords/vivenciadb.h>
#include <vmWidgets/vmlistwidget.h>
#include <vmWidgets/vmlistitem.h>
#include <vmWidgets/vmtablewidget.h>
#include <vmNotify/vmnotify.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QKeyEvent>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QApplication>

static const QString information_schema_tables[] = {
	"TABLE_CATALOG", "TABLE_SCHEMA",
	"TABLE_NAME", "TABLE_TYPE", "ENGINE", "VERSION", "ROW_FORMAT", "TABLE_ROWS", "AVG_ROW_LENGTH",
	"DATA_LENGTH", "MAX_DATA_LENGTH", "INDEX_LENGTH", "DATA_FREE", "AUTO_INCREMENT", "CREATE_TIME",
	"UPDATE_TIME", "CHECK_TIME", "TABLE_COLLATION", "CHECKSUM", "CREATE_OPTIONS", "TABLE_COMMENT"
};

static RECORD_ACTION getQueryAction ( const QString& query )
{
	if ( query.startsWith ( QStringLiteral ( "UPDATE " ) ) )
		return ACTION_EDIT;
	if ( query.startsWith ( QStringLiteral ( "INSERT INTO " ) ) )
		return ACTION_ADD;
	if ( query.startsWith ( QStringLiteral ( "DELETE FROM " ) ) )
		return ACTION_DEL;
	return ACTION_NONE;
}

static const QString query_key_words ( QStringLiteral ( "SELECT UPDATE INSERT INTO VALUES SET WHERE DELETE FROM ID" ) );

static void getTableNameFromQuery ( const QString& query, QString& table_name )
{
	const QStringList words ( query.split ( CHR_SPACE, Qt::SkipEmptyParts ) );
	const QStringList::const_iterator& itr_end ( words.constEnd () );
	QStringList::const_iterator itr ( words.constBegin () );
	
	QSqlQuery queryRes;
	static QString tables;
	
	if ( tables.isEmpty () )
	{
		if ( VDB ()->runSelectLikeQuery ( QStringLiteral ( "SHOW TABLES" ), queryRes ) )
		{
			do
			{
				tables += queryRes.value ( 0 ).toString () + CHR_SPACE;
			} while ( queryRes.next () );
		}
	}

	for ( ; itr != itr_end; ++itr )
	{
		if ( (*itr).size () > 3 )
		{
			if ( !query_key_words.contains ( *itr, Qt::CaseInsensitive ) )
			{
				if ( tables.contains ( *(itr) ) )
				{
					table_name = *(itr);
					return;
				}
			}
		}
	}
}

static uint getRecordIdFromQuery ( const QString& query )
{
	int idx ( -1 );
	int idx_shift ( -1 );
	
	switch ( getQueryAction ( query ) )
	{
		case ACTION_EDIT:
		case ACTION_DEL:
			idx = query.indexOf ( QStringLiteral ( "WHERE ID=" ) );
			idx_shift = 10;
		break;
		case ACTION_ADD:
			idx = query.indexOf ( QStringLiteral ( "VALUES (" ) );
			idx_shift = 9;
		break;
		case ACTION_NONE:
		case ACTION_READ:
		case ACTION_REVERT:
		break;
	}

	if ( idx > 0 )
	{
		const int len ( query.size () );
		int chr ( idx + idx_shift );
		QString id;
		while ( chr < len )
		{
			if ( query.at ( chr ).isDigit () )
				id += query.at ( chr++ );
			else if ( (query.at ( chr ).isSpace ()) || (query.at ( chr ) == CHR_CHRMARK) )
				++chr;
			else
				break;
		} 
		return id.toUInt ();
	}
	return 0;
}

static TABLE_IDS tableNameToTableId ( const QString& table_name )
{
	if ( table_name.startsWith ( QLatin1Char ( 'C' ) ) )
	{
		if ( table_name.at ( 1 ) == QLatin1Char ( 'O' ) )
			return table_name.at ( 4 ) == QLatin1Char ( 'A' ) ? COMPANY_PURCHASES_TABLE : COMPLETER_RECORDS_TABLE;
		else
			return table_name.at ( 1 ) == QLatin1Char ( 'L' ) ? CLIENT_TABLE : CALENDAR_TABLE;
	}
	else if ( table_name.startsWith ( QLatin1Char ( 'P' ) ) )
	{
		return table_name.at ( 1 ) == QLatin1Char ( 'A' ) ? PAYMENT_TABLE : PURCHASE_TABLE;
	}
	else if ( table_name.startsWith ( QLatin1Char ( 'S' ) ) )
	{
		return table_name.at ( 7 ) == QLatin1Char ( 'S' ) ? SUPPLIES_TABLE: SUPPLIER_TABLE;
	}
	else
	{
		if ( table_name.startsWith ( QLatin1Char ( 'J' ) ) )
			return JOB_TABLE;
		if ( table_name.startsWith ( QLatin1Char ( 'I' ) ) )
			return INVENTORY_TABLE;
		if ( table_name.startsWith ( QLatin1Char ( 'M' ) ) )
			return MACHINES_TABLE;
		if ( table_name.startsWith ( QLatin1Char ( 'G' ) ) )
			return GENERAL_TABLE;
		if ( table_name.startsWith ( QLatin1Char ( 'Q' ) ) )
			return QUICK_PROJECT_TABLE;
		return USERS_TABLE;
	}
}

dbTableView::dbTableView ()
	: QObject ( nullptr )
{
	mTablesList = new vmListWidget;
	mTablesList->setMaximumWidth ( 300 );
	mTablesList->setCallbackForCurrentItemChanged ( [&] ( vmListItem* item ) { return showTable ( mTablesList->item ( item->row () )->text () ); } );

	auto actionReload ( new vmAction ( 0 ) );
	actionReload->setShortcut ( QKeySequence ( Qt::Key_F5 ) );
	static_cast<void>( connect ( actionReload, &QAction::triggered, this, [&] ( const bool ) { return reload (); } ) );	
	mTablesList->addAction ( actionReload );
	
	auto vLayout ( new QVBoxLayout );
	vLayout->addWidget ( new QLabel ( TR_FUNC ( "Available tables in the database:" ) ) );
	vLayout->addWidget ( mTablesList, 3 );
	
	mLeftFrame = new QFrame;
	mLeftFrame->setLayout ( vLayout );
	
	mTabView = new QTabWidget;
	mTabView->setTabsClosable ( true );
	mTabView->connect ( mTabView, &QTabWidget::tabCloseRequested, mTabView, [&] ( const int idx ) { mTabView->removeTab ( idx ); } );
	
	vmAction* actionCloseTab ( new vmAction ( 1 ) );
	actionCloseTab->setShortcut ( QKeySequence ( "Ctrl+W" ) );
	static_cast<void>( connect ( actionCloseTab, &QAction::triggered, this, [&] ( const bool ) { mTabView->removeTab ( mTabView->currentIndex () ); } ) );	
	mTablesList->addAction ( actionCloseTab );
	
	mMainLayoutSplitter = new QSplitter;
	mMainLayoutSplitter->insertWidget ( 0, mLeftFrame );
	mMainLayoutSplitter->insertWidget ( 1, mTabView );
	
	const int screen_width ( QGuiApplication::primaryScreen ()->availableGeometry ().width () );
	mMainLayoutSplitter->setSizes ( QList<int> () << static_cast<int>( screen_width / 5 ) << static_cast<int>( 4 * screen_width / 5 ) );

	mTxtQuery = new vmLineEdit;
	static_cast<void>( connect ( mTxtQuery, &QLineEdit::textChanged, this, [&] ( const QString& text ) { mBtnRunQuery->setEnabled ( text.length () > 10 ); } ) );
	mTxtQuery->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) { maybeRunQuery ( ke ); } );
	mTxtQuery->setEditable ( true );
	mBtnRunQuery = new QToolButton;
	mBtnRunQuery->setIcon ( ICON ( "arrow-right.png" ) );
	mBtnRunQuery->setEnabled ( false );
	static_cast<void>( connect ( mBtnRunQuery, &QToolButton::clicked, this, [&] () { runPersonalQuery (); } ) );
	auto hLayout ( new QHBoxLayout );
	hLayout->addWidget ( new QLabel ( TR_FUNC ( "Execute custom SQL query: " ) ), 1 );
	hLayout->addWidget ( mTxtQuery, 5 );
	hLayout->addWidget ( mBtnRunQuery, 0 );
	
	mMainLayout = new QVBoxLayout;
	mMainLayout->addLayout ( hLayout, 0 );
	mMainLayout->addWidget ( mMainLayoutSplitter, 2 );
}

dbTableView::~dbTableView () {}

void dbTableView::reload ()
{
	mTablesList->setIgnoreChanges ( true );
	loadTablesIntoList ();
	mTablesList->setIgnoreChanges ( false );
}

void dbTableView::loadTablesIntoList ()
{
	mTablesList->clear ( true, true );
	QSqlQuery queryRes;
	if ( VDB ()->runSelectLikeQuery ( QStringLiteral ( "SHOW TABLES" ), queryRes ) )
	{
		do
		{
			mTablesList->addItem ( new vmListItem ( queryRes.value ( 0 ).toString () ) );
		} while ( queryRes.next () );
	}
	for ( int i ( 0 ); i < mTabView->count (); ++i )
	{
		auto widget ( static_cast<tableViewWidget*>( mTabView->widget ( i ) ) );
		if ( widget && widget->isVisible () )
			widget->reload ();
	}
}

void dbTableView::showTable ( const QString& tablename )
{
	bool b_widgetpresent ( false );
	for ( int i ( 0 ); i < mTabView->count (); ++i )
	{
		auto widget ( static_cast<tableViewWidget*>( mTabView->widget ( i ) ) );
		if ( widget )
		{
			if ( widget->tableName () == tablename )
			{
				if ( widget->isVisible () )
				{
					widget->reload ();
					mTabView->setCurrentWidget ( widget );
					widget->show ();
				}
				else
				{
					mTabView->setCurrentIndex ( mTabView->indexOf ( widget ) );
				}
				b_widgetpresent = true;
			}
		}
	}
	if ( !b_widgetpresent )
	{
		auto widget ( new tableViewWidget ( tablename ) );
		mTabView->setCurrentIndex ( mTabView->addTab ( widget, tablename ) );
		widget->setFocus ();
	}
}

void dbTableView::runPersonalQuery ()
{
	QSqlQuery query;
	
	const bool b_query_is_select ( mTxtQuery->text ().startsWith ( QStringLiteral ( "SELECT " ), Qt::CaseInsensitive ) );
	bool b_ok ( false );
	
	if ( b_query_is_select )
		b_ok = VDB ()->runSelectLikeQuery ( mTxtQuery->text (), query );
	else
		b_ok = VDB ()->runModifyingQuery ( mTxtQuery->text (), query );
			
	if ( !b_ok )		
	{
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Error!" ), query.lastError ().text () );
		mTxtQuery->setFocus ();
		return;
	}
	
	tableViewWidget* widget ( nullptr );
	if ( b_query_is_select )
	{
		widget = new tableViewWidget ( &query );
	}
	else
	{
		QString tablename;
		getTableNameFromQuery ( query.lastQuery (), tablename );
		widget = new tableViewWidget ( tablename );
		notifyMainWindowOfChanges ( query.lastQuery () );
	}
	
	mTabView->setCurrentIndex ( mTabView->addTab ( widget, mTxtQuery->text () ) );
	widget->setFocus ();
	mTxtQuery->clear ();
	mBtnRunQuery->setEnabled ( false );
}

void dbTableView::maybeRunQuery ( const QKeyEvent* const ke )
{
	if ( ke->key () == Qt::Key_Escape )
	{
		mTxtQuery->clear ();
		mBtnRunQuery->setEnabled ( false );
	}
	else if ( ke->key () == Qt::Key_Enter || ke->key () == Qt::Key_Return )
	{
		if ( mTxtQuery->text ().length () > 10 )
			runPersonalQuery ();
	}
}

void dbTableView::notifyMainWindowOfChanges ( const QString& query )
{
	QString table_name;
	getTableNameFromQuery ( query, table_name );
	if ( !table_name.isEmpty () )
	{	
		MAINWINDOW ()->notifyExternalChange ( 
					getRecordIdFromQuery ( query ), 
					tableNameToTableId ( table_name ),
					getQueryAction ( query )
		);
	}
}

tableViewWidget::tableViewWidget ( const QString& tablename )
	: QFrame ( nullptr ), m_table ( nullptr ), m_cols ( QString (), 10 ), m_tablename ( tablename ),
	  mQuery ( nullptr ), m_nrows ( 0 ), mb_loaded ( false )
{
	mLayout = new QVBoxLayout;
	setLayout ( mLayout );
	createTable ();
	showTable ();
}

tableViewWidget::tableViewWidget ( QSqlQuery* const query )
	: QFrame ( nullptr ), m_table ( nullptr ), m_cols ( QString (), 10 ),
	  mQuery ( query ), m_nrows ( 0 ), mb_loaded ( false )
{
	mLayout = new QVBoxLayout;
	setLayout ( mLayout );
	createTable ();
	showTable ();
}

void tableViewWidget::showTable ()
{
	if ( !mb_loaded )
		load ();
	show ();
}

void tableViewWidget::load ()
{
	QSqlQuery query;
	if ( mQuery == nullptr )
	{
		QString str_tables;
		for ( uint i ( 0 ); i < m_cols.count (); ++i )
			str_tables += m_cols.at ( i ) + CHR_COMMA;
		str_tables.chop ( 1 );
		
		/* From the Qt's doc: Using SELECT * is not recommended because the order of the fields in the query is undefined.*/
		if ( !VDB ()->runSelectLikeQuery ( QStringLiteral ( "SELECT " ) + str_tables + QStringLiteral ( " FROM " ) + tableName (), query ) )
			return;
	}
	else
		query = *mQuery;

	uint row ( 0 );
	uint col ( 0 );

	m_table->setIgnoreChanges ( true );
	do {
		do {
			m_table->sheetItem ( row, col )->setText ( query.value ( static_cast<int>( col ) ).toString (), false );
		} while ( ++col < m_cols.count () );
		col = 0;
		++row;
	} while ( query.next () );
	m_table->setIgnoreChanges ( false );

	getTableLastUpdate ( m_updatedate, m_updatetime ); // save the last update time to avoid unnecessary reloads
	mb_loaded = true;
}

void tableViewWidget::reload ()
{
	bool b_doreload ( false );
	
	vmNumber update_date, update_time;
	getTableLastUpdate ( update_date, update_time );
	if ( update_date <= m_updatedate )
	{
		if ( update_time > m_updatetime )
			b_doreload = true;
	}
	else
		b_doreload = true;
	
	if ( b_doreload )
	{
		//Reload column information
		vmList<QString> old_cols ( m_cols );
		getTableInfo ();
		mLayout->removeWidget ( m_table );
		delete m_table;
		createTable ();

		for ( uint i ( 0 ); i < m_cols.count (); ++i )
		{
			if ( m_cols.at ( i ) != old_cols.at ( i ) )
				m_table->horizontalHeaderItem ( static_cast<int>( i ) )->setText ( m_cols.at ( i ) );
		}
		
		getTableLastUpdate ( m_updatedate, m_updatetime ); // save the last update time to avoid unnecessary reloads
		
		if ( m_nrows > static_cast<uint>( m_table->rowCount () ) )
			m_table->insertRow ( static_cast<uint>( m_table->rowCount () ), m_nrows - static_cast<uint>( m_table->rowCount () ) + 1 );
		load ();
	}
}

void tableViewWidget::focusInEvent ( QFocusEvent* e )
{
	QFrame::focusInEvent ( e );
	m_table->setFocus ();
}

void tableViewWidget::createTable ()
{
	getTableInfo ();
	
	m_table = new vmTableWidget;
	vmTableColumn* columns ( m_table->createColumns ( m_cols.count () ) );
	for ( uint i ( 0 ); i < m_cols.count (); ++i )
	{
		columns[i].label = m_cols.at ( i );
	}
	m_table->setIsPlainTable ( false );
	m_table->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) { return updateTable ( item ); } );
	m_table->setPlainTableEditable ( true );
	m_table->initTable ( m_nrows );
	m_table->setMinimumWidth ( 800 );
	m_table->setColumnsAutoResize ( true );
	m_table->addToLayout ( mLayout, 3 );
	m_table->setCallbackForRowInserted ( [&] ( const uint row ) { return rowInserted ( row ); } );
	m_table->setCallbackForRowRemoved ( [&] ( const uint row ) { return rowRemoved ( row ); } );
	
	auto actionReload ( new vmAction ( 0 ) );
	actionReload->setShortcut ( QKeySequence ( Qt::Key_F5 ) );
	static_cast<void>( connect ( actionReload, &QAction::triggered, this, [&] ( const bool ) { return reload (); } ) );	
	addAction ( actionReload );
	//m_table->setEditable ( true );
}

void tableViewWidget::getTableInfo ()
{
	m_cols.clearButKeepMemory ();
	if ( mQuery == nullptr )
	{
		QSqlQuery query;
		if ( VDB ()->runSelectLikeQuery ( QStringLiteral ( "DESCRIBE " ) + tableName (), query ) )
		{
			do {
				m_cols.append ( query.value ( 0 ).toString () );
			} while ( query.next () );
		}
		query.clear ();
		if ( VDB ()->runSelectLikeQuery ( QStringLiteral ( "SELECT " ) + information_schema_tables[7] + QStringLiteral ( " FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME LIKE '" ) + tableName () + QLatin1Char ( '\'' ), query ) )
			m_nrows = query.value ( 0 ).toUInt ();
	}
	else
	{
		if ( mQuery->size () > 0 )
		{
			const QSqlRecord rec ( mQuery->record () );
		
			const int nCols ( rec.count () );
			for ( int i_col ( 0 ); i_col < nCols; ++i_col )
				m_cols.append ( rec.fieldName ( i_col ) );
		
			m_nrows = static_cast<uint>( mQuery->size () );
			getTableNameFromQuery ( mQuery->lastQuery (), m_tablename );
		}
	}
}

void tableViewWidget::getTableLastUpdate ( vmNumber& date, vmNumber& time )
{
	QSqlQuery query;
	if ( VDB ()->runSelectLikeQuery ( QStringLiteral ( "SELECT " ) + information_schema_tables[15] + QStringLiteral ( " FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME LIKE '" ) + tableName () + QLatin1Char ( '\'' ), query ) )
	{
		const QString query_res ( query.value ( 0 ).toString () );
		date.dateFromMySQLDate ( query_res.left ( 10 ) );
		time.fromTrustedStrTime ( query_res.right ( 8 ), vmNumber::VTF_24_HOUR, true );
	}
}

void tableViewWidget::updateTable ( const vmTableItem* const item )
		
{
	const QString record_id ( m_table->sheetItem ( static_cast<uint>( item->row () ), 0 )->text () );
	const auto field ( static_cast<uint>( item->column () ) );
	QSqlQuery query;
	if ( VDB ()->runModifyingQuery ( QStringLiteral ( "UPDATE " ) + tableName () + QStringLiteral ( " SET " ) +
							m_cols.at ( field ) + QStringLiteral ( "=\'" ) + item->text () + QStringLiteral ( "\' WHERE ID=" )
							+ record_id, query ) )
	{
		getTableLastUpdate ( m_updatedate, m_updatetime ); // save the last update time to avoid unnecessary reloads
		
		NOTIFY ()->notifyMessage ( TR_FUNC ( "Warning!! Table %1 updated" ).arg ( tableName () ),
									  TR_FUNC ( "The record id (%1) was updated from the old value (%2) to the new (%3)" ).arg ( 
										  record_id, item->prevText (), item->text () ) );
		
		MAINWINDOW ()->notifyExternalChange ( record_id.toUInt (), tableNameToTableId ( m_tablename ), ACTION_EDIT );
	}
}

void tableViewWidget::rowInserted ( const uint row )
{
	if ( row > 0 )
	{
		QSqlQuery query;
		const QString new_id ( QString::number ( m_table->sheetItem ( row - 1, 0 )->text ().toUInt () + 1 ) );
		
		if ( VDB ()->runModifyingQuery ( QStringLiteral ( "INSERT INTO " ) + tableName () + QStringLiteral ( " VALUES ( " ) + new_id + QStringLiteral ( " )" ), query ) )
		{
			getTableLastUpdate ( m_updatedate, m_updatetime ); // save the last update time to avoid unnecessary reloads
			m_table->setIgnoreChanges ( true );
			m_table->sheetItem ( row, 0 )->setText ( new_id, false );
			m_table->setIgnoreChanges ( false );
			 
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Warning!! Table %1 updated" ).arg ( tableName () ),
										  TR_FUNC ( "A new record (with ID=%1) was appended to the table" ).arg ( new_id ) );
			
			MAINWINDOW ()->notifyExternalChange ( new_id.toUInt (), tableNameToTableId ( m_tablename ), ACTION_ADD );
		}
	}
}

bool tableViewWidget::rowRemoved ( const uint row )
{
	if ( NOTIFY ()->questionBox ( TR_FUNC ( "Delete record?" ), TR_FUNC ( "Are you sure you want to remove the record " ) + QString::number ( row + 1 ) +
																				QStringLiteral ( " (ID " ) + m_table->sheetItem ( row, 0 )->text () + QStringLiteral ( ") ?" ) ) )
	{
		QSqlQuery query;
		const QString id ( m_table->sheetItem ( row, 0 )->text () );
		
		if ( VDB ()->runModifyingQuery ( QStringLiteral ( "DELETE FROM " ) + tableName () + QStringLiteral ( " WHERE ID='" ) + id + CHR_CHRMARK, query ) )
		{
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Warning!! Table %1 updated" ).arg ( tableName () ),
										  TR_FUNC ( "The record with ID=%1 was removed from the table" ).arg ( id ) );
			
			MAINWINDOW ()->notifyExternalChange ( id.toUInt (), tableNameToTableId ( m_tablename ), ACTION_DEL );
			return true;
		}
	}
	return false;
}
