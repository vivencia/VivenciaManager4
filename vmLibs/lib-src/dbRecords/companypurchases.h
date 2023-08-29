#ifndef COMPANYPURCHASES_H
#define COMPANYPURCHASES_H

#include "dbrecord.h"

const uint COMPANY_PURCHASES_FIELD_COUNT ( 10 );

enum
{
	FLD_CP_ID = 0, FLD_CP_DATE = 1, FLD_CP_DELIVERY_DATE = 2, FLD_CP_DELIVERY_METHOD = 3,
	FLD_CP_NOTES = 4, FLD_CP_SUPPLIER = 5, FLD_CP_TOTAL_PRICE = 6, FLD_CP_PAY_VALUE = 7,
	FLD_CP_ITEMS_REPORT = 8, FLD_CP_PAY_REPORT = 9
};

class companyPurchases : public DBRecord
{

friend class VivenciaDB;
friend class companyPurchasesUI;

public:
	explicit companyPurchases ( const bool connect_helper_funcs = false );
	virtual ~companyPurchases ();
	void exportToInventory ();

private:
	static const TABLE_INFO t_info;

	friend bool updateCPTable ( const unsigned char current_table_version );

	RECORD_FIELD m_RECFIELDS[COMPANY_PURCHASES_FIELD_COUNT];
	void ( *helperFunction[COMPANY_PURCHASES_FIELD_COUNT] ) ( const DBRecord* );
};

#endif // COMPANYPURCHASES_H
