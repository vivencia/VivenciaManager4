#ifndef INVENTORY_H
#define INVENTORY_H

#include "dbrecord.h"

const uint INVENTORY_FIELD_COUNT ( 10 );

enum
{
	FLD_INVENTORY_ID = 0, FLD_INVENTORY_ITEM = 1, FLD_INVENTORY_BRAND = 2, FLD_INVENTORY_QUANTITY = 3,
	FLD_INVENTORY_UNIT = 4, FLD_INVENTORY_TYPE = 5, FLD_INVENTORY_DATE_IN = 6, FLD_INVENTORY_PRICE = 7,
	FLD_INVENTORY_SUPPLIER = 8, FLD_INVENTORY_PLACE = 9
};

class Inventory : public DBRecord
{

friend class VivenciaDB;

public:
	explicit Inventory ( const bool connect_helper_funcs = false );
	virtual ~Inventory () override;

	uint isrRecordField ( const ITEMS_AND_SERVICE_RECORD isr_field ) const override;
	inline QString isrValue ( const ITEMS_AND_SERVICE_RECORD isr_field, const int = -1 ) const override
	{
		return recStrValue ( this, isrRecordField ( isr_field ) );
	}

	static const TABLE_INFO t_info;

protected:
	friend bool updateInvetory ();
	RECORD_FIELD m_RECFIELDS[INVENTORY_FIELD_COUNT];
	void ( *helperFunction[INVENTORY_FIELD_COUNT] ) ( const DBRecord* );
};

#endif // INVENTORY_H
