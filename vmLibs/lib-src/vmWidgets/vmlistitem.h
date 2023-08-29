#ifndef LISTITEMS_H
#define LISTITEMS_H

#include "vmtableitem.h"

#include <vmUtils/tristatetype.h>

#include <QtWidgets/QListWidgetItem>

class vmListWidget;

class vmListItem : public vmTableItem
{

friend class vmListWidget;

friend void list_item_swap ( vmListItem& item1, vmListItem& item2 );

public:
	vmListItem ();
	inline vmListItem ( const vmListItem& other ) : vmListItem () { copy ( other ); }
	vmListItem ( const QString& label );
	virtual ~vmListItem ();
	
	inline const vmListItem& operator= ( vmListItem item )
	{
		list_item_swap ( *this, item );
		return *this;
	}
	
	inline vmListItem ( vmListItem&& other ) : vmListItem ()
	{
		list_item_swap ( *this, other );
	}

	void addToList ( vmListWidget* const w_list, const bool b_makecall = true );
	inline vmListWidget* listWidget () const { return m_list; }

	inline virtual void update () { setText ( const_cast<vmListItem*>( this )->text (), false, false, false ); }

	inline bool itemIsSorted () const { return mbSorted; }
	inline void setItemIsSorted ( const bool b_sorted ) { mbSorted = b_sorted; }

protected:
	void copy ( const vmListItem& src_item );

	virtual void changeAppearance ();
	void deleteRelatedItem ( const uint rel_idx );

	bool mbInit;

private:
	vmListItem ( const QListWidgetItem& );
	vmListItem& operator=( const QListWidgetItem& );
	vmListWidget* m_list;
	bool mbSorted;
};
#endif // LISTITEMS_H
