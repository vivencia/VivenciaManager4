#ifndef COMPLETERS_H
#define COMPLETERS_H

#include "vmlist.h"

#include <QtCore/QStringList>
#include <QtWidgets/QCompleter>

const uint COMPLETERS_COUNT ( 15 );

class DBRecord;
class vmWidget;
class completerRecord;

enum COMPLETER_CATEGORIES
{
	CC_NONE = -1, CC_ALL_CATEGORIES = 0, CC_PRODUCT_OR_SERVICE = 1,
	CC_SUPPLIER = 2, CC_BRAND = 3, CC_STOCK_TYPE = 4, CC_STOCK_PLACE = 5,
	CC_PAYMENT_METHOD = 6, CC_ADDRESS = 7, CC_ITEM_NAMES = 8, CC_CLIENT_NAME = 9,
	CC_DELIVERY_METHOD = 10, CC_ACCOUNT = 11, CC_JOB_TYPE = 12, CC_MACHINE_NAME = 13,
	CC_MACHINE_EVENT = 14
};

class vmCompleters
{

public:
	explicit vmCompleters ( const bool b_full_init = true );
	~vmCompleters ();

	inline QCompleter* getCompleter ( const COMPLETER_CATEGORIES type ) const
	{
		return completersList.at ( static_cast<int> ( type ) );
	}

	void setCompleterForWidget ( vmWidget *widget, const int completer_type );

	void updateCompleter ( const DBRecord* const db_rec, const uint field, const COMPLETER_CATEGORIES type );
	void updateCompleter ( const QString& str, const COMPLETER_CATEGORIES type );

	void fillList ( const COMPLETER_CATEGORIES type, QStringList &list ) const;
	int inList ( const QString& str, const COMPLETER_CATEGORIES type ) const;

	COMPLETER_CATEGORIES deriveCompleterTypeFromItsContents ( QCompleter* completer, const QString& completion = QString () );
	void encodeCompleterISRForSpreadSheet ( const DBRecord* dbrec );

private:
	void loadCompleters ();
	bool mbFullInit;

	pointersList<QCompleter*> completersList;
};
#endif // COMPLETERS_H
