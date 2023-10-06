#include "vmlistwidget.h"
#include "vmlistitem.h"

#include <QtGui/QResizeEvent>

vmListWidget::vmListWidget ( QWidget* parent, const uint nRows )
	: vmTableWidget ( parent ), mbIgnore ( true ), mbDestroyDelete ( false ), mbForceEmit ( false ), 
	  mPrevRow ( -2 ), mCurrentItem ( nullptr ), mPrevItem ( nullptr ), mCurrentItemChangedFunc ( nullptr ),
	  mGotFocusFunc ( nullptr )
{
	setIsList ();
	static_cast<void>( createColumns ( 1 ) );
	initTable ( nRows );
	//setIgnoreChanges ( false );
	setHorizontalScrollMode ( QAbstractItemView::ScrollPerPixel );
}

vmListWidget::~vmListWidget ()
{
	this->vmListWidget::clear ( true, isDeleteItemsWhenDestroyed () );
}

void vmListWidget::setIgnoreChanges ( const bool b_ignore )
{
	rowClickedConnection ( !(mbIgnore = b_ignore) );
	if ( !b_ignore )
		static_cast<void>( connect ( this, &QTableWidget::currentCellChanged, this, [&] ( const int row, const int, const int prev_row, const int )
				  { rowSelected ( row, prev_row ); } ) );
	else
		static_cast<void>( disconnect ( this, &QTableWidget::currentCellChanged, nullptr, nullptr ) );
	// When ignoring changes, disconnect from (possibly) connect signals. When not, restore the property to its previous value
	setAlwaysEmitCurrentItemChanged ( isIgnoringChanges () ? false : alwaysEmitCurrentItemChanged () );
}

void vmListWidget::setCurrentRow ( int row, const bool b_force, const bool b_makecall )
{
	if ( !b_force )
	{
		if ( row == mPrevRow || !alwaysEmitCurrentItemChanged () )
			return;
	}
	
	if ( row < 0 )
	{
		row = mPrevRow;
		if ( row < 0 ) // force a non-negative value for row
			row = mPrevRow = lastUsedRow ();
	}
	else
	{
		if ( mCurrentItem != nullptr )
			mPrevRow = mCurrentItem->row ();
	}
	setCurrentItem ( item ( row ) );
	if ( !isIgnoringChanges () ) //&& mCurrentItem )
	{
		scrollToItem ( mCurrentItem );
		setCurrentCell ( row, 0, QItemSelectionModel::ClearAndSelect );
		if ( b_makecall && mCurrentItemChangedFunc )
			mCurrentItemChangedFunc ( mCurrentItem );
	}
}

void vmListWidget::addItem ( vmListItem* item, const bool b_makecall )
{
	if ( item != nullptr )
	{
		uint row ( 0 );
		if ( isSortEnabled () )
		{
			//item->setItemIsSorted ( true );
			insertRow ( 0 );
		}
		else
		{
			row = static_cast<uint>( rowCount () );
			appendRow ();
		}
		
		setItem ( static_cast<int>( row ), 0, item );
		item->setTable ( this );
		item->update ();
		
		if ( !isIgnoringChanges () )
		{
			setCurrentItem ( item );
			scrollToItem ( mCurrentItem );
			setCurrentCell ( item->row (), 0, QItemSelectionModel::ClearAndSelect );
			if ( b_makecall && mCurrentItemChangedFunc )
				mCurrentItemChangedFunc ( mCurrentItem );
		}
	}
}

void vmListWidget::insertRow ( const uint row, const uint n )
{
	if ( row <= static_cast<uint>( rowCount () ) )
	{
		setVisibleRows ( visibleRows () + n );
		for ( uint i_row ( 0 ); i_row < n; ++i_row )
			QTableWidget::insertRow ( static_cast<int>( row + i_row ) );
		setLastUsedRow ( rowCount () - 1 );
	}
}

void vmListWidget::removeRow_list ( const uint row, const uint n, const bool bDel )
{
	if ( row < static_cast<uint>( rowCount () ) )
	{
		auto i_row ( static_cast<int>( row + n - 1 ) );
		const bool b_ignorechanges ( isIgnoringChanges () );
		const bool bResetCurrentRow ( !isIgnoringChanges () && ( mCurrentItem ? ( mCurrentItem->row () >= static_cast<int>( row ) ) : true ) );
		
		setIgnoreChanges ( true );
		setVisibleRows ( visibleRows () - n );
		if ( mPrevRow >= static_cast<int>( row ) )
			mPrevRow -= n;

		vmListItem* item ( nullptr );
		if ( bDel )
		{
			for ( ; i_row >= static_cast<int>( row ); --i_row  )
			{
				for ( int i_col ( static_cast<int>( colCount () ) - 1 ); i_col >= 0; --i_col )
				{
					item = dynamic_cast<vmListItem*>( sheetItem ( static_cast<uint>( i_row ), static_cast<uint>( i_col ) ) );
					if ( item != nullptr )
					{
						item->m_list = nullptr;
						delete item;
					}
				}
				QTableWidget::removeRow ( i_row );
			}
		}
		else
		{
			for ( ; i_row >= static_cast<int>( row ); --i_row  )
			{
				for ( int i_col ( static_cast<int>( colCount () ) - 1 ); i_col >= 0; --i_col )
				{
					if ( sheetItem ( static_cast<uint>( i_row ), static_cast<uint>(i_col) ) != nullptr )
						dynamic_cast<vmListItem*>( sheetItem ( static_cast<uint>( i_row ), static_cast<uint>( i_col ) ) )->m_list = nullptr;
					static_cast<void>( QTableWidget::takeItem ( i_row, i_col ) );
				}
				QTableWidget::removeRow ( static_cast<int>( i_row ) );
			}
		}

		setIgnoreChanges ( b_ignorechanges );
		//if ( rowCount () > 0 )
			setLastUsedRow ( rowCount () - 1 );
		//else
		//	resetLastUsedRow ();
		if ( bResetCurrentRow )
			setCurrentRow ( -1, true, true );
	}
}

void vmListWidget::clear ( const bool b_ignorechanges, const bool b_del )
{
	actionsBeforeClearing ();
	setUpdatesEnabled ( false );
	setIgnoreChanges ( b_ignorechanges ); //once called, the callee must set/unset this property
	removeRow_list ( 0, static_cast<uint>( rowCount () ), b_del );
	mPrevRow = -2;
	mCurrentItem = nullptr;
	setUpdatesEnabled ( true );
}

void vmListWidget::setAlwaysEmitCurrentItemChanged ( const bool b_emit )
{
	if ( !isIgnoringChanges () )
		mbForceEmit = b_emit ;
}

void vmListWidget::rowSelected ( const int row, const int prev_row )
{
	if ( mCurrentItem )
	{
		if ( mCurrentItem->row () == row && !alwaysEmitCurrentItemChanged () )
			return;
		mPrevRow = prev_row;
	}
	else
		mPrevRow = -2;	
	
	setCurrentItem ( item ( row ) );
	if ( !isIgnoringChanges () )
	{
		if ( mCurrentItemChangedFunc )
			mCurrentItemChangedFunc ( mCurrentItem );
	}
}

void vmListWidget::resizeEvent ( QResizeEvent* e )
{
	setColumnWidth ( 0, width () );
	e->accept ();
}

void vmListWidget::focusInEvent ( QFocusEvent* e )
{
	e->accept ();
	if ( mGotFocusFunc )
		mGotFocusFunc ();
}
