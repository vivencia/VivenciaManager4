#include "vmcheckedtableitem.h"

#include <vmUtils/fast_library_functions.h>

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

vmCheckedTableItem::vmCheckedTableItem ( const Qt::Orientation orientation, QWidget* parent )
	: QHeaderView ( orientation, parent ), checkChange_func ( nullptr), mColumnState ( 0 ), mbIsCheckable ( false )
{
	// set clickable by default
	setSectionsClickable ( true );
}

void vmCheckedTableItem::paintSection ( QPainter* painter, const QRect& rect, const int logicalIndex ) const
{
	if ( logicalIndex >= 0 )
	{
		if ( !mbIsCheckable )
			QHeaderView::paintSection ( painter, rect, logicalIndex );
		else
		{
			painter->save ();
			QHeaderView::paintSection ( painter, rect, logicalIndex );
			painter->restore ();
			QStyleOptionButton option;
			if ( isEnabled () )
				option.state |= QStyle::State_Enabled;
			option.rect = checkBoxRect ( rect );
			if ( isBitSet ( mColumnState, static_cast<uchar>(logicalIndex) ) )
				option.state |= QStyle::State_On;
			else
				option.state |= QStyle::State_Off;
			style ()->drawControl ( QStyle::CE_CheckBox, &option, painter );
		}
	}
}

void vmCheckedTableItem::mousePressEvent ( QMouseEvent* event )
{
	if ( mbIsCheckable )
	{
		const int column ( logicalIndexAt ( event->pos () ) );
		if ( column >= 0 )
			setChecked ( static_cast<uint>(column), !isBitSet ( mColumnState, static_cast<uchar>(column) ), true );
	}
	else
		QHeaderView::mousePressEvent ( event );
}

void vmCheckedTableItem::setCheckable ( const bool checkable )
{
	mbIsCheckable = checkable;
	// repaint (), update (), qApp->flush (), qApp->processEvents () .. nothing works but this
	update ();
	//parentWidget ()->resize ( parentWidget ()->width () + 1 , parentWidget ()->height () );
}

bool vmCheckedTableItem::isChecked ( const uint column ) const
{
	// when not checkable, all columns are considered checked
	return mbIsCheckable ? isBitSet ( mColumnState, static_cast<uchar>(column) ) : true;
}

void vmCheckedTableItem::setChecked ( const uint column, const bool checked, const bool b_notify )
{
	if ( isEnabled () && ( isBitSet ( mColumnState, static_cast<uchar>(column) ) != checked ) )
	{
		checked ? setBit ( mColumnState, static_cast<uchar>(column) ) : unSetBit ( mColumnState, static_cast<uchar>(column) );
		updateSection ( static_cast<int>(column) );
		if ( b_notify && checkChange_func )
			checkChange_func ( column, checked );
	}
}

QRect vmCheckedTableItem::checkBoxRect ( const QRect& sourceRect ) const
{
	QStyleOptionButton checkBoxStyleOption;
	const QRect checkBoxRect ( style ()->subElementRect ( QStyle::SE_CheckBoxIndicator, &checkBoxStyleOption ) );
	const QPoint checkBoxPoint ( sourceRect.x () + 2,
								 sourceRect.y () + sourceRect.height () / 2 -
								 checkBoxRect.height () / 2 );
	return { checkBoxPoint, checkBoxRect.size () };
}
