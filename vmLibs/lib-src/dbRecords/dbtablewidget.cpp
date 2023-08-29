#include "dbtablewidget.h"
#include "dbrecord.h"
#include "dbsupplies.h"
#include "completers.h"

constexpr uint PURCHASES_TABLE_REG_COL ( 6 );
constexpr auto PURCHASES_TABLE_COLS ( 7 );
static const char* const PROPERTY_TABLE_HAS_ITEM_TO_REGISTER ( "pthitr" );

vmCompleters* dbTableWidget::completer_manager ( new vmCompleters ( false ) );

dbTableWidget::dbTableWidget ( QWidget* parent )
	: vmTableWidget ( parent ), mbDoNotUpdateCompleters ( false )
{}

dbTableWidget::dbTableWidget ( const uint rows, QWidget* parent )
	: vmTableWidget ( rows, parent ), mbDoNotUpdateCompleters ( false )
{}

dbTableWidget::~dbTableWidget () {}

void dbTableWidget::setCompleterManager ( vmCompleters* const completer )
{
	delete dbTableWidget::completer_manager;
	dbTableWidget::completer_manager = completer;
}

void dbTableWidget::setIgnoreChanges ( const bool b_ignore )
{
	if ( colCount () == PURCHASES_TABLE_COLS )
	{
		if ( !b_ignore )
		{
			static_cast<void>( connect ( completerManager ()->getCompleter ( PRODUCT_OR_SERVICE ),
					  static_cast<void (QCompleter::*)( const QModelIndex& )>( &QCompleter::activated ),
					this, ( [&, this] ( const QModelIndex& index ) { return interceptCompleterActivated ( index );
			} ) ) );
		}
		else
			static_cast<void>( disconnect ( completerManager ()->getCompleter ( PRODUCT_OR_SERVICE ), nullptr, nullptr, nullptr ) );
	}
	vmTableWidget::setIgnoreChanges ( b_ignore );
}

void dbTableWidget::interceptCompleterActivated ( const QModelIndex& index )
{
	const QCompleter* const prod_completer ( completerManager ()->getCompleter ( PRODUCT_OR_SERVICE ) );
	if ( prod_completer->widget ()->parentWidget ()->parentWidget () != this )
		return;

	mbDoNotUpdateCompleters = true; //avoid a call to update the item's completer because we are retrieving the values from the completer itself
	const stringRecord record ( prod_completer->completionModel ()->data ( index.sibling ( index.row (), 1 ) ).toString () );

	if ( record.isOK () )
	{
		if ( currentRow () == -1 )
			setCurrentCell ( 0, 0, QItemSelectionModel::ClearAndSelect );
		if ( currentRow () >= 0 )
		{
			const auto current_row ( static_cast<uint>( currentRow () ) );
			if ( record.first () )
			{
				uint i_col ( 0 );
				do
				{
					sheetItem ( current_row, i_col )->setText ( record.curValue (), true );
				} while ( (++i_col < ISR_TOTAL_PRICE) && record.next () );
				setCurrentCell ( static_cast<int>( current_row ), ISR_QUANTITY, QItemSelectionModel::ClearAndSelect );
			}
		}
	}
	mbDoNotUpdateCompleters = false;
}

void dbTableWidget::exportPurchaseToSupplies ( const DBRecord* const src_dbrec, dbSupplies* const dst_dbrec )
{
	if ( !property ( PROPERTY_TABLE_HAS_ITEM_TO_REGISTER ).toBool () )
		return;

	auto s_row ( new spreadRow );
	s_row->column[0] = dst_dbrec->isrRecordField ( ISR_NAME );
	s_row->column[1] = dst_dbrec->isrRecordField ( ISR_UNIT );
	s_row->column[2] = dst_dbrec->isrRecordField ( ISR_BRAND );
	s_row->column[3] = dst_dbrec->isrRecordField ( ISR_UNIT_PRICE );
	s_row->column[4] = dst_dbrec->isrRecordField ( ISR_SUPPLIER );
	s_row->column[5] = dst_dbrec->isrRecordField ( ISR_DATE );

	s_row->field_value[s_row->column[4]] = recStrValue ( src_dbrec, src_dbrec->isrRecordField ( ISR_SUPPLIER ) );
	s_row->field_value[s_row->column[5]] = recStrValue ( src_dbrec, src_dbrec->isrRecordField ( ISR_DATE ) );

	for ( uint i_row ( 0 ); i_row <= static_cast<uint>( lastUsedRow () ); ++i_row )
	{
		if ( sheetItem ( i_row, PURCHASES_TABLE_REG_COL )->text () != CHR_ONE ) // item is not marked to be exported
			continue;

		s_row->field_value[s_row->column[0]] = sheetItem ( i_row, ISR_NAME )->text ();
		s_row->field_value[s_row->column[1]] = sheetItem ( i_row, ISR_UNIT )->text ();
		s_row->field_value[s_row->column[2]] = sheetItem ( i_row, ISR_BRAND )->text ();
		s_row->field_value[s_row->column[3]] = sheetItem ( i_row, ISR_UNIT_PRICE )->text ();

		if ( dst_dbrec->readRecord ( s_row->column[0], s_row->field_value[s_row->column[0]] ) ) //item description
		{
			if ( recStrValue ( dst_dbrec, s_row->column[1] ) == s_row->field_value[s_row->column[1]] ) // item unit
			{
				if ( recStrValue ( dst_dbrec, s_row->column[2] ) == s_row->field_value[s_row->column[2]] ) // item brand
				{
					if ( recStrValue ( dst_dbrec, s_row->column[4] ) == s_row->field_value[s_row->column[4]] ) // item supplier
					{
						if ( recStrValue ( dst_dbrec, s_row->column[3] ) == s_row->field_value[s_row->column[3]] ) // item price
						{
							if ( recStrValue ( dst_dbrec, s_row->column[5] ) == s_row->field_value[s_row->column[5]] ) // item date
								continue; // everything is the same, do nothing
						}
						dst_dbrec->setAction ( ACTION_EDIT ); // either price or date are different: edit to update the info
					}
				}
			}
		}
		if ( dst_dbrec->action () == ACTION_READ )
		{
			dst_dbrec->setAction ( ACTION_ADD ); //one single piece of different information (price and date excluded ) and we add the new info
			setRecValue ( dst_dbrec, s_row->column[0], s_row->field_value[s_row->column[0]] );
			setRecValue ( dst_dbrec, s_row->column[1], s_row->field_value[s_row->column[1]] );
			setRecValue ( dst_dbrec, s_row->column[2], s_row->field_value[s_row->column[2]] );
			setRecValue ( dst_dbrec, s_row->column[4], s_row->field_value[s_row->column[4]] );
		}

		// only update is on either PRICE or DATE when editing.
		setRecValue ( dst_dbrec, s_row->column[3], s_row->field_value[s_row->column[3]] );
		setRecValue ( dst_dbrec, s_row->column[5], s_row->field_value[s_row->column[5]] );
		dst_dbrec->saveRecord ();
	}
	delete s_row;
}

dbTableWidget* dbTableWidget::createPurchasesTable ( dbTableWidget* table, QWidget* parent )
{
	vmTableColumn* cols ( table->createColumns ( PURCHASES_TABLE_COLS ) );
	uint i ( 0 );
	for ( ; i < PURCHASES_TABLE_COLS; ++i )
	{
		switch ( i )
		{
			case ISR_NAME:
				cols[ISR_NAME].completer_type = PRODUCT_OR_SERVICE;
				cols[ISR_NAME].width = 280;
				cols[ISR_NAME].label = TR_FUNC ( "Item" );
			break;
			case ISR_UNIT:
				cols[ISR_UNIT].label = TR_FUNC ( "Unit" );
			break;
			case ISR_BRAND:
				cols[ISR_BRAND].completer_type = BRAND;
				cols[ISR_BRAND].text_type = vmWidget::TT_UPPERCASE;
				cols[ISR_BRAND].label = TR_FUNC ( "Maker" );
			break;
			case ISR_QUANTITY:
				cols[ISR_QUANTITY].text_type = vmLineEdit::TT_DOUBLE;
				cols[ISR_QUANTITY].label = TR_FUNC ( "Quantity" );
			break;
			case ISR_UNIT_PRICE:
				cols[ISR_UNIT_PRICE].text_type = vmLineEdit::TT_PRICE;
				cols[ISR_UNIT_PRICE].button_type = vmLineEditWithButton::LEBT_CALC_BUTTON;
				cols[ISR_UNIT_PRICE].label = TR_FUNC ( "Unit price" );
			break;
			case ISR_TOTAL_PRICE:
				cols[ISR_TOTAL_PRICE].text_type = vmLineEdit::TT_PRICE;
				cols[ISR_TOTAL_PRICE].formula = QStringLiteral ( "* D%1 E%1" );
				cols[ISR_TOTAL_PRICE].editable = false;
				cols[ISR_TOTAL_PRICE].label = TR_FUNC ( "Total price" );
			break;
			case PURCHASES_TABLE_REG_COL:
				cols[PURCHASES_TABLE_REG_COL].wtype = WT_CHECKBOX;
				cols[PURCHASES_TABLE_REG_COL].width = 50;
				cols[PURCHASES_TABLE_REG_COL].default_value = CHR_ZERO;
				cols[PURCHASES_TABLE_REG_COL].label = TR_FUNC ( "Register" );
			break;
		}
	}

	if ( table )
	{
		table->setCallbackForSettingCompleterForWidget ( [&] ( vmWidget* widget, const int completer_type )
				{ return completerManager ()->setCompleterForWidget ( widget, completer_type ); } );
		table->initTable ( 10 );
	}
	else
		table = new dbTableWidget ( 10, parent );

	table->setKeepModificationRecords ( false );
	table->insertMonitoredCell ( static_cast<uint>( table->totalsRow () ), ISR_TOTAL_PRICE );
	return table;
}

dbTableWidget* dbTableWidget::createPayHistoryTable ( dbTableWidget* table, QWidget* parent,
		const PAY_HISTORY_RECORD last_column )
{
	vmTableColumn* cols ( table->createColumns ( last_column + 1 ) );

	for ( uint i ( 0 ); i <= last_column; ++i )
	{
		switch ( i )
		{
			case PHR_DATE:
				cols[PHR_DATE].label = TR_FUNC ( "Date" );
				cols[PHR_DATE].wtype = WT_DATEEDIT;
			break;
			case PHR_VALUE:
				cols[PHR_VALUE].label = TR_FUNC ( "Value" );
				cols[PHR_VALUE].button_type = vmLineEditWithButton::LEBT_CALC_BUTTON;
				cols[PHR_VALUE].text_type = vmLineEdit::TT_PRICE;
				cols[PHR_VALUE].wtype = WT_LINEEDIT_WITH_BUTTON;
			break;
			case PHR_PAID:
				cols[PHR_PAID].label = TR_FUNC ( "Paid?" );
				cols[PHR_PAID].wtype = WT_CHECKBOX;
				cols[PHR_PAID].width = 50;
				//cols[PHR_PAID].default_value = CHR_ZERO;
			break;
			case PHR_METHOD:
				cols[PHR_METHOD].label = TR_FUNC ( "Method" );
				cols[PHR_METHOD].completer_type = PAYMENT_METHOD;
				cols[PHR_METHOD].text_type = vmWidget::TT_UPPERCASE;
				cols[PHR_METHOD].width = 200;
			break;
			case PHR_ACCOUNT:
				cols[PHR_ACCOUNT].label = TR_FUNC ( "Account" );
				cols[PHR_ACCOUNT].completer_type = ACCOUNT;
				cols[PHR_ACCOUNT].width = 120;
			break;
			case PHR_USE_DATE:
				cols[PHR_USE_DATE].label = TR_FUNC ( "Use Date" );
				cols[PHR_USE_DATE].wtype = WT_DATEEDIT;
			break;
		}
	}

	table->setCallbackForSettingCompleterForWidget ( [&] ( vmWidget* widget, const int completer_type ) {
				return completerManager ()->setCompleterForWidget ( widget, completer_type ); } );
	if ( table )
		table->initTable ( 5 );
	else
		table = new dbTableWidget ( 5, parent );

	table->setKeepModificationRecords ( false );
	/* insertMonitoredCell () assumes the totals row and columns are monitored. Se we setup the callback to nullptr
	 * to avoid the c++ lib to throw a std::bad_function_call. Any pay history object that wishes to monitor changes to
	 * those cells must simply call setCallbackForMonitoredCellChanged () when setting up the table right after the call
	 * to this function.
	 */
	table->setCallbackForMonitoredCellChanged ( [&] ( const vmTableItem* const ) { return nullptr; } );
	table->insertMonitoredCell ( static_cast<uint>( table->totalsRow () ), PHR_VALUE );
	return table;
}

void dbTableWidget::derivedClassCellContentChanged ( const vmTableItem* const item )
{
	/*if ( !mbDoNotUpdateCompleters )
	{
		if ( item->completerType () >= 0 ) // TODO have a way to undo in case the text is wrong, altered, invalid, etc. Or to not have this method at all
		{
			// Update the runtime completers to make the entered text available for the current session, if not already in the model
			// The runtime completer will, in turn, update the database.
			completerManager ()->updateCompleter ( item->text (), static_cast<COMPLETER_CATEGORIES>( item->completerType () ) );
		}
	}*/

	const vmTableItem* extra_item ( sheetItem ( static_cast<uint>( item->row () ), PURCHASES_TABLE_REG_COL ) );
	if ( extra_item ) // only purchases tables have it
	{
		if ( extra_item->text () == CHR_ONE )
			setProperty ( PROPERTY_TABLE_HAS_ITEM_TO_REGISTER, true );
	}
}

void dbTableWidget::derivedClassTableCleared ()
{
	setProperty ( PROPERTY_TABLE_HAS_ITEM_TO_REGISTER, false );
}

void dbTableWidget::derivedClassTableUpdated ()
{
	setProperty ( PROPERTY_TABLE_HAS_ITEM_TO_REGISTER, false );
}
