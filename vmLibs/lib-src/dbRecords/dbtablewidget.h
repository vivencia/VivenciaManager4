#ifndef DBTABLEWIDGET_H
#define DBTABLEWIDGET_H

#include "db_defs.h"

#include <vmWidgets/vmtablewidget.h>

class DBRecord;
class dbSupplies;
class vmCompleters;

class dbTableWidget : public vmTableWidget
{

public:
	explicit dbTableWidget ( QWidget* parent = nullptr );
	dbTableWidget ( const uint rows, QWidget* parent = nullptr );
	virtual ~dbTableWidget () override;

	void setIgnoreChanges ( const bool b_ignore ) override;
	void exportPurchaseToSupplies ( const DBRecord* const src_dbrec, dbSupplies* const dst_dbrec );
	void interceptCompleterActivated ( const QModelIndex& index );

	static void createPurchasesTable ( dbTableWidget* table );
	static void createPayHistoryTable ( dbTableWidget* table, const PAY_HISTORY_RECORD last_column = PHR_USE_DATE );

private:
	void derivedClassCellContentChanged ( const vmTableItem* const item ) override;
	void derivedClassTableCleared () override;
	void derivedClassTableUpdated () override;

	bool mbDoNotUpdateCompleters;
};

#endif // DBTABLEWIDGET_H
