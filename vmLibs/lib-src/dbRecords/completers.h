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
	NONE = -1, ALL_CATEGORIES = 0, PRODUCT_OR_SERVICE = 1,
	SUPPLIER = 2, BRAND = 3, STOCK_TYPE = 4, STOCK_PLACE = 5,
	PAYMENT_METHOD = 6, ADDRESS = 7, ITEM_NAMES = 8, CLIENT_NAME = 9,
	DELIVERY_METHOD = 10, ACCOUNT = 11, JOB_TYPE = 12, MACHINE_NAME = 13,
	MACHINE_EVENT = 14
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

	COMPLETER_CATEGORIES completerType ( QCompleter* completer, const QString& completion = QString () ) const;
	void encodeCompleterISRForSpreadSheet ( const DBRecord* dbrec );

private:
	void loadCompleters ();
	bool mbFullInit;

	pointersList<QCompleter*> completersList;
};
#endif // COMPLETERS_H
