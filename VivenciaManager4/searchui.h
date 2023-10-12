#ifndef SEARCHUI_H
#define SEARCHUI_H

#include <vmlist.h>
#include <dbRecords/dblistitem.h>
#include <vmUtils/tristatetype.h>
#include <vmStringRecord/stringrecord.h>

#include <QtWidgets/QDialog>
#include <QtCore/QThread>

#include <functional>

class searchThreadWorker;

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
Q_OBJECT

public:
	explicit searchUI ( QWidget* parent );
	virtual ~searchUI () override;
	void setupUI ();
	inline const QString& searchTerm () const { return mSearchTerm; }

public slots:
	void insertFoundInfo ( const stringRecord& row_info );

private:
	void searchCancel ();
	void createTable ();
	void on_txtSearch_textEdited ( const QString& text );
	void searchCallbackSelector ( const QKeyEvent* const ke );
	void btnSearchClicked ();
	void btnPrevClicked ();
	void btnNextClicked ();
	void listRowSelected ( const int row );

	searchThreadWorker* mSearchWorker;
	QString mSearchTerm;
	int mCurRow;
	vmTableWidget* mFoundList;
	vmLineEdit* mtxtSearch;
	QToolButton* mBtnSearch;
	QToolButton* mBtnNext;
	QToolButton* mBtnPrev;
	QPushButton* mBtnClose;
	DBRecord* mCurrentDisplayedRec;
};

class searchThreadWorker : public QObject
{
Q_OBJECT

public:
	explicit searchThreadWorker ( QObject* parent );
	~searchThreadWorker ();

	void search ();
	void parseSearchTerm ( QString& searchTerm );
	void searchTable ( DBRecord* db_rec, const QString& columns, const std::function<void( const QSqlQuery& query_res, stringRecord& item_info )>& formatResult );
	void searchClients ();
	void searchJobs ();
	void searchPayments ();
	void searchPurchases ();
	void searchInventory ();
	void searchSuppliers ();

signals:
	void infoFound ( const stringRecord& row_info );
	void searchDone ();

private:
	VivenciaDB* mVDB;
	uint mSearchFields;
	bool mbFound;
	QString mSearchTerm;
};

#endif // SEARCHUI_H
