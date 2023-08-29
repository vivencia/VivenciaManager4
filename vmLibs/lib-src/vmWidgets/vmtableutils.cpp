#include "vmtableutils.h"
#include "vmtablewidget.h"
#include "vmcheckedtableitem.h"
#include "vmlinefilter.h"

#include <QtWidgets/QHBoxLayout>
#include <QtGui/QShowEvent>
#include <QtGui/QHideEvent>

vmTableSearchPanel::vmTableSearchPanel ( const vmTableWidget* const table )
	: QFrame ( nullptr ), m_SearchedWord ( emptyString ), m_utilidx ( -1 ), m_table ( const_cast<vmTableWidget*>( table ) ),
	  chkSearchAllTable ( nullptr )
{
	auto lblSearch ( new QLabel ( TR_FUNC ( "Search:" ) ) );
	txtSearchTerm = new vmLineEdit;
	txtSearchTerm->setEditable ( true );
	static_cast<void>( connect ( txtSearchTerm, &QLineEdit::textEdited, this, [&] ( const QString& text ) { btnSearchStart->setEnabled ( text.length () >= 2 ); } ) );
	txtSearchTerm->setCallbackForRelevantKeyPressed ( ( [&]( const QKeyEvent* ke, const vmWidget* const ) {
		return txtSearchTerm_keyPressed ( ke ); } ) );

	btnSearchStart = new QToolButton;
	btnSearchStart->setIcon ( ICON ( "search" ) );
	static_cast<void>( connect ( btnSearchStart, &QToolButton::clicked, this, [&] () { return searchStart (); } ) );

	btnSearchPrev = new QToolButton;
	btnSearchPrev->setIcon ( ICON ( "arrow-left" ) );
	btnSearchPrev->setEnabled ( false );
	static_cast<void>( connect ( btnSearchPrev, &QToolButton::clicked, this, [&] () { return searchPrev (); } ) );

	btnSearchNext = new QToolButton;
	btnSearchNext->setIcon ( ICON ( "arrow-right" ) );
	btnSearchNext->setEnabled ( false );
	static_cast<void>( connect ( btnSearchNext, &QToolButton::clicked, this, [&] () { return searchNext (); } ) );

	btnSearchCancel = new QToolButton;
	btnSearchCancel->setIcon ( ICON ( "cancel" ) );
	static_cast<void>( connect ( btnSearchCancel, &QToolButton::clicked, this, [&] () { return hide (); } ) );

	auto mLayout ( new QHBoxLayout );
	mLayout->setSpacing( 2 );
	mLayout->setMargin ( 2 );
	mLayout->addWidget ( lblSearch );
	mLayout->addWidget ( txtSearchTerm, 3 );
	mLayout->addWidget ( btnSearchStart );
	mLayout->addWidget ( btnSearchPrev );
	mLayout->addWidget ( btnSearchNext );
	mLayout->addWidget ( btnSearchCancel );
	
	if ( !m_table->isList () )
	{
		chkSearchAllTable = new vmCheckBox ( tr ( "Search all fields" ) );
		chkSearchAllTable->setChecked ( true );
		chkSearchAllTable->setCallbackForContentsAltered ( [&] ( const vmWidget* const ) { return searchFieldsChanged (); } );
		chkSearchAllTable->setEditable ( true );
		mLayout->addStretch ( 2 );
		mLayout->addWidget ( chkSearchAllTable, 0, Qt::AlignRight );
	}
	setLayout ( mLayout );
	setMaximumHeight ( 30 );
}

void vmTableSearchPanel::showEvent ( QShowEvent* se )
{
	se->setAccepted ( false );
	QFrame::showEvent ( se );
	txtSearchTerm->setFocus ();
}

void vmTableSearchPanel::hideEvent ( QHideEvent* he )
{
	he->setAccepted ( false );
	QFrame::hideEvent ( he );
	m_table->searchCancel ();
	btnSearchNext->setEnabled ( false );
	btnSearchPrev->setEnabled ( false );
	txtSearchTerm->clear ();
	m_table->setFocus ();
}

void vmTableSearchPanel::searchFieldsChanged ( const vmCheckBox* const )
{
	if ( chkSearchAllTable && chkSearchAllTable->isChecked () )
		dynamic_cast<vmCheckedTableItem*>( m_table->horizontalHeader () )->setCheckable ( false );
	else
	{
		dynamic_cast<vmCheckedTableItem*>( m_table->horizontalHeader () )->setCheckable ( true );
		for ( uint col ( 0 ); col < m_table->colCount (); ++col )
			dynamic_cast<vmCheckedTableItem*>( m_table->horizontalHeader () )->setChecked ( col, false );
	}
}

bool vmTableSearchPanel::searchStart ()
{
	m_table->searchStart ( txtSearchTerm->text () );
	const bool b_result ( m_table->searchFirst () );
	btnSearchStart->setEnabled ( !b_result );
	btnSearchNext->setEnabled ( b_result );
	btnSearchPrev->setEnabled ( !b_result );
	return b_result;
}

bool vmTableSearchPanel::searchNext ()
{
	const bool b_result ( m_table->searchNext () );
	btnSearchNext->setEnabled ( b_result );
	btnSearchPrev->setEnabled ( btnSearchPrev->isEnabled () | b_result );
	return b_result;
}

bool vmTableSearchPanel::searchPrev ()
{
	const bool b_result ( m_table->searchPrev () );
	btnSearchNext->setEnabled ( btnSearchNext->isEnabled () | b_result );
	btnSearchPrev->setEnabled ( b_result );
	return b_result;
}

void vmTableSearchPanel::txtSearchTerm_keyPressed ( const QKeyEvent* ke )
{
	switch ( ke->key () )
	{
		case Qt::Key_Enter:
		case Qt::Key_Return:
			searchStart ();
		break;
		case Qt::Key_Escape:
			if ( txtSearchTerm->text () != m_table->searchTerm () )
			{
				txtSearchTerm->setText ( m_table->searchTerm () );
				btnSearchStart->setEnabled ( !txtSearchTerm->text ().isEmpty () );
			}
			else
			{
				hide ();
				m_table->setFocus ();
			}
		break;
		case Qt::Key_F2:
			searchPrev ();
		break;
		case Qt::Key_F3:
			searchNext ();
		break;
		default:
		break;
	}
}

vmTableFilterPanel::vmTableFilterPanel ( const vmTableWidget * const table )
	: QFrame ( nullptr ), m_utilidx ( -1 ), m_table ( const_cast<vmTableWidget*>( table ) ),
		searchLevels ( 10 ), newView_func ( nullptr )
{
	searchLevels.setAutoDeleteItem ( true );
	
	m_btnClose = new QToolButton;
	m_btnClose->setIcon ( ICON ( "cancel" ) );
	connect ( m_btnClose, &QToolButton::clicked, this, [&] () { return hide (); } );

	auto lblFilter ( new QLabel ( TR_FUNC ( "Filter:" ) ) );
	m_txtFilter = new vmLineFilter;
	m_txtFilter->setEditable ( true );
	m_txtFilter->setCallbackForValidKeyEntered ( [&] ( const triStateType level, const int startlevel ) { return doFilter ( level, startlevel ); } );

	auto mainLayout ( new QHBoxLayout );
	mainLayout->setMargin ( 2 );
	mainLayout->setSpacing ( 2 );
	mainLayout->addWidget ( lblFilter );
	mainLayout->addWidget ( m_txtFilter, 2 );
	mainLayout->addWidget ( m_btnClose );
	setLayout ( mainLayout );

	setMaximumHeight ( 30 );
}

void vmTableFilterPanel::showEvent ( QShowEvent* se )
{
	se->setAccepted ( true );
	QFrame::showEvent ( se );
	m_txtFilter->setFocus ();
}

void vmTableFilterPanel::hideEvent ( QHideEvent* he )
{
	he->setAccepted ( true );
	QFrame::hideEvent ( he );
	m_txtFilter->textCleared ();
	m_table->setFocus ();
}

void vmTableFilterPanel::doFilter ( const triStateType& level, const int startlevel )
{
	if ( level == CLEAR_LEVEL )
	{
		for ( uint i_row ( 0 ); static_cast<int>(i_row) <= m_table->lastUsedRow (); ++i_row )
			m_table->setRowVisible ( i_row, true );
		searchLevels.clearButKeepMemory ();
	}
	else
	{
		if ( startlevel < static_cast<int>(searchLevels.count ()) )
		{
			for ( int i ( startlevel ); i < static_cast<int>(searchLevels.count ()); ++i )
				searchLevels.remove ( i, true );
			podList<int>* rowLevel ( searchLevels.at ( startlevel - 1 ) );
			if ( rowLevel )
			{
				for ( uint i ( 0 ); i < rowLevel->count (); ++i )
					m_table->setRowVisible ( static_cast<uint>(rowLevel->at ( i )), true );	
			}
		}
		if ( level == ADD_LEVEL )
		{
			bool b_keeprow ( false );
			uint i_row ( 0 );
			auto newRowLevel ( new podList<int> );
			newRowLevel->setPreAllocNumber ( m_table->visibleRows () );
			const uint max_cols ( m_table->isList () ? 1 : m_table->colCount () );
			
			do
			{
				if ( !m_table->isRowHidden ( static_cast<int>(i_row) ) )
				{
					b_keeprow = false;
					for ( uint i_col ( 0 ); i_col < max_cols; ++i_col )
					{
						if ( m_txtFilter->matches ( m_table->sheetItem ( i_row, i_col )->text () ) )
						{
							b_keeprow = true;
							break;
						}
					}
					m_table->setRowVisible ( i_row, b_keeprow );
					if ( b_keeprow )
						newRowLevel->append ( static_cast<int>(i_row) );
				}
			} while ( static_cast<int>(++i_row) <= m_table->lastUsedRow () );
			searchLevels.append ( newRowLevel );
		}
	}
	m_table->scrollToItem ( m_table->currentItem (), QAbstractItemView::PositionAtCenter );
	if ( newView_func )
		newView_func ();
}
