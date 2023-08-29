#include "fixdatabaseui.h"
#include "heapmanager.h"
#include "vmlibs.h"
#include "vivenciadb.h"

#include <vmWidgets/vmtablewidget.h>

#include <QtGui/QIcon>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>

static QIcon* dbTableStateIcon ( const fixDatabase::CheckResult result )
{
	switch ( result )
	{
		case fixDatabase::CR_UNDEFINED:
		{
			static QIcon* res_undef ( nullptr );
			if ( !res_undef )
				res_undef = new QIcon ( ICON ( "notify-1" ) );
			return res_undef;
		}
		
		case fixDatabase::CR_OK:
		{
			static QIcon* res_ok ( nullptr );
			if ( !res_ok )
				res_ok = new QIcon ( ICON ( "db-ok" ) );
			return res_ok;
		}
		
		default:
		{
			static QIcon* res_err ( nullptr );
			if ( !res_err )
				res_err = new QIcon ( ICON ( "db-error" ) );
			return res_err;
		}
	}
}

fixDatabaseUI::fixDatabaseUI ()
	: QDialog ( nullptr ), tablesView ( nullptr ), m_fdb ( nullptr ), m_tables ( 5 )
{
	setModal ( true );
}

fixDatabaseUI::~fixDatabaseUI ()
{
	tablesView->clear ();
	heap_del ( m_fdb );
}

void fixDatabaseUI::showWindow ()
{
	setupUI ();
	btnCheck->setEnabled ( true );
	btnFix->setEnabled ( false );
	exec ();
}

void fixDatabaseUI::setupUI ()
{
	if ( tablesView )
		return;

	tablesView = new vmTableWidget ( this );
	vmTableColumn* cols ( tablesView->createColumns ( 4 ) );
	cols[0].label = tr ( "Table" );
	cols[1].label = tr ( "Status" );
	cols[2].label = tr ( "Error message" );
	cols[3].label = tr ( "Fix ?" );
	cols[3].wtype = WT_CHECKBOX;

	tablesView->setEditable ( false );
	tablesView->initTable ( TABLES_IN_DB );
	populateTable ();

	btnCheck = new QPushButton ( tr ( "Check integrity" ) );
	connect ( btnCheck,	&QPushButton::clicked, this, [&] () { return doCheck (); } );

	btnFix = new QPushButton ( tr ( "Fix errors" ) );
	connect ( btnFix, &QPushButton::clicked, this, [&] () { return doFix (); } );

	btnClose = new QPushButton ( tr ( "Close" ) );
	connect ( btnClose, &QPushButton::clicked, this, [&] () { return close (); } );

	auto btnsLayout = new QHBoxLayout;
	btnsLayout->insertWidget ( 0, btnCheck, 1 );
	btnsLayout->insertWidget ( 1, btnFix, 1 );
	btnsLayout->insertWidget ( 2, btnClose, 1 );

	auto mainLayout = new QVBoxLayout;
	mainLayout->insertWidget ( 0, tablesView, 2 );
	mainLayout->insertLayout ( 1, btnsLayout, 1 );
	setLayout ( mainLayout );

	setWindowTitle ( tr ( "Check and fix database" ) );
}

void fixDatabaseUI::populateTable ()
{
	auto s_row ( new spreadRow );
	s_row->column[0] = 0;
	s_row->column[1] = 1;
	s_row->column[2] = 2;
	s_row->column[3] = 3;
	for ( uint i ( 0 ); i <= TABLES_IN_DB; ++i )
	{
		s_row->row = static_cast<int>( i );
		s_row->field_value[0] = VivenciaDB::tableInfo ( i - 1 )->table_name;
		s_row->field_value[1] = tr ( "Unchecked" );
		s_row->field_value[2] = tr ( "Unknown" );
		s_row->field_value[3] = CHR_ZERO;
		tablesView->setRowData ( s_row );
		tablesView->sheetItem ( i, 1 )->setData ( Qt::DecorationRole, QVariant ( *dbTableStateIcon ( fixDatabase::CR_UNDEFINED ) ) );
		tablesView->sheetItem ( i, 1 )->setData ( Qt::WhatsThisRole, QVariant ( fixDatabase::CR_UNDEFINED ) );
	}
}

void fixDatabaseUI::doCheck ()
{
	if ( !m_fdb )
		m_fdb = new fixDatabase;
	if ( m_fdb->checkDatabase () )
	{
		m_fdb->badTables ( m_tables );
		for ( uint i_row ( 1 ); i_row <= unsigned ( tablesView->rowCount () ); ++i_row )
		{
			for ( uint i ( 0 ); i < m_tables.count (); ++i )
			{
				if ( tablesView->sheetItem ( i_row, 0 )->text () == m_tables.at ( i )->table )
				{
					tablesView->sheetItem ( i, 1 )->setData ( Qt::DecorationRole, QVariant ( *dbTableStateIcon ( m_tables.at ( i )->result ) ) );
					tablesView->sheetItem ( i, 1 )->setData ( Qt::WhatsThisRole, QVariant ( m_tables.at ( i )->result ) );
					tablesView->sheetItem ( i_row, 2 )->setText ( m_tables.at ( i )->err, false, false );
					tablesView->sheetItem ( i_row, 3 )->setText ( m_tables.at ( i )->result >= fixDatabase::CR_TABLE_CORRUPTED ? CHR_ONE : CHR_ZERO, false, false );
				}
			}
		}
		btnFix->setEnabled ( m_fdb->needsFixing () );
	}
}

void fixDatabaseUI::doFix ()
{
	if ( tablesView->tableChanged () )
		m_fdb->fixTables ();
	else {
		for ( uint i ( 0 ); i < unsigned ( tablesView->rowCount () ); ++i )
		{
			if ( tablesView->sheetItem ( i, 3 )->data ( Qt::WhatsThisRole ).toInt () >= fixDatabase::CR_TABLE_CORRUPTED )
				m_fdb->fixTables ( tablesView->item ( static_cast<int>( i ), 0 )->text () );
		}
		tablesView->setTableUpdated ();
	}
	btnFix->setEnabled ( false );
}
