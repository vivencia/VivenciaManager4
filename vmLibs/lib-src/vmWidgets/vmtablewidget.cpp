#include "vmtablewidget.h"
#include "vmwidgets.h"
#include "vmcheckedtableitem.h"
#include "vmtableutils.h"
#include "heapmanager.h"

#include <vmUtils/fast_library_functions.h>

#include <vmNotify/vmnotify.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenu>
#include <QtGui/QKeyEvent>
#include <QtCore/QMetaMethod>

constexpr auto MAX_COL_WIDTH ( 400 );

//---------------------------------------------STATIC---------------------------------------------------------
/*static QString encode_pos(int row, int col)
{
	return QString(col + 'A') + QString::number(row + 1);
}*/
//---------------------------------------------STATIC---------------------------------------------------------

vmTableWidget::vmTableWidget ( QWidget* parent )
	: QTableWidget ( parent ), vmWidget ( WT_TABLE ),
	  mCols ( nullptr ), mTotalsRow ( -1 ), m_lastUsedRow ( -1 ), m_nVisibleRows ( 0 ), m_ncols ( 0 ), mAddedItems ( 0 ),
	  mTableChanged ( false ), mb_TableInit ( true ), mbKeepModRec ( true ), mbPlainTable ( false ), mbIsSorted ( false ),
	  mbUseWidgets ( true ), mbTableIsList ( false ), mbColumnAutoResize ( false ), mbIgnoreChanges ( true ), readOnlyColumnsMask ( 0 ),
	  modifiedRows ( 0, 10 ), mMonitoredCells ( 2 ), m_highlightedCells ( 5 ), m_itemsToReScan ( 10 ), mSearchList ( 10 ),
	  mContextMenu ( nullptr ), mOverrideFormulaAction ( nullptr ), mSetFormulaAction ( nullptr ),
	  mFormulaTitleAction ( nullptr ), mParentLayout ( nullptr ), m_searchPanel ( nullptr ), m_filterPanel ( nullptr ),
	  mContextMenuCell ( nullptr ), cellChanged_func ( nullptr ), cellNavigation_func ( nullptr ),
	  monitoredCellChanged_func ( nullptr ), rowRemoved_func ( nullptr ), rowInserted_func ( nullptr ),
	  rowClicked_func ( nullptr ), completer_func ( nullptr )
{
	setWidgetPtr ( static_cast<QWidget*>( this ) );
	//setSelectionMode ( QAbstractItemView::ExtendedSelection );
	//setSelectionBehavior ( QAbstractItemView::SelectItems );
	setAlternatingRowColors ( true );
	setAttribute ( Qt::WA_KeyCompression );
	setFrameShape ( QFrame::NoFrame );
	setFrameShadow ( QFrame::Plain );

	setContextMenuPolicy ( Qt::CustomContextMenu );
	static_cast<void>( connect ( this, &QTableWidget::customContextMenuRequested, this,
						[&] ( const QPoint pos ) { displayContextMenuForCell ( pos ); } ) );

	setEditable ( false );

	mUndoAction = new vmAction ( UNDO, TR_FUNC ( "Undo change (CTRL+Z)" ), this );
	static_cast<void>( connect ( mUndoAction, &vmAction::triggered, this, [&] () { undoChange (); } ) );

	mCopyCellAction = new vmAction ( COPY_CELL, TR_FUNC ( "Copy cell contents to clipboard (CTRL+C)" ), this );
	static_cast<void>( connect ( mCopyCellAction, &vmAction::triggered, this, [&] () { copyCellContents (); } ) );

	mCopyRowContents = new vmAction ( COPY_ROW, TR_FUNC ( "Copy row contents to clipboard (CTRL+B)" ), this );
	static_cast<void>( connect ( mCopyRowContents, &vmAction::triggered, this, [&] () { copyRowContents (); } ) );

	mInsertRowAction = new vmAction ( ADD_ROW, TR_FUNC ( "Insert line here (CTRL+I)" ), this );
	static_cast<void>( connect ( mInsertRowAction, &vmAction::triggered, this, [&] () { insertRow_slot (); } ) );

	mAppendRowAction = new vmAction ( APPEND_ROW, TR_FUNC ( "Append line (CTRL+O)" ), this );
	static_cast<void>( connect ( mAppendRowAction, &vmAction::triggered, this, [&] () { appendRow (); } ) );

	mDeleteRowAction = new vmAction ( DEL_ROW, TR_FUNC ( "Remove this line (CTRL+R)" ), this );
	static_cast<void>( connect ( mDeleteRowAction, &vmAction::triggered, this, [&] () { removeRow_slot (); } ) );

	mClearRowAction = new vmAction ( CLEAR_ROW, TR_FUNC ( "Clear line (CTRL+D)" ), this );
	static_cast<void>( connect ( mClearRowAction, &vmAction::triggered, this, [&] () { clearRow_slot (); } ) );

	mClearTableAction = new vmAction ( CLEAR_TABLE, TR_FUNC ( "Clear table (CTRL+T)" ), this );
	static_cast<void>( connect ( mClearTableAction, &vmAction::triggered, this, [&] () { clearTable_slot (); } ) );

	mSubMenuFormula = new QMenu ( QStringLiteral ( "Formula" ) );

	mContextMenu = new QMenu ( this );
	mContextMenu->addAction ( mUndoAction );
	mContextMenu->addSeparator ();
	mContextMenu->addAction ( mCopyCellAction );
	mContextMenu->addAction ( mCopyRowContents );
	mContextMenu->addSeparator ();
	mContextMenu->addAction ( mAppendRowAction );
	mContextMenu->addAction ( mInsertRowAction );
	mContextMenu->addAction ( mDeleteRowAction );
	mContextMenu->addAction ( mClearRowAction );
	mContextMenu->addAction ( mClearTableAction );
	mContextMenu->addSeparator ();
	mContextMenu->addMenu ( mSubMenuFormula );
}

vmTableWidget::~vmTableWidget ()
{
	disconnect ( this, nullptr, nullptr, nullptr );
	heap_dellarr ( mCols );
	heap_del ( mCopyCellAction );
	heap_del ( mCopyRowContents );
	heap_del ( mInsertRowAction );
	heap_del ( mDeleteRowAction );
	heap_del ( mClearRowAction );
	heap_del ( mClearTableAction );
	heap_del ( mUndoAction );
	heap_del ( mContextMenu );
	heap_del ( mOverrideFormulaAction );
	heap_del ( mSetFormulaAction );
	heap_del ( mFormulaTitleAction );
	heap_del ( m_searchPanel );
	heap_del ( m_filterPanel );
}

vmTableColumn* vmTableWidget::createColumns ( const uint nCols )
{
	mCols = new vmTableColumn[nCols];
	m_ncols = nCols;
	return mCols;
}

void vmTableWidget::initTable ( const uint rows )
{
	setUpdatesEnabled ( false );
	m_nVisibleRows = rows + static_cast<uint>( !mbPlainTable );

	if ( isList () )
		initList ();
	else
		initTable2 ();

	setUpdatesEnabled ( true );
	mb_TableInit = false;
}

bool vmTableWidget::selectFound ( const vmTableItem* item )
{
	if ( item != nullptr )
	{
		scrollToItem ( item, QAbstractItemView::PositionAtCenter );
		//setCurrentItem ( const_cast<vmTableItem*>( item ), QItemSelectionModel::ClearAndSelect );
		//selectRow ( item->row () );
		return true;
	}
	return false;
}

// Assumes searchTerm is valid. It's a private function for that reason: only friend
bool vmTableWidget::searchStart ( const QString& searchTerm )
{
	if ( searchTerm.count () >= 2 && searchTerm != mSearchTerm )
	{
		searchCancel ( false );

		uint i_row ( 0 ), i_col ( 0 );
		uint max_row ( lastUsedRow () >= 0 ? static_cast<uint>( lastUsedRow () ) + 1 : 0 );
		uint max_col ( isList () ? 1 : colCount () );
		vmTableItem* item ( nullptr );
		mSearchTerm = searchTerm;

		for ( ; i_row < max_row; ++i_row )
		{
			for ( i_col = 0 ; i_col < max_col; ++i_col )
			{
				if ( isColumnSelectedForSearch ( i_col ) )
				{
					item = sheetItem ( i_row, i_col );
					if ( item->text ().contains ( searchTerm, Qt::CaseInsensitive ) )
					{
						item->highlight ( vmBlue );
						mSearchList.append ( item );
					}
				}
			}
		}
		return !mSearchList.isEmpty ();
	}
	return false;
}

void vmTableWidget::searchCancel ( const bool b_real_cancel )
{
	for ( uint i ( 0 ); i < mSearchList.count (); ++i )
		mSearchList.at ( i )->highlight ( vmDefault_Color );
	if ( b_real_cancel && horizontalHeader () && horizontalHeader ()->isVisible () )
		dynamic_cast<vmCheckedTableItem*>( horizontalHeader () )->setCheckable ( false );
	mSearchList.clearButKeepMemory ( false );
	mSearchTerm.clear ();
}

bool vmTableWidget::searchFirst ()
{
	return selectFound ( mSearchList.first () );
}

bool vmTableWidget::searchPrev ()
{
	return selectFound ( mSearchList.prev () );
}

bool vmTableWidget::searchNext ()
{
	return selectFound ( mSearchList.next () );
}

bool vmTableWidget::searchLast ()
{
	return selectFound ( mSearchList.last () );
}

int vmTableWidget::getRow ( const QString& cellText, const Qt::CaseSensitivity cs, const uint startrow, uint nth_find )
{
	if ( isEmpty () ) return -1;

	for ( uint row ( startrow ); row <= static_cast<uint>( lastUsedRow () ); ++row )
	{
		for ( uint col ( 0 ); col < colCount (); ++col )
		{
			if ( cellText.compare ( sheetItem ( row, col )->text (), cs ) == 0 )
			{
				if ( nth_find == 0 )
					return static_cast<int>( row );
				--nth_find;
			}
		}
	}
	return -1;
}

void vmTableWidget::prepareToInsertOrRemoveRow ()
{
	if ( m_filterPanel && m_filterPanel->isVisible () )
	{
		m_filterPanel->hide ();
	}
	if ( isSortEnabled () )
	{
		setSortingEnabled ( false );
	}
}

void vmTableWidget::recallAfterRowInsertedOrRemoved ()
{
	if ( isSortEnabled () )
	{
		setSortingEnabled ( true );
	}
}

void vmTableWidget::insertRow ( const uint row, const uint n )
{
	if ( row <= static_cast<uint> ( rowCount () ) )
	{
		uint i_row ( 0 );
		uint i_col ( 0 );
		QString new_formula;
		vmTableItem* sheet_item ( nullptr ), *new_SheetItem ( nullptr );

		if ( !mb_TableInit )
		{
			m_nVisibleRows += n;
			mTotalsRow += n;
		}

		prepareToInsertOrRemoveRow ();
		for ( ; i_row < n; ++i_row )
		{
			QTableWidget::insertRow ( static_cast<int>( row + i_row ) );
			for ( i_col = 0; i_col < colCount (); ++i_col )
			{
				new_SheetItem = new vmTableItem ( mCols[i_col].wtype, mCols[i_col].text_type, mCols[i_col].default_value, this );
				setItem ( static_cast<int>( row + i_row ), static_cast<int>( i_col ), new_SheetItem );

				if ( mbUseWidgets )
				{
					new_SheetItem->setButtonType ( mCols[i_col].button_type );
					new_SheetItem->setCompleterType ( mCols[i_col].completer_type );
					setCellWidget ( new_SheetItem );

					/* The formula must be set for last because setFormula calls vmTableItem::targetsFromFormula ();
					 * when inserting cells, that function might attempt to solve the formula using data from the previous
					 * cell (i.e. before inserting) or, worse, from the totalsRow (), which will cause unpredicted (but fatal) results
					 */
					if ( !mCols[i_col].formula.isEmpty () )
						new_SheetItem->setFormula ( mCols[i_col].formula, QString::number ( row + i_row ) );
				}
				new_SheetItem->setEditable ( isEditable () );
			}
			if ( !mb_TableInit && rowInserted_func )
				rowInserted_func ( row + i_row );
		}

		if ( isPlainTable () )
			return;

		if ( !mb_TableInit )
		{
			// update all rows after the inserted ones, so that their formula reflects the row changes
			for ( i_row = row + n; static_cast<int>( i_row ) < totalsRow (); ++i_row )
			{
				for ( i_col = 0; i_col < colCount (); ++i_col )
				{
					sheet_item = sheetItem ( i_row, i_col );
					if ( sheet_item->hasFormula () )
					{
						new_formula = sheet_item->formulaTemplate ();
						sheet_item->setFormula ( new_formula, QString::number ( i_row ) );
					}
				}
			}
			fixTotalsRow ();

			const uint modified_rows ( modifiedRows.count () );
			if ( modified_rows > 0 )
			{
				uint i ( 0 ), list_value ( 0 ), start_modification ( 0 );
				for ( i = 0; i < modified_rows; ++i )
				{
					if ( row < modifiedRows.at ( i ) )
						break;
					start_modification++; // modified above insertion start point are not affected
				}
				for ( i = start_modification; i < modified_rows; ++i )
				{
					list_value = modifiedRows.at ( i );
					modifiedRows.replace ( i, list_value + n ); // push down modified rows
				}
			}
		}
		recallAfterRowInsertedOrRemoved ();
	}
}

void vmTableWidget::removeRow ( const uint row, const uint n )
{
	if ( static_cast<int>( row ) < lastRow () )
	{
		if ( !isPlainTable () && static_cast<int>(row) >= totalsRow () )
			return;
		uint actual_n ( 0 );
		uint i_row ( 0 );

		prepareToInsertOrRemoveRow ();
		for ( ; i_row < n; ++i_row )
		{
			// Emit the signal before removing the row so that the slot(s) that catch the signal can retrieve information from the row
			// (i.e. an ID, or name or anything to be used by the database ) before it disappears
			if ( rowRemoved_func )
			{
				if ( !rowRemoved_func ( row ) )
					continue;
			}
			//if ( !isPlainTable () && static_cast<int>(row) >= lastRow () )
			//	continue;

			QTableWidget::removeRow ( static_cast<int>( row ) );
			actual_n++;
		}

		if ( actual_n > 0 )
		{
			m_nVisibleRows -= actual_n;
			if ( static_cast<int>( row ) <= lastUsedRow () )
				m_lastUsedRow -= actual_n;
			//if ( static_cast<int>( row + actual_n ) >= lastUsedRow () )
			//	setLastUsedRow ( static_cast<int>( row - 1 ) );

			if ( !isPlainTable () )
			{
				mTotalsRow -= actual_n;
				vmTableItem* sheet_item ( nullptr );
				uint i_col ( 0 );
				QString new_formula;
				const int last_row ( lastRow () );

				for ( i_row = row; static_cast<int>(i_row) < last_row; ++i_row )
				{
					for ( i_col = 0; i_col < colCount (); ++i_col )
					{
						sheet_item = sheetItem ( i_row, i_col );
						if ( sheet_item->hasFormula () )
						{
							new_formula = sheet_item->formulaTemplate ();
							sheet_item->setFormula ( new_formula, QString::number ( i_row ) );
						}
					}
				}
				fixTotalsRow ();

				const uint modified_rows ( modifiedRows.count () );
				if ( modified_rows > 0 )
				{
					uint i ( 0 ), list_value ( 0 ), start_modification ( 0 );
					for ( i = 0 ; i < modifiedRows.count (); ++i )
					{
						if ( row < modifiedRows.at ( i ) )
							break;
						start_modification++; // modified rows above deletion start point are not affected
					}
					for ( i = start_modification; i < modifiedRows.count (); ++i )
					{
						list_value = modifiedRows.at ( i );
						modifiedRows.replace ( i, list_value - n ); // push up modified rows
					}
					modifiedRows.removeOne ( row, 0 );
				}
			}
		}
		recallAfterRowInsertedOrRemoved ();
	}
}

void vmTableWidget::setRowVisible ( const uint row, const bool bVisible )
{
	const bool b_isVisible ( !isRowHidden ( static_cast<int>( row ) ) );
	if ( !b_isVisible && bVisible )
	{
		++m_nVisibleRows;
		QTableWidget::setRowHidden ( static_cast<int>( row ), false );
	}
	else if ( b_isVisible && !bVisible )
	{
		--m_nVisibleRows;
		QTableWidget::setRowHidden ( static_cast<int>( row ), true );
	}
}

void vmTableWidget::clearRow ( const uint row, const uint n )
{
	if ( isEditable () )
	{
		if ( static_cast<int>( row ) <= lastUsedRow () )
		{
			vmTableItem* item ( nullptr );
			for ( uint i_row ( row ); i_row < row + n; ++i_row )
			{
				// Callback before removing so that its values might be used for something before they cannot be used anymore
				if ( rowRemoved_func )
				{
					if ( !rowRemoved_func ( i_row ) )
						continue;
				}

				for ( uint i_col ( 0 ); i_col <= ( colCount () - 1 ); ++i_col )
				{
					if ( !isBitSet ( readOnlyColumnsMask, static_cast<uchar>( i_col ) ) )
					{
						item = sheetItem ( i_row, i_col );
						/* TODO test: In previous versions the argument to setTextToDefault was set to true.
						 * Now, we issue rowRemoved_func only once
						*/
						item->setTextToDefault ();
						item->highlight ( vmDefault_Color );
					}
				}
			}
		}
	}
}

// Clear items for fresh view
void vmTableWidget::clear ( const bool force )
{
	const int max_row ( force ? totalsRow () - 1 : lastUsedRow () );
	if ( max_row > 0 )
	{
		actionsBeforeClearing ();
		setUpdatesEnabled ( false );
		uint i_col ( 0 );
		vmTableItem* item ( nullptr );
		for ( uint i_row ( 0 ); i_row <= static_cast<uint>( max_row ); ++i_row )
		{
			for ( i_col = 0; i_col <= ( colCount () - 1 ); ++i_col )
			{
				if ( !isBitSet ( readOnlyColumnsMask, static_cast<uchar>( i_col ) ) )
				{
					item = sheetItem ( i_row, i_col );
					item->setTextToDefault ( isEditable () );
					// TEST 2023-10-12: is the next line ever necessary??
					//item->highlight ( (i_row % 2 == 0) ? vmDefault_Color : vmDefault_Color_Alternate );
				}
			}
		}
		scrollToItem ( sheetItem ( 0, 0 ), QAbstractItemView::PositionAtTop );
		setCurrentCell ( 0, 0, QItemSelectionModel::ClearAndSelect );
		mTableChanged = false;
		resetLastUsedRow ();
		derivedClassTableCleared ();
		setUpdatesEnabled ( true );
	}
}

void vmTableWidget::actionsBeforeClearing ()
{
	if ( m_searchPanel && m_searchPanel->isVisible () )
	{
		utilitiesLayout ()->removeWidget ( m_searchPanel );
		m_searchPanel->setVisible ( false );
	}
	if ( m_filterPanel && m_filterPanel->isVisible () )
	{
		utilitiesLayout ()->removeWidget ( m_filterPanel );
		m_filterPanel->setVisible ( false );
	}
}

void vmTableWidget::setIgnoreChanges ( const bool b_ignore )
{
	rowClickedConnection ( !(mbIgnoreChanges = b_ignore) );
	if ( !b_ignore )
	{
		if ( isPlainTable () )
		{
			static_cast<void>( connect ( this, &QTableWidget::itemChanged, this, [&] ( QTableWidgetItem *item ) {
					cellModified ( dynamic_cast<vmTableItem*>( item ) ); } ) );
		}
		if ( cellNavigation_func != nullptr )
		{
			static_cast<void>( connect ( this, &QTableWidget::currentCellChanged, this, [&] ( const int row, const int col, const int prev_row, const int prev_col ) {
				cellNavigation_func ( static_cast<uint>( row ), static_cast<uint>( col ), static_cast<uint> ( prev_row ), static_cast<uint> ( prev_col ) ); } ) );
		}
	}
	else
	{
		static_cast<void>( this->disconnect () );
	}
}

void vmTableWidget::rowClickedConnection ( const bool b_activate )
{
	if ( rowClicked_func )
	{
		if ( b_activate )
		{
			static_cast<void>( connect ( this, &QTableWidget::itemClicked, this, [&] ( QTableWidgetItem* current ) {
				rowClicked_func ( current->row () ); } ) );
		}
		else
			static_cast<void>( disconnect ( this, &QTableWidget::itemClicked, nullptr, nullptr ) );
	}
}

void vmTableWidget::setCellValue ( const QString& value, const uint row, const uint col )
{
	if ( col < colCount () )
	{
		if ( static_cast<int>( row ) >= lastRow () )
			appendRow ();
		sheetItem ( row, col )->setText ( value, false );
	}
}

void vmTableWidget::setSimpleCellText ( vmTableItem* const item )
{
	item->QTableWidgetItem::setText ( item->text () );
	if ( cellChanged_func != nullptr )
		cellChanged_func ( item );
}

void vmTableWidget::setSimpleCellTextWithoutNotification ( vmTableItem* const item, const QString& text )
{
	const bool b_isconnected ( isSignalConnected ( QMetaMethod::fromSignal ( &QTableWidget::itemChanged ) ) );
	if ( b_isconnected )
		static_cast<void> ( disconnect ( this, &QTableWidget::itemChanged, nullptr, nullptr ) );
	item->QTableWidgetItem::setText ( text );
	if ( b_isconnected && !isIgnoringChanges () )
	{
		static_cast<void>( connect ( this, &QTableWidget::itemChanged, this, [&] ( QTableWidgetItem* item ) {
				cellModified ( dynamic_cast<vmTableItem*>( item ) ); } ) );
	}
}

void vmTableWidget::setRowData ( const spreadRow* s_row, const bool b_notify )
{
	if ( s_row != nullptr )
	{
		if ( s_row->row == -1 )
			const_cast<spreadRow*>( s_row )->row = lastUsedRow () + 1;
		if ( s_row->row >= lastRow () )
			appendRow ();
		for ( uint i_col ( 0 ), used_col ( s_row->column.at ( 0 ) ); i_col < s_row->column.count (); used_col = s_row->column.at ( ++i_col ) )
			sheetItem ( static_cast<uint>( s_row->row ), used_col )->setText ( s_row->field_value.at ( used_col ), b_notify, false );
	}
}

void vmTableWidget::rowData ( const uint row, spreadRow* s_row ) const
{
	if ( static_cast<int>( row ) < lastRow () )
	{
		uint i_col ( 0 );
		s_row->row = static_cast<int>( row );
		s_row->field_value.clearButKeepMemory ();

		for ( ; i_col < colCount (); ++i_col )
			s_row->field_value.append ( sheetItem ( row, i_col )->text () );
	}
}

void vmTableWidget::cellContentChanged ( const vmTableItem* const item )
{
	mTableChanged = true;
	if ( mbKeepModRec )
	{
		const auto row ( static_cast<uint>( item->row () ) );
		uint insert_pos ( modifiedRows.count () ); // insert rows in crescent order
		for ( auto i ( static_cast<int>( modifiedRows.count () - 1 ) ); i >= 0; --i )
		{
			if ( row == modifiedRows.at ( i ) )
			{
				if ( cellChanged_func )
					cellChanged_func ( item );
				return;
			}
			if ( row > modifiedRows.at ( i ) )
			{
				break;
			}
			--insert_pos;
		}
		modifiedRows.insert ( insert_pos, row );
	}
	/* There is no need to make this method virtual, nor to override it in any derived classes;
	 * Since this method does what it does well and any virtual method would invariably call here,
	 * we save the trouble and call the derived class' method for it.
	 */

	derivedClassCellContentChanged ( item );
	if ( cellChanged_func )
		cellChanged_func ( item );
}

void vmTableWidget::cellModified ( vmTableItem* const item )
{
	/* Simple tables have no widgets, only items (vmTableItem). If the table is editable, i.e. lets the user press F2 to edit the content of a cell,
	 * we are called here to signify the contents have changed (after an enter key press or a focus out). Because vmTableItem::setText hasn't been called
	 * at all, the item's private members are left untouched and do not reflect the new text. So we get only the current (now old) information. We have to call ourselves
	 * setText and avoid any signals that it might trigger
	 */
	item->setText ( item->QTableWidgetItem::data ( Qt::DisplayRole ).toString (), false );
	if ( cellChanged_func != nullptr )
		cellChanged_func ( item );
}

// Customize context menu
void vmTableWidget::renameAction ( const contextMenuDefaultActions action, const QString& new_text )
{
	if ( new_text.isEmpty () ) return;
	switch ( action )
	{
		case UNDO:
			mUndoAction->setLabel ( new_text );
		break;
		case COPY_CELL:
			mCopyCellAction->setLabel ( new_text );
		break;
		case COPY_ROW:
			mCopyRowContents->setLabel ( new_text );
		break;
		case ADD_ROW:
			mInsertRowAction->setLabel ( new_text );
		break;
		case DEL_ROW:
			mDeleteRowAction->setLabel ( new_text );
		break;
		case CLEAR_ROW:
			mClearRowAction->setLabel ( new_text );
		break;
		case CLEAR_TABLE:
			mClearTableAction->setLabel ( new_text );
		break;
		default:
		break;
	}
}

void vmTableWidget::addContextMenuAction ( vmAction* action )
{
	if ( mAddedItems == 0 )
		mSeparator = mContextMenu->addSeparator ();
	++mAddedItems;
	mContextMenu->addAction ( static_cast<QAction*>( action ) );
}

void vmTableWidget::removeContextMenuAction ( vmAction* action )
{
	if ( mAddedItems == 0 )
		return;
	if ( mAddedItems == 1 )
		mContextMenu->removeAction ( mSeparator );
	--mAddedItems;
	mContextMenu->removeAction ( action );
}

void vmTableWidget::rowFromStringRecord ( const stringRecord& rec, const uint row, spreadRow* s_row )
{
	spreadRow* srow ( s_row );
	uint i_col ( 0 );
	if ( srow == nullptr )
	{
		srow = new spreadRow;
		for ( ; i_col < colCount (); ++i_col )
			srow->column[i_col] = i_col;
	}
	srow->row = row;
	if ( rec.first () )
	{
		i_col = 0;
		do
		{
			srow->field_value[i_col++] = rec.curValue ();
		} while ( rec.next () );
		setRowData ( srow );
	}
}

void vmTableWidget::loadFromStringTable ( const stringTable& data, const bool b_append )
{
	//int start_row ( 0 );
	if ( !b_append )
		clear ( true );
	/*else
	{
		start_row = lastUsedRow () == -1 ? 0 : lastUsedRow () + 1;
		int n_new_rows ( static_cast<int>( data.countRecords () ) );
		if ( n_new_rows >= (lastRow () - start_row) )
		{
			n_new_rows -= (lastRow () - start_row);
			n_new_rows++;
			insertRow ( static_cast<uint>(start_row), static_cast<uint>(n_new_rows) );
		}
	}*/

	if ( data.isOK () )
	{
		setUpdatesEnabled ( false );
		auto s_row ( new spreadRow );
		uint i_col ( 0 );
		int start_row ( lastUsedRow () == -1 ? 0 : lastUsedRow () + 1 );

		for ( ; i_col < colCount (); ++i_col )
			s_row->column[i_col] = i_col;

		const stringRecord* rec ( &data.first () );
		if ( rec->isOK () )
		{
			do
			{
				rowFromStringRecord ( *rec, start_row, s_row );
				++start_row;
				rec = &data.next ();
			} while ( rec->isOK () );
		}
		delete s_row;
		setUpdatesEnabled ( true );
		scrollToTop ();
	}
}

void vmTableWidget::makeStringTable ( stringTable& data )
{
	if ( lastUsedRow () >= 0 )
	{
		stringRecord rec;
		uint i_col ( 0 );
		for ( uint i_row ( 0 ); i_row <= static_cast<uint>( lastUsedRow () ); ++i_row )
		{
			for ( i_col = 0; i_col < colCount (); ++i_col )
				rec.fastAppendValue ( sheetItem ( i_row, i_col )->text () );
			data.fastAppendRecord ( rec );
			rec.clear ();
		}
	}
}

void vmTableWidget::setCellWidget ( vmTableItem* const sheet_item )
{
	vmWidget* widget ( nullptr );
	// read only columns do not connect signals, do not need completers
	const bool bCellReadOnly ( isBitSet ( readOnlyColumnsMask, static_cast<uchar>( sheet_item->column () ) )
					|| ( !isPlainTable () && sheet_item->row () == totalsRow () ) );

	switch ( sheet_item->widgetType () )
	{
		case WT_LINEEDIT:
			widget = new vmLineEdit;
			dynamic_cast<vmLineEdit*>( widget )->setFrame ( false );
			dynamic_cast<vmLineEdit*>( widget )->setTextType ( sheet_item->textType () );
			if ( !bCellReadOnly )
			{
				widget->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) { textWidgetChanged ( sender ); } );
			}
		break;
		case WT_LINEEDIT_WITH_BUTTON:
			widget = new vmLineEditWithButton;
			dynamic_cast<vmLineEditWithButton*>( widget )->setButtonType ( 0, sheet_item->buttonType () );
			dynamic_cast<vmLineEditWithButton*>( widget )->lineControl ()->setTextType ( sheet_item->textType () );
			if ( !bCellReadOnly )
			{
				dynamic_cast<vmLineEditWithButton*>( widget )->lineControl ()->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
					textWidgetChanged ( sender ); } );
			}
		break;
		case WT_CHECKBOX: // so far, only check box has a different default value, depending on the table to which they belong
			widget = new vmCheckBox;
			dynamic_cast<vmCheckBox*>( widget )->setChecked ( sheet_item->defaultValue () == CHR_ONE );
			if ( !bCellReadOnly )
				widget->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) { checkBoxToggled ( sender ); } );
		break;
		case WT_DATEEDIT:
			widget = new vmDateEdit;
			if ( !bCellReadOnly )
				widget->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) { dateWidgetChanged ( sender ); } );
		break;
		case WT_TIMEEDIT:
			widget = new vmTimeEdit;
			if ( !bCellReadOnly )
				widget->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) { timeWidgetChanged ( sender ); } );
		break;
		case WT_COMBO:
			widget = new vmComboBox;
			if ( !bCellReadOnly )
				widget->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) { textWidgetChanged ( sender ); } );
		break;
		default:
		return;
	}

	if ( completer_func )
		completer_func ( widget, sheet_item->completerType () );
	sheet_item->setWidget ( widget );
	QTableWidget::setCellWidget ( sheet_item->row (), sheet_item->column (), widget->toQWidget () );
	widget->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const widget ) {
		cellWidgetRelevantKeyPressed ( ke, widget ); } );
	widget->setOwnerItem ( sheet_item );
}

void vmTableWidget::addToLayout ( QGridLayout* glayout, const uint row, const uint column )
{
	if ( setParentLayout ( glayout ) )
		glayout->addWidget ( this, static_cast<int>(row), static_cast<int>(column) );
}

void vmTableWidget::addToLayout ( QVBoxLayout* vblayout, const uint stretch )
{
	if ( setParentLayout ( vblayout ) )
	{
		vblayout->addWidget ( this, static_cast<int>(stretch) );
		setUtilitiesPlaceLayout ( vblayout );
	}
}

bool vmTableWidget::setParentLayout ( QGridLayout* glayout )
{
	if ( mParentLayout == static_cast<QLayout*>( glayout ) )
		return false;
	if ( mParentLayout != nullptr )
		mParentLayout->removeWidget ( this );
	mParentLayout = static_cast<QLayout*>( glayout );
	return true;
}

bool vmTableWidget::setParentLayout ( QVBoxLayout* vblayout )
{
	if ( mParentLayout == static_cast<QLayout*>( vblayout ) )
		return false;
	if ( mParentLayout != nullptr )
		mParentLayout->removeWidget ( this );
	mParentLayout = static_cast<QLayout*>( vblayout );
	return true;
}

void vmTableWidget::setIsList ()
{
	mbTableIsList = true;
	setIsPlainTable ( false );
}

void vmTableWidget::setIsPlainTable ( const bool b_usewidgets )
{
	mbUseWidgets = b_usewidgets;
	mbPlainTable = true;
	setKeepModificationRecords ( false );
	setSelectionMode ( QAbstractItemView::SingleSelection );
	setSelectionBehavior ( QAbstractItemView::SelectRows );
}

void vmTableWidget::setEnableSorting ( const bool b_enable )
{
	mbIsSorted = b_enable;
	setSortingEnabled ( b_enable );
}

/* For optimal results, when populating a table, set it readonly.
 * When clearing a table, set it readonly. This way, there won't be a plethora of
 * signals emitted.
 */
void vmTableWidget::setEditable ( const bool editable )
{
	const int last_row ( lastRow () );
	if ( last_row > 0 )
	{
		uint i_row ( 0 );
		uint i_col ( 0 );
		for ( ; static_cast<int>( i_row ) < last_row; ++i_row )
		{
			for ( i_col = 0; i_col < colCount (); ++i_col )
			{
				if ( !isBitSet ( readOnlyColumnsMask, static_cast<uchar>(i_col) ) )
					sheetItem ( i_row, i_col )->setEditable ( editable );
			}
		}
		setIgnoreChanges ( !editable );
		vmWidget::setEditable ( editable );
	}
}

void vmTableWidget::setPlainTableEditable ( const bool editable )
{
	vmWidget::mb_editable = editable;
}

void vmTableWidget::setLastUsedRow ( const int row )
{
	//if ( row > m_lastUsedRow && row < ( isPlainTable () ? rowCount () : static_cast<int>( totalsRow () ) ) )
	if ( row < lastRow () )
		m_lastUsedRow = row;
}

uint vmTableWidget::selectedRowsCount () const
{
	uint n ( 0 );
	for ( uint i_row ( 0 ); static_cast<int>(i_row) <= lastUsedRow (); ++i_row )
	{
		for ( uint i_col ( 0 ); i_col < ( colCount () - 1 ); ++i_col )
		{
			if ( sheetItem ( i_row, i_col )->isSelected () )
			{
				++n;
				break;
			}
		}
	}
	return n;
}

bool vmTableWidget::isRowEmpty ( const uint row ) const
{
	for ( uint i_col ( 0 ); i_col < colCount (); ++i_col )
	{
		if ( sheetItem ( row, i_col )->text () != sheetItem ( row, i_col )->defaultValue () )
		{ // cell is not empty
			//if ( !isBitSet ( readOnlyColumnsMask, i_col ) ) { // and it is a cell that can be emptied
			return false; // ergo, row is not clear
			//}
		}
	}
	return true;
}

/* The table defaults to selection by item instead of by row so that one can deal with the cells individually,
but more than often we need the data from the row and we use a single selected item to account for the whole row
*/
bool vmTableWidget::isRowSelected ( const uint row ) const
{
	if ( selectionBehavior () == QAbstractItemView::SelectRows )
	{
		if ( !selectedItems ().isEmpty () )
			return selectedItems ().contains ( sheetItem ( row, 0 ) );
	}
	else
	{
		for ( uint i_col ( 0 ); i_col < colCount (); ++i_col )
		{
			if ( sheetItem ( row, i_col )->isSelected () )
				return true;
		}
	}
	return false;
}

bool vmTableWidget::isRowSelectedAndNotEmpty ( const uint row ) const
{
	if ( isRowSelected ( row ) )
	{
		vmTableItem* item ( nullptr );
		for ( uint i_col = 0; i_col < colCount (); ++i_col )
		{
			item = sheetItem ( row, i_col );
			if ( !item->text ().isEmpty () )
				return ( !isBitSet ( readOnlyColumnsMask, static_cast<uchar>(i_col) ) );
		}
	}
	return false;
}

void vmTableWidget::setTableUpdated ()
{
	modifiedRows.clearButKeepMemory ();
	mTableChanged = false;
	// Accept user input and save data for newer usage
	for ( uint i_row ( 0 ); static_cast<int>(i_row) <= lastUsedRow (); ++i_row )
	{
		for ( uint i_col = 0; i_col <= ( colCount () - 1 ); ++i_col )
			sheetItem ( i_row, i_col )->syncOriginalTextWithCurrent ();
	}
	derivedClassTableUpdated ();
}

void vmTableWidget::setHiddenText ( const uint row, const uint col, const QString& str, const bool notify_change )
{
	vmTableItem* item ( sheetItem ( row, col ) );
	if ( item != nullptr )
	{
		item->setData ( Qt::WhatsThisRole, str );
		if ( notify_change )
			cellContentChanged ( item );
	}
}

QString vmTableWidget::hiddenText ( const uint row, const uint col ) const
{
	const vmTableItem* item ( sheetItem ( row, col ) );
	return ( ( item != nullptr ) ? item->data ( Qt::WhatsThisRole ).toString () : emptyString );
}

void vmTableWidget::cellNavigateUpward ()
{
	if ( currentRow () >= 1 )
	{
		setCurrentCell ( currentRow () - 1, currentColumn () );
	}
}

void vmTableWidget::cellNavigateDownward ()
{
	if ( currentRow () < lastRow () )
	{
		setCurrentCell ( currentRow () + 1, currentColumn () );
	}
}

void vmTableWidget::cellNavigateForward ()
{
	if ( currentColumn () < static_cast<int>( colCount () ) )
	{
		setCurrentCell ( currentRow (), currentColumn () + 1 );
	}
	else
	{
		if ( currentRow () < lastRow () )
		{
			setCurrentCell ( currentRow () + 1, 0 );
		}
	}
}

void vmTableWidget::cellNavigageBackward ()
{
	if ( currentColumn () >= 1 )
	{
		setCurrentCell ( currentRow (), currentColumn () - 1 );
	}
	else
	{
		if ( currentRow () >= 1 )
		{
			setCurrentCell ( currentRow () - 1, colorCount () - 1 );
		}
	}
}

void vmTableWidget::setColumnReadOnly ( const uint i_col, const bool readonly )
{
	if ( i_col < colCount () )
	{
		readonly ?  setBit ( readOnlyColumnsMask, static_cast<uchar>(i_col) ) :
					unSetBit ( readOnlyColumnsMask, static_cast<uchar>(i_col) );
	}
}

int vmTableWidget::isCellMonitored ( const vmTableItem* const item )
{
	if ( item == nullptr )
		return -2; // a null item is to be ignored, obviously. -2 will not be monitored
	for ( uint i ( 0 ); i < mMonitoredCells.count (); ++i )
	{
		if ( mMonitoredCells.at ( i ) == item )
			return static_cast<int>( i );
	}
	return -1; //-1 not monitored
}

//todo: monitoredCellChanged_func gets called before the item's text is updated, so we get the old text instead of the new one
void vmTableWidget::insertMonitoredCell ( const uint row, const uint col )
{
	const vmTableItem* item ( sheetItem ( row, col ) );
	if ( isCellMonitored ( item ) == -1 )
	{
		mMonitoredCells.append ( item );
		/* Most likely, a monitored cell will be read only, and display the result of some formula dependent on changes happening
		 * on other cells. setCellWidget is the method responsible for setting the callbacks for altered contents, but will not
		 * do so on read only cells. Therefore, we do this here only for cells that matter. We do not need to change the text of the
		 * widget because vmTableItem already does it, and, in fact, monitoredCellChanged_func will be called by the setText method
		 * of widget which, in turn, is called by vmTableItem::setText
		*/
		if ( isBitSet ( readOnlyColumnsMask, static_cast<uchar>(col) ) || row == static_cast<uint>(totalsRow ()) )
		{
			if ( item->widget () )
			{ // call setCallbackForMonitoredCellChanged with nullptr as argument when you wish to handle the monitoring outside this class
				item->widget ()->setCallbackForContentsAltered ( [&, item] ( const vmWidget* const ) {
						monitoredCellChanged_func ( item ); } );
			}
			else //callback for simple text cells (plain table)
				cellChanged_func = monitoredCellChanged_func;
		}
	}
}

void vmTableWidget::removeMonitoredCell ( const uint row, const uint col )
{
	const int pos ( isCellMonitored ( sheetItem ( row, col ) ) );
	if ( pos >= 0 )
		mMonitoredCells.remove ( pos );
}

void vmTableWidget::setCellColor ( const uint row, const uint col, const Qt::GlobalColor color )
{
	if ( col < colCount () )
		sheetItem ( row, col )->setData ( Qt::BackgroundRole, QBrush ( color ) );
}

void vmTableWidget::highlight ( const VMColors color, const QString& text )
{
	if ( color != vmDefault_Color )
	{
		vmTableItem* item ( nullptr );
		uint i_col ( 0 );
		for ( uint i_row ( 0 ); static_cast<int>( i_row ) <= lastUsedRow (); ++i_row )
		{
			for ( i_col = 0; i_col < colCount (); ++i_col )
			{
				if ( sheetItem ( i_row, i_col )->text ().contains ( text, Qt::CaseInsensitive ) )
				{
					item = sheetItem ( i_row, i_col );
					item->highlight ( color );
					m_highlightedCells.append ( item );
				}
			}
		}
	}
	else
	{
		for ( uint i ( 0 ); i < m_highlightedCells.count (); ++i )
			m_highlightedCells.at ( i )->highlight ( vmDefault_Color );
		m_highlightedCells.clearButKeepMemory ();
	}
}

void vmTableWidget::resizeColumn ( const uint col, const QString& text )
{
	if ( autoResizeColumns () )
	{
		const QSize sz ( fontMetrics ().size ( Qt::TextDontClip, text ) );
		if ( sz.width () < MAX_COL_WIDTH )
		{
			if ( sz.width () >= columnWidth ( static_cast<int>( col ) ) )
				setColumnWidth ( static_cast<int>( col ), sz.width () + 10 );
		}
	}
}

inline bool vmTableWidget::isColumnSelectedForSearch ( const uint column ) const
{
	if ( !isList () )
		return column < colCount () ? dynamic_cast<vmCheckedTableItem*>( horizontalHeader () )->isChecked ( column ) : false;
	return ( column == 0 );
}

void vmTableWidget::setColumnSearchStatus ( const uint column, const bool bsearch )
{
	if ( column < colCount () )
		dynamic_cast<vmCheckedTableItem*> ( horizontalHeader () )->setChecked ( column, bsearch );
}

void vmTableWidget::reHilightItems ( vmTableItem* next, vmTableItem* prev )
{
	if ( prev )
		prev->setBackground ( QBrush () );
	if ( next )
	{
		next->setBackground ( QBrush ( Qt::yellow ) );
		scrollToItem ( next, QAbstractItemView::PositionAtTop );
	}
}

void vmTableWidget::displayContextMenuForCell ( const QPoint pos )
{
	static QList<QAction*> sepActionList;
	if ( !sepActionList.isEmpty () )
	{
		QList<QAction*>::const_iterator itr_start ( sepActionList.constBegin () );
		const QList<QAction*>::const_iterator& itr_end ( sepActionList.constEnd () );
		while ( itr_start != itr_end )
			mContextMenu->removeAction ( (*(itr_start++)) );
		sepActionList.clear ();
	}

	const QModelIndex& index ( indexAt ( pos ) );
	// QModelIndex always returns the wrong row; namely one row further down than what it actually is.
	mContextMenuCell = sheetItem ( static_cast<uint>( index.row () - 1 ), static_cast<uint>( index.column () ) );
	if ( mContextMenuCell != nullptr )
	{
		if ( mContextMenuCell->widget () != nullptr )
		{
			QMenu* menu ( mContextMenuCell->widget ()->standardContextMenu () );
			if ( menu != nullptr )
			{
				sepActionList = menu->actions ();
				mContextMenu->addActions ( sepActionList );
			}
		}
		setCurrentItem ( mContextMenuCell, QItemSelectionModel::ClearAndSelect );
		enableOrDisableActionsForCell ( mContextMenuCell );
		mContextMenu->exec ( viewport ()->mapToGlobal ( pos ) );
		mContextMenuCell = nullptr;
	}
}

void vmTableWidget::keyPressEvent ( QKeyEvent* k )
{
	const bool ctrlKey ( k->modifiers () & Qt::ControlModifier );
	if ( ctrlKey )
	{
		bool b_accept ( false );
		if ( k->key () == Qt::Key_F )
		{
			if ( !m_searchPanel )
			{
				m_searchPanel = new vmTableSearchPanel ( this );
				m_searchPanel->setUtilityIndex ( addUtilityPanel ( m_searchPanel ) );
			}
			b_accept = toggleUtilityPanel ( m_searchPanel->utilityIndex () );
		}

		else if ( k->key () == Qt::Key_L )
		{
			if ( !m_filterPanel )
			{
				m_filterPanel = new vmTableFilterPanel ( this );
				m_filterPanel->setUtilityIndex ( addUtilityPanel ( m_filterPanel ) );
				if ( newView_func )
				{
					m_filterPanel->setCallbackForNewViewAfterFilter ( newView_func );
				}
			}
			b_accept = toggleUtilityPanel ( m_filterPanel->utilityIndex () );
		}

		else if ( k->key () == Qt::Key_Escape )
		{
			actionsBeforeClearing ();
			b_accept = true;
		}

		k->setAccepted ( b_accept );
		if ( b_accept )
			return;
	}

	if ( isEditable () )
	{
		if ( ctrlKey )
		{
			switch ( k->key () )
			{
				case Qt::Key_Enter:
				case Qt::Key_Return:
					if ( keypressed_func )
						keypressed_func ( k, this );
				break;
				case Qt::Key_I:
					insertRow_slot ();
				break;
				case Qt::Key_O:
					appendRow ();
				break;
				case Qt::Key_R:
					removeRow_slot ();
				break;
				case Qt::Key_D:
					clearRow_slot ();
				break;
				case Qt::Key_C:
					copyCellContents ();
				break;
				case Qt::Key_B:
					copyRowContents ();
				break;
				case Qt::Key_Z:
					undoChange ();
				break;
				default:
					k->setAccepted ( false );
				return;
			}
		}
		else
		{ // no CTRL modifier
			switch ( k->key () )
			{
				case Qt::Key_Enter:
				case Qt::Key_Return:
					if ( keypressed_func )
						keypressed_func ( k, this );
				break;
				case Qt::Key_Delete:
					removeRow_slot ();
				break;
				case Qt::Key_F2:
				case Qt::Key_F3:
				case Qt::Key_F4:
					if ( m_searchPanel && m_searchPanel->isVisible () )
					{
						QApplication::sendEvent ( m_searchPanel, k ); // m_searchPanel will propagate the event its children
						QApplication::sendPostedEvents ( m_searchPanel, 0 ); // force event to be processed before posting another one to que queue
					}
				break;
				default:
				break;
			}
		}
	}
	QTableWidget::keyPressEvent ( k );
}

void vmTableWidget::derivedClassCellContentChanged ( const vmTableItem* const ) {}
void vmTableWidget::derivedClassTableCleared () {}
void vmTableWidget::derivedClassTableUpdated () {}

void vmTableWidget::initList ()
{
	horizontalHeader ()->setVisible ( false );
	mTotalsRow = static_cast<int>( rowCount () );
	insertColumn ( 0 );
}

void vmTableWidget::initTable2 ()
{
	QFont titleFont ( font () );
	titleFont.setBold ( true );
	auto headerItem ( new vmCheckedTableItem ( Qt::Horizontal, this ) );
	headerItem->setCallbackForCheckStateChange ( [&] ( const uint col, const bool checked ) {
		return headerItemToggled ( col, checked ); } );
	setHorizontalHeader ( headerItem );

	uint i_col ( 0 );

	vmTableColumn* column ( nullptr );
	QString col_header;

	do
	{
		column = &mCols[i_col];
		insertColumn ( static_cast<int>( i_col ) );

		if ( !column->editable )
			setBit ( readOnlyColumnsMask, static_cast<uchar>( i_col ) ); // bit is set? read only cell : cell can be cleared or other actions

		setHorizontalHeaderItem ( static_cast<int>( i_col ), new vmTableItem ( column->label, this ) );

		uint colWidth ( column->width );
		if ( colWidth == 0 )
		{
			switch ( column->wtype )
			{
				case WT_DATEEDIT:
				case WT_TIMEEDIT:
					colWidth = 180;
				break;
				case WT_LINEEDIT:
					colWidth = ( column->text_type >= vmWidget::TT_PHONE ? 130 : 100 );
				break;
				case WT_LINEEDIT_WITH_BUTTON:
					colWidth = 150;
				break;
				default:
					colWidth = 100;
				break;
			}
		}
		setColumnWidth ( static_cast<int>( i_col ), static_cast<int>( colWidth ) );
		mPreferredSize.setWidth ( mPreferredSize.width () + colWidth );
		++i_col;
	}
	while ( i_col < m_ncols );

	mTotalsRow = static_cast<int>( visibleRows () - 1 );
	// If the table is a spreadsheet (not plain table) then the last row will be special and inserted in the code block below, after this line
	insertRow ( 0, isPlainTable () ? static_cast<uint>( visibleRows () ) : static_cast<uint>( totalsRow () ) );
	mPreferredSize.setHeight ( 2 * visibleRows () * rowHeight ( 0 ) );

	if ( !isPlainTable () )
	{
		QTableWidget::insertRow ( totalsRow () );
		for ( i_col = 0; i_col < colCount (); ++ i_col )
		{
			auto sheet_item ( new vmTableItem ( WT_LINEEDIT, i_col != 0 ? mCols[i_col].text_type : vmWidget::TT_TEXT, emptyString, this ) );
			setItem ( totalsRow (), static_cast<int>( i_col ), sheet_item );
			if ( i_col == 0 )
				sheet_item->setText ( TR_FUNC ( "Total:" ), false );
			else
			{
				if ( mCols[i_col].text_type >= vmLineEdit::TT_PRICE )
				{
					setCellWidget ( sheet_item );
					col_header = QChar ( 'A' + i_col );
					sheet_item->setFormula ( QLatin1String ( "sum " ) + col_header + CHR_ZERO + CHR_SPACE + col_header +
										 QLatin1String ( "%1" ), QString::number ( mTotalsRow - 1 ) );
				}
			}
			sheet_item->setEditable ( false );
		}
		//All the items that could not have their formula calculated at the time of creation will now
		for ( uint i_row ( 0 ); i_row < m_itemsToReScan.count (); ++i_row )
			m_itemsToReScan.at ( i_row )->targetsFromFormula ();
	}
}

void vmTableWidget::enableOrDisableActionsForCell ( const vmTableItem* sheetItem )
{
	mUndoAction->setEnabled ( sheetItem->cellIsAltered () );
	mCopyCellAction->setEnabled ( !isRowEmpty ( static_cast<uint>( sheetItem->row () ) ) );
	mCopyRowContents->setEnabled ( !isRowEmpty ( static_cast<uint>( sheetItem->row () ) ) );
	mAppendRowAction->setEnabled ( sheetItem->isEditable () );
	mInsertRowAction->setEnabled ( sheetItem->isEditable () );
	mDeleteRowAction->setEnabled ( sheetItem->isEditable () );
	mClearRowAction->setEnabled ( sheetItem->isEditable () );
	mClearTableAction->setEnabled ( sheetItem->isEditable () );
	mSubMenuFormula->setEnabled ( sheetItem->isEditable () );
	if ( this->isEditable () ) //even if the item is not editable, the formula might be changed
	{
		if ( mOverrideFormulaAction == nullptr )
		{
			mFormulaTitleAction = new vmAction ( FORMULA_TITLE );
			mOverrideFormulaAction = new vmAction ( SET_FORMULA_OVERRIDE, TR_FUNC ( "Allow formula override" ) );
			mOverrideFormulaAction->setCheckable ( true );
			connect ( mOverrideFormulaAction, &vmAction::triggered, this, [&, sheetItem ] ( const bool checked ) {
						setFormulaOverrideForCell ( const_cast<vmTableItem*>( sheetItem ), checked ); } );
			mSetFormulaAction = new vmAction ( SET_FORMULA, TR_FUNC ( "Set formula" ) );
			connect ( mSetFormulaAction, &vmAction::triggered, this, [&, sheetItem ] () {
						setFormulaForCell ( const_cast<vmTableItem*>( sheetItem ) ); } );
			mSubMenuFormula->addAction ( mFormulaTitleAction );
			mSubMenuFormula->addSeparator ();
			mSubMenuFormula->addAction ( mOverrideFormulaAction );
			mSubMenuFormula->addAction ( mSetFormulaAction );
		}
		mFormulaTitleAction->QAction::setText ( sheetItem->hasFormula () ? sheetItem->formula () : TR_FUNC ( "No formula" ) );
		mOverrideFormulaAction->setEnabled ( sheetItem->hasFormula () );
		mOverrideFormulaAction->setChecked ( sheetItem->formulaOverride () );
	}
}

void vmTableWidget::fixTotalsRow ()
{
	const QString totalsRowStr ( QString::number ( totalsRow () - 1 ) );
	vmTableItem* sheet_item ( nullptr );
	for ( uint i_col ( 0 ); i_col < colCount (); ++i_col )
	{
		sheet_item = sheetItem ( static_cast<uint>( totalsRow () ), i_col );
		if ( sheet_item->textType () >= vmLineEdit::TT_PRICE )
			sheet_item->setFormula ( sheet_item->formulaTemplate (), totalsRowStr );
	}
}

void vmTableWidget::reScanItem ( vmTableItem* const sheet_item )
{
	if ( m_itemsToReScan.contains ( sheet_item ) == -1 )
		m_itemsToReScan.append ( sheet_item );
}

void vmTableWidget::undoChange ()
{
	if ( isEditable () )
	{
		vmTableItem* item ( mContextMenuCell ? mContextMenuCell : dynamic_cast<vmTableItem*>( currentItem () ) );
		if ( item && item->cellIsAltered () )
			item->setText ( item->prevText (), true );
	}
}

void vmTableWidget::copyCellContents ()
{
	QString cell_text;
	if ( mContextMenuCell != nullptr )
		cell_text = mContextMenuCell->text ();
	else
	{
		const QModelIndexList& selIndexes ( selectedIndexes () );
		if ( selIndexes.count () > 0 )
		{
			QModelIndexList::const_iterator itr ( selIndexes.constBegin () );
			const QModelIndexList::const_iterator& itr_end ( selIndexes.constEnd () );
			for ( ; itr != itr_end; ++itr )
			{
				cell_text.append ( sheetItem ( static_cast<uint>( static_cast<QModelIndex>( *itr ).row () ),
											   static_cast<uint>( static_cast<QModelIndex>( *itr ).column () ) )->text () );
				cell_text.append ( CHR_NEWLINE );
			}
			cell_text.chop ( 1 ); // remove last newline character
		}
	}
	VM_LIBRARY_FUNCS::copyToClipboard ( cell_text );
}

void vmTableWidget::copyRowContents ()
{
	QString rowText;
	uint i_col ( 0 );
	if ( mContextMenuCell != nullptr )
	{
		for ( ; i_col < colCount (); ++i_col )
			rowText += sheetItem ( static_cast<uint>(mContextMenuCell->row ()), i_col )->text () + QLatin1Char ( '\t' );
		rowText.chop ( 1 );
	}
	else
	{
		const QModelIndexList& selIndexes ( selectedIndexes () );
		if ( selIndexes.count () > 0 )
		{
			QModelIndexList::const_iterator itr ( selIndexes.constBegin () );
			const QModelIndexList::const_iterator& itr_end ( selIndexes.constEnd () );
			for ( ; itr != itr_end; ++itr )
			{
				for ( i_col = 0; i_col < colCount (); ++i_col )
					rowText += sheetItem ( static_cast<uint>( static_cast<QModelIndex>( *itr ).row () ), i_col )->text () + QLatin1Char ( '\t' );
				rowText.chop ( 1 );
				rowText += CHR_NEWLINE;
			}
			rowText.chop ( 1 );
		}
	}

	VM_LIBRARY_FUNCS::copyToClipboard ( rowText );
}

// Inserts one or more lines ( depending on how many are selected - i.e. two selected, in whichever order, inserts two lines )
void vmTableWidget::insertRow_slot ()
{
	if ( mContextMenuCell != nullptr )
		insertRow ( static_cast<uint>( mContextMenuCell->row () ) );
	else
	{
		const QModelIndexList& selIndexes ( selectedIndexes () );
		if ( selIndexes.count () > 0 )
		{
			QModelIndexList::const_iterator itr ( selIndexes.constBegin () );
			const QModelIndexList::const_iterator& itr_end ( selIndexes.constEnd () );
			for ( ; itr != itr_end; ++itr )
				insertRow ( static_cast<uint>( static_cast<QModelIndex>( *itr ).row () ) + 1 );
		}
	}
}

void vmTableWidget::removeRow_slot ()
{
	if ( mContextMenuCell )
	{
		removeRow ( static_cast<uint>( mContextMenuCell->row () ) );
	}
	else
	{
		const QModelIndexList& selIndexes ( selectionModel ()->selectedRows () );
		if ( selIndexes.count () > 0 )
		{
			// Move from last selected toward the first to avoid conflicts and errors
			QModelIndexList::const_iterator itr ( selIndexes.constEnd () );
			const QModelIndexList::const_iterator& itr_begin ( selIndexes.constBegin () );
			for ( --itr; ; --itr )
			{
				removeRow ( static_cast<uint>( static_cast<QModelIndex>( *itr ).row () ) );
				if ( itr == itr_begin )
					break;
			}
		}
	}
}

void vmTableWidget::clearRow_slot ()
{
	if ( isEditable () )
	{
		if ( mContextMenuCell != nullptr )
			clearRow ( static_cast<uint>( mContextMenuCell->row () ) );
		else
		{
			const QModelIndexList& selIndexes ( selectedIndexes () );
			if ( selIndexes.count () > 0 )
			{
				QModelIndexList::const_iterator itr ( selIndexes.constBegin () );
				const QModelIndexList::const_iterator& itr_end ( selIndexes.constEnd () );
				for ( ; itr != itr_end; ++itr )
					clearRow ( static_cast<uint>( static_cast<QModelIndex>( *itr ).row () ) );
			}
		}
	}
}

void vmTableWidget::clearTable_slot ()
{
	if ( isEditable () )
		clear ( true );
}

void vmTableWidget::setFormulaOverrideForCell ( vmTableItem* item, const bool boverride )
{
	const bool bWasOverride ( item->formulaOverride () );
	item->setFormulaOverride ( boverride );
	if ( bWasOverride )
		item->computeFormula (); // change cell value to that of the formula
	item->setEditable ( true ); //temporarily editable
}

void vmTableWidget::setFormulaForCell ( vmTableItem* item )
{
	if ( item )
		 return;
	//TODO TODO
	QString strNewFormula;
	if ( vmNotify::inputBox ( strNewFormula, this, TR_FUNC ( "New Formula" ), TR_FUNC ( "Set new cell formula" ),
							  item->formula () ) ) {
		if ( item->setCustomFormula ( strNewFormula ) )
			mFormulaTitleAction->QAction::setText ( item->formula () );
	}
}

void vmTableWidget::headerItemToggled ( const uint col, const bool checked )
{
	qDebug () << "Column " << col << " is now " << ( checked ? "checked." : "unchecked." );
}

void vmTableWidget::textWidgetChanged ( const vmWidget* const sender )
{
	auto const item ( const_cast<vmTableItem*>( sender->ownerItem () ) );
	item->setText ( dynamic_cast<const vmLineEdit* const>( sender )->text (), false, true );
	cellContentChanged ( item );
}

void vmTableWidget::dateWidgetChanged ( const vmWidget* const sender )
{
	const_cast<vmTableItem*>( sender->ownerItem () )->setText (
		dynamic_cast<const vmDateEdit* const>( sender )->date ().toString ( DATE_FORMAT_DB ), false, true  );
	cellContentChanged ( sender->ownerItem () );
}

void vmTableWidget::timeWidgetChanged ( const vmWidget* const sender )
{
	const_cast<vmTableItem*>( sender->ownerItem () )->setText (
		dynamic_cast<const vmTimeEdit* const>( sender )->time ().toString ( TIME_FORMAT_DEFAULT ), false, true );
	cellContentChanged ( sender->ownerItem () );
}

void vmTableWidget::checkBoxToggled ( const vmWidget* const sender )
{
	const_cast<vmTableItem*>( sender->ownerItem () )->setText ( sender->text (), false, true );
	cellContentChanged ( sender->ownerItem () );
}

void vmTableWidget::cellWidgetRelevantKeyPressed ( const QKeyEvent* const ke, const vmWidget* const widget )
{
	if ( ke->key () == Qt::Key_Escape )
	{
		/* The text is different from the original, canceling (pressing the ESC key) should return
		 * the widget's text to the original
		 */
		auto item ( const_cast<vmTableItem*>( widget->ownerItem () ) );
		if ( item->text () != item->originalText () )
		{
			item->setText ( item->originalText (), false );
			item->setCellIsAltered ( false );
		}
		/* If text is the same the item had when it went from non-editable to editable,
		 * propagate the signal to the table so that it can do the appropriate cancel editing routine
		 */
		else
			keyPressEvent ( const_cast<QKeyEvent*>( ke ) );
	}
}
