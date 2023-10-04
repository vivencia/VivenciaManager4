#ifndef SEARCHUI_H
#define SEARCHUI_H

#include <vmlist.h>
#include <dbRecords/dblistitem.h>
#include <vmUtils/tristatetype.h>
#include <vmStringRecord/stringrecord.h>

#include <QtWidgets/QDialog>

#include <functional>

class vmWidget;
class vmTableWidget;
class vmLineEdit;

class QToolButton;
class QPushButton;
class QKeyEvent;

enum SEARCH_STEPS
{
	SS_CLIENT = 0, SS_JOB, SS_PAY, SS_BUY,
	SS_INVENTORY, SS_SUPPLIES, SS_SUPPLIERS
};

class searchUI : public QDialog
{

public:
	explicit searchUI ();
	virtual ~searchUI () override;

	inline bool canSearch () const { return static_cast<SEARCH_STATUS>( searchStatus.state () ) == SS_NOT_FOUND; }
	inline bool isSearching () const { return static_cast<SEARCH_STATUS>( searchStatus.state () ) == SS_SEARCH_FOUND; }
	inline const QString& searchTerm () const { return mSearchTerm; }

	void searchCancel ();
	void prepareSearch ( const QString& searchTerm );
	void parseSearchTerm ( const QString& searchTerm );
	void search ( uint search_start = SS_CLIENT, const uint search_end = SS_SUPPLIERS );
	bool searchFirst ();
	bool searchPrev ();
	bool searchNext ();
	bool searchLast ();

	bool isLast () const;
	bool isFirst () const;

private:
	void setupUI ();
	void createTable ();
	void on_txtSearch_textEdited ( const QString& text );
	void searchCallbackSelector ( const QKeyEvent* const ke );
	void btnSearchClicked ();
	void btnPrevClicked ();
	void btnNextClicked ();
	void listRowSelected ( const int row );
	void searchClients ();
	void searchJobs ();
	void searchPayments ();
	void searchPurchases ();
	void searchInventory ();
	void searchSupplies ();
	void searchSuppliers ();

	QString mSearchTerm;
	uint mSearchFields;
	int mCurRow;
	triStateType searchStatus;
	vmTableWidget* mFoundList;
	vmLineEdit* mtxtSearch;
	QToolButton* mBtnSearch;
	QToolButton* mBtnNext;
	QToolButton* mBtnPrev;
	QPushButton* mBtnClose;
	stringTable mFoundItems;
};
#endif // SEARCHUI_H
