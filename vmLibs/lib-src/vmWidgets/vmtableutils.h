#ifndef VMTABLEUTILS_H
#define VMTABLEUTILS_H

#include "vmlist.h"
#include <vmUtils/tristatetype.h>

#include <QtWidgets/QFrame>

#include <functional>

class vmLineFilter;
class vmLineEdit;
class vmCheckBox;
class vmTableWidget;
class vmTableItem;

class QToolButton;

class vmTableSearchPanel final : public QFrame
{

public:
	vmTableSearchPanel ( const vmTableWidget* const table );
	virtual ~vmTableSearchPanel () final = default;
	inline int utilityIndex () const { return m_utilidx; }
	inline void setUtilityIndex ( const int idx ) { m_utilidx = idx; }
	
protected:
	void showEvent ( QShowEvent* se ) override;
	void hideEvent ( QHideEvent* he ) override;

private:
	void searchFieldsChanged ( const vmCheckBox* const = nullptr );
	bool searchStart ();
	bool searchNext ();
	bool searchPrev ();
	void txtSearchTerm_keyPressed ( const QKeyEvent* ke );
	
	QString m_SearchedWord;
	int m_utilidx;
	vmTableWidget* m_table;
	vmCheckBox* chkSearchAllTable;
	vmLineEdit* txtSearchTerm;
	QToolButton* btnSearchStart;
	QToolButton* btnSearchNext;
	QToolButton* btnSearchPrev;
	QToolButton* btnSearchCancel;
};

class vmTableFilterPanel final : public QFrame
{

public:
	explicit vmTableFilterPanel ( const vmTableWidget* const table );
	virtual ~vmTableFilterPanel () final = default;
	inline int utilityIndex () const { return m_utilidx; }
	inline void setUtilityIndex ( const int idx ) { m_utilidx = idx; }
	
	inline void setCallbackForNewViewAfterFilter ( const std::function<void ()>& func ) { newView_func = func; }

protected:
	void showEvent ( QShowEvent* se ) override;
	void hideEvent ( QHideEvent* he ) override;
	
private:
	void doFilter ( const triStateType& level, const int startlevel = 0 );
	
	int m_utilidx;
	vmTableWidget* m_table;
	vmLineFilter* m_txtFilter;
	QToolButton* m_btnClose;
	pointersList<podList<int>*> searchLevels;
	std::function<void ()> newView_func;
};

#endif // VMTABLEUTILS_H
