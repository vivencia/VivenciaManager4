#include "vmlistitem.h"
#include "vmlistwidget.h"

#include <QtWidgets/QApplication>

void list_item_swap ( vmListItem& item1, vmListItem& item2 )
{
	table_item_swap ( static_cast<vmTableItem&>( item1 ), static_cast<vmTableItem&>( item2 ) );
	
	using std::swap;
	swap ( item1.mbInit, item2.mbInit );
	swap ( item1.m_list, item2.m_list );
	swap ( item1.mbSorted, item2.mbSorted );
}

vmListItem::vmListItem ()
	: vmTableItem (), mbInit ( true ), m_list ( nullptr ), mbSorted ( false )
{
	setFlags ( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren );
	mbInit = false;
}

void vmListItem::copy ( const vmListItem& src_item )
{
	this->vmTableItem::copy ( src_item );
	mbInit = src_item.mbInit;
	mbSorted = src_item.mbSorted;
	setText ( src_item.text (), false, false, false );
	addToList ( src_item.listWidget (), false );
}

vmListItem::vmListItem ( const QString& label )
	: vmListItem ()
{
	//Calling vmTableItem::setText here, and in vmTableItem::update is a waste of time and resouces. When the item
	//is created it does not need to undergo a series of calculations and procedures. Only when inserted
	//into a list
	//setText ( label, false, false, false );
	setMemoryTextOnly ( label );
}

vmListItem::~vmListItem () {}

void vmListItem::addToList ( vmListWidget* const w_list, const bool b_makecall )
{
	if ( m_list != w_list )
	{
		if ( m_list != nullptr )
			m_list->removeItem ( this );
		m_list = w_list;
	}
	w_list->addItem ( this, b_makecall );
}

void vmListItem::changeAppearance ()
{
	if ( listWidget () )
		listWidget ()->setUpdatesEnabled ( false );

	update ();
	if ( listWidget () )
	{
		listWidget ()->setUpdatesEnabled ( true );
		listWidget ()->repaint ();
		listWidget ()->parentWidget ()->repaint ();
	}
	qApp->sendPostedEvents ();
}
