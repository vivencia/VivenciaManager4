#ifndef GENERALTABLE_H
#define GENERALTABLE_H

#include "dbrecord.h"

const uint GENERAL_FIELD_COUNT ( 6 );

enum
{
	FLD_GENERAL_ID = 0, FLD_GENERAL_TABLENAME = 1, FLD_GENERAL_TABLEVERSION = 2, FLD_GENERAL_TABLEID = 3,
	FLD_GENERAL_CONFIG_FILE = 4, FLD_GENERAL_EXTRA = 5
};

extern void updateIDs ( const TABLE_ORDER table , podList<int>* new_ids = nullptr );

class generalTable : public DBRecord
{

friend class VivenciaDB;

public:
	generalTable ();
	virtual ~generalTable ();

	void insertOrUpdate ( const TABLE_INFO* const t_info );
	
protected:
	friend bool updateGeneralTable ( const unsigned char current_table_version );
	static const TABLE_INFO t_info;

	RECORD_FIELD m_RECFIELDS[GENERAL_FIELD_COUNT];
	void ( *helperFunction[GENERAL_FIELD_COUNT] ) ( const DBRecord* );
};

#endif // GENERALTABLE_H
