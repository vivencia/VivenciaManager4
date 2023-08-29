#ifndef QUICKPROJECT_H
#define QUICKPROJECT_H

#include "dbrecord.h"

const uint QUICK_PROJECT_FIELD_COUNT ( 10 );

enum
{
	FLD_QP_ID = 0, FLD_QP_JOB_ID = 1, FLD_QP_ITEM = 2, FLD_QP_SELL_QUANTITY = 3, FLD_QP_SELL_UNIT_PRICE = 4,
	FLD_QP_SELL_TOTALPRICE = 5, FLD_QP_PURCHASE_QUANTITY = 6, FLD_QP_PURCHASE_UNIT_PRICE = 7, FLD_QP_PURCHASE_TOTAL_PRICE = 8,
	FLD_QP_RESULT = 9
};

class quickProject : public DBRecord
{

friend class quickProjectUI;
friend class VivenciaDB;

public:
	quickProject ();
	virtual ~quickProject ();

private:
	static const TABLE_INFO t_info;

	RECORD_FIELD m_RECFIELDS[QUICK_PROJECT_FIELD_COUNT];
	void ( *helperFunction[QUICK_PROJECT_FIELD_COUNT] ) ( const DBRecord* );
	friend bool updateQuickProjectTable ();
};

#endif // QUICKPROJECT_H
