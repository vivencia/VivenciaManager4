#ifndef DBSUPPLIES_H
#define DBSUPPLIES_H

#include "dbrecord.h"

const uint SUPPLIES_FIELD_COUNT ( 10 );

enum
{
	FLD_SUPPLIES_ID = 0, FLD_SUPPLIES_ITEM = 1, FLD_SUPPLIES_BRAND = 2, FLD_SUPPLIES_QUANTITY = 3, FLD_SUPPLIES_UNIT = 4, FLD_SUPPLIES_TYPE = 5,
	FLD_SUPPLIES_DATE_IN = 6, FLD_SUPPLIES_PRICE = 7, FLD_SUPPLIES_SUPPLIER = 8, FLD_SUPPLIES_PLACE = 9
};

class dbSupplies : public DBRecord
{

friend class VivenciaDB;
friend class dbSuppliesUI;

public:
	explicit dbSupplies ( const bool connect_helper_funcs = false );
	virtual ~dbSupplies ();

	uint isrRecordField ( const ITEMS_AND_SERVICE_RECORD ) const;
	inline QString isrValue ( const ITEMS_AND_SERVICE_RECORD isr_field, const int = -1 ) const
	{
		return recStrValue ( this, isrRecordField ( isr_field ) );
	}

	static const TABLE_INFO t_info;

	static void notifyDBChange ( const uint id );
	
protected:
	friend bool updateSuppliesTable ();

	RECORD_FIELD m_RECFIELDS[SUPPLIES_FIELD_COUNT];
	void ( *helperFunction[SUPPLIES_FIELD_COUNT] ) ( const DBRecord* );
};

#endif // DBSUPPLIES_H
