#ifndef DBTABLEVIEW_H
#define DBTABLEVIEW_H

#include <vmlist.h>
#include <vmNumbers/vmnumberformats.h>

#include <QtCore/QObject>
#include <QtWidgets/QFrame>

class vmListWidget;
class vmTableWidget;
class vmTableItem;
class vmLineEdit;
class tableViewWidget;
		
class QTabWidget;
class QSplitter;
class QVBoxLayout;
class QToolButton;
class QSqlQuery;

class dbTableView : public QObject
{

public:
	explicit dbTableView ();
	virtual ~dbTableView ();
	
	void reload ();
	inline QVBoxLayout* layout () const { return mMainLayout; }
	
private:
	void loadTablesIntoList ();
	void showTable ( const QString& tablename );
	void runPersonalQuery ();
	void maybeRunQuery ( const QKeyEvent* const ke );
	void notifyMainWindowOfChanges ( const QString& query );
	
	vmListWidget* mTablesList;
	QFrame* mLeftFrame;
	QTabWidget* mTabView;
	vmLineEdit* mTxtQuery;
	QToolButton* mBtnRunQuery;
	QSplitter* mMainLayoutSplitter;
	QVBoxLayout* mMainLayout;
};

class tableViewWidget final : public QFrame
{

public:
	explicit tableViewWidget ( const QString& tablename );
	tableViewWidget ( QSqlQuery* const query );
	virtual ~tableViewWidget () final = default;
	
	void showTable ();
	void load ();
	void reload ();

	inline const QString& tableName () const { return m_tablename; }
	
private:
	void focusInEvent ( QFocusEvent* e );
	void createTable ();
	void getTableInfo ();
	void getTableLastUpdate ( vmNumber& date , vmNumber& time );
	void updateTable ( const vmTableItem* const item );
	void rowInserted ( const uint row );
	bool rowRemoved ( const uint row );
	
	vmTableWidget* m_table;
	vmList<QString> m_cols;
	QString m_tablename;
	QVBoxLayout* mLayout;
	vmNumber m_updatedate, m_updatetime;
	QSqlQuery* mQuery;
	uint m_nrows;
	bool mb_loaded;
};

#endif // DBTABLEVIEW_H
