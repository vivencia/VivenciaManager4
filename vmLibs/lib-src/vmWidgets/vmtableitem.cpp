#include "vmtableitem.h"
#include "heapmanager.h"
#include "vmtablewidget.h"
#include "vmwidgets.h"

auto vmColorToQt = [] ( const VMColors color ) -> Qt::GlobalColor
	{	switch ( color ) { case	vmGray: return Qt::gray; case vmRed: return Qt::red; case vmYellow: return Qt::yellow;
			case vmGreen: return Qt::green; case vmBlue: return Qt::blue; default: return Qt::white; } };

static const QString SUM ( QStringLiteral ( "sum" ) );

void table_item_swap ( vmTableItem& t_item1, vmTableItem& t_item2 )
{
	vmwidget_swap ( static_cast<vmWidget&>( t_item1 ), static_cast<vmWidget&>( t_item2 ) );
	
	using std::swap;
	swap ( t_item1.m_wtype, t_item2.m_wtype );
	swap ( t_item1.m_btype, t_item2.m_btype );
	swap ( t_item1.mb_hasFormula, t_item2.mb_hasFormula );
	swap ( t_item1.mb_formulaOverride, t_item2.mb_formulaOverride );
	swap ( t_item1.mb_customFormula, t_item2.mb_customFormula );
	swap ( t_item1.mb_CellAltered, t_item2.mb_CellAltered );
	swap ( t_item1.mStr_Formula, t_item2.mStr_Formula );
	swap ( t_item1.mStr_FormulaTemplate, t_item2.mStr_FormulaTemplate );
	swap ( t_item1.mStrOp, t_item2.mStrOp );
	swap ( t_item1.mDefaultValue, t_item2.mDefaultValue );
	swap ( t_item1.mCache, t_item2.mCache );
	swap ( t_item1.mprev_datacache, t_item2.mprev_datacache );
	swap ( t_item1.mBackupData_cache, t_item2.mBackupData_cache );
	swap ( t_item1.m_table, t_item2.m_table );
	pointersList<vmTableItem*>::vmList_swap ( t_item1.m_targets, t_item2.m_targets );
	swap ( t_item1.m_widget, t_item2.m_widget );
}

static void decode_pos ( const QString& pos, int* const row, int* const col )
{
	if ( pos.isEmpty () )
	{
		*col = -1;
		*row = -1;
	}
	else
	{
		*col = pos.at ( 0 ).toLatin1 () - 'A';
		*row = pos.rightRef ( pos.size () - 1 ).toInt ();
	}
}

vmTableItem::vmTableItem ()
	: QTableWidgetItem (), vmWidget ( WT_TABLE_ITEM ),
	  m_wtype ( WT_WIDGET_UNKNOWN ), m_btype ( vmLineEditWithButton::LEBT_NO_BUTTON ),
	  mb_hasFormula ( false ), mb_formulaOverride ( false ), mb_customFormula ( false ),
	  mb_CellAltered ( false ), mDefaultValue (), mCache (), m_table ( nullptr ),
	  m_targets ( 4 ), m_widget ( nullptr )
{}

vmTableItem::vmTableItem ( const PREDEFINED_WIDGET_TYPES wtype,
						   const vmLineEdit::TEXT_TYPE ttype, const QString& text, const vmTableWidget* table )
	: vmTableItem ()
{
	m_table = const_cast<vmTableWidget*>( table );
	setWidgetType ( wtype );
	setTextType ( ttype );
	setDefaultValue ( text );
	setText ( text, false, false, false );
	//setMemoryTextOnly ( text ); //TEST
}

vmTableItem::vmTableItem (const QString& text , const vmTableWidget *table )
	: vmTableItem ()
{
	m_table = const_cast<vmTableWidget*>( table );
	setText	( text, false, false, false );
	setDefaultValue ( text );
	setTextType ( vmLineEdit::TT_TEXT );
}

void vmTableItem::copy ( const vmTableItem& src_item )
{
	m_wtype = src_item.m_wtype;
	m_btype = src_item.m_btype;
	mb_hasFormula = src_item.mb_hasFormula;
	mb_formulaOverride = src_item.mb_formulaOverride;
	mb_customFormula = src_item.mb_customFormula;
	mb_CellAltered = src_item.mb_CellAltered;
	mStr_Formula = src_item.mStr_Formula;
	mStr_FormulaTemplate = src_item.mStr_FormulaTemplate;
	mStrOp = src_item.mStrOp;
	mDefaultValue = src_item.mDefaultValue;
	mCache = src_item.mCache;
	mprev_datacache = src_item.mprev_datacache;
	mBackupData_cache = src_item.mBackupData_cache;
	m_table = src_item.m_table;
	m_targets = src_item.m_targets;
	m_widget = src_item.m_widget;
}

vmTableItem::~vmTableItem () {}

const QString vmTableItem::defaultStyleSheet () const
{
	QString colorstr;
	if ( !table () )
		colorstr = QStringLiteral ( "#ffffff" );
	else
		colorstr = table ()->palette ().color ( QPalette::AlternateBase ).name ();
	return colorstr;
}

void vmTableItem::setEditable ( const bool editable )
{
	if ( m_widget )
		m_widget->setEditable ( editable );
	else
	{
		Qt::ItemFlags currentFlags ( flags () );
		if ( editable )
		{
			currentFlags |= Qt::ItemIsEditable;
		}
		else
			currentFlags ^= Qt::ItemIsEditable;
		QTableWidgetItem::setFlags ( currentFlags );
		
	}
	vmWidget::setEditable ( editable );
}

void vmTableItem::setText ( const QString& text, const bool b_notify, const bool b_from_cell_itself, const bool b_formulaResult )
{
	if ( b_formulaResult )
	{
		 if ( formulaOverride () )
			return;
	}
	else
	{
		if ( hasFormula () && b_from_cell_itself )
			setFormulaOverride ( true );
	}

	if ( !isEditable () )
	{
		mBackupData_cache = text;
		mprev_datacache = text;
	}
	else
	{
		mb_CellAltered = true;
		mprev_datacache = mCache;
	}

	mCache = text;
	if ( table () )
	{
		table ()->setLastUsedRow ( row () );
		table ()->resizeColumn ( static_cast<uint>( column () ), text );
	}

	if ( m_widget )
	{
		for ( uint i ( 0 ); i < m_targets.count (); ++i )
			m_targets.at ( i )->computeFormula ();

		// The call to change the widget's text must be the last operation so that callbackers can
		// take the item's text (if so they wish) when there is a signal call by the widget, and get
		// the updated value, instead of the old one
		if ( !b_from_cell_itself )
			m_widget->setText ( text, b_notify );
	}
	else
	{
		if ( table () )
			b_notify ? table ()->setSimpleCellText ( this ) : table ()->setSimpleCellTextWithoutNotification ( this, text );
		else
			QTableWidgetItem::setText ( text );
	}
}

void vmTableItem::setDate ( const vmNumber& date )
{
	if ( m_widget->type () == WT_DATEEDIT )
		dynamic_cast<vmDateEdit*>( m_widget )->setDate ( date, isEditable () );
}

vmNumber vmTableItem::date ( const bool bCurText ) const
{
	if ( m_widget->type () == WT_DATEEDIT )
	{
		return bCurText ? vmNumber ( dynamic_cast<vmDateEdit*>( m_widget )->date () ) :
					vmNumber ( originalText (), VMNT_DATE, vmNumber::VDF_HUMAN_DATE );
	}
	return vmNumber::emptyNumber;
}

static VM_NUMBER_TYPE textTypeToNbrType ( const vmLineEdit::TEXT_TYPE tt )
{
	switch ( tt )
	{
		case vmWidget::TT_PRICE:	return VMNT_PRICE;
		case vmWidget::TT_DOUBLE:	return VMNT_DOUBLE;
		case vmWidget::TT_PHONE:	return VMNT_PHONE;
		case vmWidget::TT_INTEGER:	return VMNT_INT;
		case vmWidget::TT_TEXT:
		case vmWidget::TT_ZIPCODE:
		case vmWidget::TT_UPPERCASE:
									return VMNT_UNSET;
	}
	return VMNT_UNSET;
}

vmNumber vmTableItem::number ( const bool bCurText ) const
{
	if ( textType () >= vmLineEdit::TT_PHONE ) // ignore the text types (TT_TEXT and TT_UPPERCASE, so far)
		return vmNumber ( bCurText ? text () : originalText (), textTypeToNbrType ( textType () ), 1 );
	return vmNumber::emptyNumber;
}

void vmTableItem::highlight ( const VMColors color, const QString& )
{
	if ( m_widget )
	{
		if ( m_widget->id () >= 0 )
			m_widget->highlight ( color );
	}
	else
	{
		//setBackground triggers the signal QTableWidget::itemChanged. We have to avoid that
		const bool b_ignorechanges ( table ()->isIgnoringChanges () );
		if ( !b_ignorechanges )
			table ()->setIgnoreChanges ( true );

		QColor new_color;
		switch ( color )
		{
			case vmDefault_Color:
				new_color = QColor ( table ()->palette ().color ( QPalette::Base ).name () );
			break;
			case vmDefault_Color_Alternate:
				new_color = QColor ( table ()->palette ().color ( QPalette::AlternateBase ).name () );
			break;
			default:
				new_color = QColor ( vmColorToQt ( color ) );
		}
		setBackground ( new_color );
		if ( !b_ignorechanges )
			table ()->setIgnoreChanges ( false );
	}
}

QVariant vmTableItem::data ( const int role ) const
{
	switch ( role )
	{
		case Qt::DisplayRole:
		case Qt::StatusTipRole:
		case Qt::EditRole:
		//Only widgets themselves should display their data. Otherwise both the widgets and the table item would
		//have their display contents drawn (and therefore calculated, even if not properly displayed for being behind
		//the widget).
		if ( ownerItem () != nullptr )
				return mCache;
		
		default:
			return QTableWidgetItem::data ( role );
	}
}

void vmTableItem::targetsFromFormula ()
{
	const QStringList list ( mStr_Formula.split ( CHR_SPACE ) );
	if ( list.isEmpty () )
		return;

	if ( list.count () > 1 )
	{
		vmTableItem* sheet_item ( nullptr );
		int firstRow ( 0 );
		int firstCol ( 0 );
		int secondRow ( 0 );
		int secondCol ( 0 );
		decode_pos ( list.value ( 1 ), &firstRow, &firstCol );
		decode_pos ( list.value ( 2 ), &secondRow, &secondCol );

		mStrOp = list.value ( 0 ).toLower ();

		if ( mStrOp == SUM )
		{
			for ( int r ( firstRow ); r <= secondRow; ++r )
			{
				for ( int c ( firstCol ); c <= secondCol; ++c )
				{
					sheet_item = table ()->sheetItem ( static_cast<uint>( r ), static_cast<uint>( c ) );
					if ( sheet_item )
					{
						if ( textType () >= vmLineEdit::TT_PRICE )
						{
							if ( sheet_item->m_targets.contains ( this ) == -1 )
								sheet_item->m_targets.append ( this );
						}
					}
					else
					{
						/* This cell depends on value of some cell that is not created yet
						 * We can break the calculation now, because it is pointless, and resume
						 * at a later point, but never during the actual use of a table. All this
						 * means is it takes a little longer to show the table, but its operation is smoother
						 */
						table ()->reScanItem ( this );
						return;
					}
				}
			}
		}
		else
		{
			sheet_item = table ()->sheetItem ( static_cast<uint>( firstRow ), static_cast<uint>( firstCol ) );
			if ( sheet_item )
			{
				if ( sheet_item->m_targets.contains ( this ) == -1 )
					sheet_item->m_targets.append ( this );
				if ( secondRow != -1 )
				{
					sheet_item = table ()->sheetItem ( static_cast<uint>( secondRow ), static_cast<uint>( secondCol ) );
					if ( sheet_item )
					{
						if ( sheet_item->m_targets.contains ( this ) == -1 )
							sheet_item->m_targets.append ( this );
					}
				}
			}
			else
			{
				table ()->reScanItem ( this );
				return;
			}
		}
	}
	computeFormula ();
}

void vmTableItem::setFormula ( const QString& formula_template,
							   const QString& firstValue, const QString& secondValue )
{
	mStr_FormulaTemplate = formula_template;
	mStr_Formula = secondValue.isEmpty () ? formula_template.arg ( firstValue ) : formula_template.arg ( firstValue, secondValue );
	mb_hasFormula = true;
	setData ( Qt::ToolTipRole, mStr_Formula );
	targetsFromFormula ();
}

bool vmTableItem::setCustomFormula ( const QString& strFormula )
{
	//TODO TODO TODO TODO TODO
	mb_customFormula = true;
	mStr_Formula = strFormula;
	mb_hasFormula = true;
	setData ( Qt::ToolTipRole, mStr_Formula );
	targetsFromFormula ();
	return true;
}

void vmTableItem::computeFormula ()
{
	const QStringList list ( mStr_Formula.split ( CHR_SPACE ) );
	int firstRow ( 0 );
	int firstCol ( 0 );
	int secondRow ( 0 );
	int secondCol ( 0 );
	decode_pos ( list.value ( 1 ), &firstRow, &firstCol );
	decode_pos ( list.value ( 2 ), &secondRow, &secondCol );

	vmNumber res;
	if ( mStrOp == SUM )
	{
		vmTableItem* tableItem ( nullptr );
		for ( int r ( firstRow ); r <= secondRow; ++r )
		{
			for ( int c ( firstCol ); c <= secondCol; ++c )
			{
				tableItem = table ()->sheetItem ( static_cast<uint>( r ), static_cast<uint>( c ) );
				if ( tableItem )
				{
					if ( textType () >= vmLineEdit::TT_PRICE )
						res += tableItem->number ();
				}
			}
		}
	}
	else
	{
		vmNumber firstVal ( table ()->sheetItem ( static_cast<uint>( firstRow ), static_cast<uint>( firstCol ) )->number () );
		vmNumber secondVal;
		if ( secondRow != -1 )
			secondVal = table ()->sheetItem ( static_cast<uint>( secondRow ), static_cast<uint>( secondCol ) )->number ();

		switch ( mStrOp.constData ()->toLatin1 () )
		{
			case '+':
				res = ( firstVal + secondVal ); break;
			case '-':
				res = ( firstVal - secondVal ); break;
			case '*':
				res = ( firstVal * secondVal ); break;
			case '/':
				res = ( firstVal / secondVal ); break;
			case '=':
				res = firstVal;	break;
			default: break;
		}
	}

	/*
	 * Force the widget to change its text and emit a signal to notify the callback,
	 * but only do that when editing the table. Changes when the table is read-only must not
	 * be captured. Since most formula cells are read-only, we need to check the table's
	 * state for an accurate position on the editing status.
	 */
	setText ( res.toString (), table ()->isEditable (), false, true );
}
