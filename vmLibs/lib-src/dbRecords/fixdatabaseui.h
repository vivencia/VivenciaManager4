#ifndef FIXDATABASEUI_H
#define FIXDATABASEUI_H

#include "vmlist.h"
#include "fixdatabase.h"

#include <QtWidgets/QDialog>

class vmTableWidget;
class QPushButton;

class fixDatabaseUI : public QDialog
{

public:
	virtual ~fixDatabaseUI ();
	void showWindow ();
	void doCheck ();
	void doFix ();

private:
	explicit fixDatabaseUI ();

	void setupUI ();
	void populateTable ();

	vmTableWidget* tablesView;

	fixDatabase *m_fdb;
	QPushButton* btnCheck;
	QPushButton* btnFix;
	QPushButton* btnClose;

	pointersList<fixDatabase::badTable*> m_tables;
};
#endif // FIXDATABASEUI_H
