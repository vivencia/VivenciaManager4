#ifndef VMMACHINES_H
#define VMMACHINES_H

#include "dbrecord.h"

const uint MACHINES_FIELD_COUNT ( 9 );

enum
{
	FLD_MACHINES_ID = 0, FLD_MACHINES_SUPPLIES_ID = 1, FLD_MACHINES_NAME = 2, FLD_MACHINES_BRAND = 3,
	FLD_MACHINES_TYPE = 4, FLD_MACHINES_EVENTS_DESC = 5, FLD_MACHINES_EVENT_DATES = 6,
	FLD_MACHINES_EVENT_TIMES = 7, FLD_MACHINES_EVENT_JOBS = 8
};

class machinesRecord : public DBRecord
{

	friend class VivenciaDB;
	friend class machinesDlg;

public:
	explicit machinesRecord ();
	virtual ~machinesRecord ();

	static const TABLE_INFO t_info;

protected:
	friend bool updateMachinesTable ();

	RECORD_FIELD m_RECFIELDS[MACHINES_FIELD_COUNT];
	void ( *helperFunction[MACHINES_FIELD_COUNT] ) ( const DBRecord* );
};

#endif // VMMACHINES_H
