#ifndef SEARCHUI_H
#define SEARCHUI_H

#include <vmlist.h>
#include <dbRecords/dblistitem.h>
#include <vmUtils/tristatetype.h>

#include <QtWidgets/QDialog>

#include <functional>

class vmWidget;
class vmTableWidget;

class QToolButton;
class QPushButton;

enum SEARCH_STEPS
{
	SS_CLIENT = 0, SS_JOB, SS_PAY, SS_BUY,
	SS_INVENTORY, SS_SUPPLIES, SS_SUPPLIERS
};

class searchUI : public QDialog
{

public:
	virtual ~searchUI () override;

	static void init ()
	{
		if ( searchUI::s_instance == nullptr )
			searchUI::s_instance = new searchUI ();
	}

	inline bool canSearch () const { return static_cast<SEARCH_STATUS>( searchStatus.state () ) == SS_NOT_FOUND; }
	inline bool isSearching () const { return static_cast<SEARCH_STATUS>( searchStatus.state () ) == SS_SEARCH_FOUND; }
	inline const QString& searchTerm () const { return mSearchTerm; }

	void searchCancel ();
	void prepareSearch ( const QString& searchTerm, QWidget* widget );
	void parseSearchTerm ( const QString& searchTerm );
	void search ( uint search_start = SS_CLIENT, const uint search_end = SS_SUPPLIERS );
	void fillList ();
	bool searchFirst ();
	bool searchPrev ();
	bool searchNext ();
	bool searchLast ();

	bool isLast () const;
	bool isFirst () const;

private:
	explicit searchUI ();
	friend searchUI* SEARCH_UI ();
	friend void deleteSearchUIInstance ();
	static searchUI* s_instance;

	void setupUI ();
	void createTable ();
	void btnPrevClicked ();
	void btnNextClicked ();
	void listRowSelected ( const int row );
	dbListItem* getOtherItem ( const uint typeID, const uint id ) const;
	void getClientInfo ( const clientListItem* const client_rec, vmList<QString>& cellData );
	bool getJobInfo (jobListItem* job_item, vmList<QString>& cellData );
	bool getPayInfo ( payListItem* pay_item, vmList<QString>& cellData );
	bool getBuyInfo ( buyListItem* buy_item, vmList<QString>& cellData );
	bool getOtherInfo ( dbListItem* item, vmList<QString>& cellData );

	QString mSearchTerm;
	bool mbShow;
	uint mSearchFields;
	triStateType searchStatus;
	vmTableWidget* mFoundList;
	QToolButton* mBtnNext;
	QToolButton* mBtnPrev;
	QPushButton* mBtnClose;
	QWidget* mWidget;
	pointersList<dbListItem*> mFoundItems;
	vmTableItem* mPreviusItemActivated;
	std::function<void ( dbListItem* item, const bool bshow )> displayFunc;
};

inline searchUI* SEARCH_UI ()
{
	return searchUI::s_instance;
}

#endif // SEARCHUI_H
