#ifndef VMLISTWIDGET_H
#define VMLISTWIDGET_H

#include "vmtablewidget.h"
#include "vmlistitem.h"

class QResizeEvent;

class vmListWidget : public vmTableWidget
{

public:
	vmListWidget ( QWidget* parent = nullptr, const uint nRows = 0 );
	virtual ~vmListWidget () override;

	void setIgnoreChanges ( const bool b_ignore ) override;
	inline bool isIgnoringChanges () const override { return mbIgnore; }
	inline void setCurrentItem ( const vmListItem* const item, const bool b_force, const bool b_makecall ) { setCurrentRow ( item ? item->row () : -1, b_force, b_makecall ); }
	void setCurrentRow ( int row, const bool b_force, const bool b_makecall );
	inline vmListItem* currentItem () const { return mCurrentItem; }
	inline vmListItem* prevItem () const { return mPrevItem; }
	void addItem ( vmListItem* item, const bool b_makecall = true );
	inline void removeItem ( vmListItem* item, const bool bDel = false ) { if ( item ) removeRow_list ( static_cast<uint>(item->row ()), 1, bDel ); }
	void insertRow ( const uint row, const uint n = 1 ) override;
	void removeRow_list ( const uint row, const uint n = 1, const bool bDel = false );
	void clear ( const bool b_ignorechanges = true, const bool b_del = false );
	inline int count () const { return rowCount (); }
	inline vmListItem* item ( const int row ) const { return static_cast<vmListItem*>( sheetItem ( row >= 0 ? static_cast<uint>(row) : static_cast<uint>(mPrevRow), 0 ) ); }
	
	/* Default: false. Many items are managed elsewhere because they are used for more than just displaying rows of
	 * text. This property is used to allow for the proper cleaning of garbage in addition to avoid the inadvertent
	 * attempt to delete something that may be deleted elsewhere another time.
	 */
	inline void setDeleteItemsWhenDestroyed ( const bool b_destroydelete ) { mbDestroyDelete = b_destroydelete; }
	inline bool isDeleteItemsWhenDestroyed () const { return mbDestroyDelete; }

	/* This property signifies that mCurrentItemChangedFunc is called even if mCurrentItem is asked to become again mCurrentItem.
	 * i.e. setCurrentRow ( row == mCurrentItem.row () ) or setCurrentItem ( item == mCurrentItem ), etc.
	 * It is useful when more than one list widget is associated with a common display view. The comparison for likelihood should
	 * be on the display view side, when applicable. So far, in MainWindow, there are three payment lists sharing a single view (payment info)
	 * When the user switches between the lists the view would not change on clicking on the already selected item for that other list because
	 * it is already the current item for that list, although the view is displaying info from another item from another list
	 **/
	void setAlwaysEmitCurrentItemChanged ( const bool b_emit );
	inline bool alwaysEmitCurrentItemChanged () const { return mbForceEmit; }
	inline void setCallbackForCurrentItemChanged ( const std::function<void( vmListItem* current )>& func ) { mCurrentItemChangedFunc = func; }
	inline void setCallbackForWidgetGettingFocus ( const std::function<void()>& func ) { mGotFocusFunc = func; }

protected:
	void resizeEvent ( QResizeEvent* e ) override;
	void focusInEvent ( QFocusEvent* e ) override;
	inline void setCurrentItem ( vmListItem* new_current_item ) { mPrevItem = mCurrentItem; mCurrentItem = new_current_item; }

private:
	void rowSelected ( const int row, const int prev_row );
	
	bool mbIgnore, mbDestroyDelete, mbForceEmit;
	int mPrevRow;
	vmListItem* mCurrentItem, *mPrevItem;
	std::function<void( vmListItem* current )> mCurrentItemChangedFunc;
	std::function<void()> mGotFocusFunc;
};

#endif // VMLISTWIDGET_H
