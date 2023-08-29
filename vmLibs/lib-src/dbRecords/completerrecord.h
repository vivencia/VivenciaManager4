#ifndef COMPLETERRECORD_H
#define COMPLETERRECORD_H

#include "dbrecord.h"

#include <QtCore/QStringList>
#include <QtSql/QSqlQuery>

const uint CR_FIELD_COUNT ( 13 );

enum
{
	FLD_CR_ID = 0, FLD_CR_PRODUCT_OR_SERVICE_1 = 1, FLD_CR_PRODUCT_OR_SERVICE_2 = 2, FLD_CR_BRAND = 3,
	FLD_CR_STOCK_TYPE = 4, FLD_CR_STOCK_PLACE = 5, FLD_CR_PAYMENT_METHOD = 6,
	FLD_CR_ADDRESS = 7, FLD_CR_DELIVERY_METHOD = 8, FLD_CR_ACCOUNT = 9, FLD_CR_JOB_TYPE = 10, FLD_CR_MACHINE_NAME = 11,
	FLD_CR_MACHINE_EVENT = 12
};

class completerRecord : public DBRecord
{

friend class VivenciaDB;
friend class vmCompleters;

public:
	completerRecord ();
	virtual ~completerRecord ();

	static void loadCompleterStrings ( QStringList& completer_strings, const COMPLETER_CATEGORIES category );
	static void loadCompleterStringsForProductsAndServices ( QStringList& completer_strings, QStringList& completer_strings_2 );
	void updateTable ( const COMPLETER_CATEGORIES category, const QString& str, const bool b_reset_search = true );
	void batchUpdateTable ( const COMPLETER_CATEGORIES category, QStringList& str_list );

protected:
	friend bool updateCompleterRecordTable ();
	static const TABLE_INFO t_info;

	RECORD_FIELD m_RECFIELDS[CR_FIELD_COUNT];
	void ( *helperFunction[CR_FIELD_COUNT] ) ( const DBRecord* );

private:
	static void runQuery ( QStringList& results, const TABLE_INFO* t_info, const uint field, const bool b_check_duplicates = false );
};

#endif // COMPLETERRECORD_H
