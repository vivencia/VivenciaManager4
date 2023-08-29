#ifndef DBCALENDAR_H
#define DBCALENDAR_H

#include "vmlist.h"
#include "dbrecord.h"

#include <vmNumbers/vmnumberformats.h>

const uint CALENDAR_FIELD_COUNT ( 13 );

enum
{
	FLD_CALENDAR_ID = 0, FLD_CALENDAR_DAY_DATE = 1, FLD_CALENDAR_WEEK_NUMBER = 2, FLD_CALENDAR_MONTH = 3,
	FLD_CALENDAR_JOBS = 4, FLD_CALENDAR_PAYS = 5, FLD_CALENDAR_PAYS_USE = 6, FLD_CALENDAR_BUYS = 7, FLD_CALENDAR_BUYS_PAY = 8,
	FLD_CALENDAR_TOTAL_JOBPRICE_SCHEDULED = 9, FLD_CALENDAR_TOTAL_PAY_RECEIVED = 10,
	FLD_CALENDAR_TOTAL_BUY_BOUGHT = 11, FLD_CALENDAR_TOTAL_BUY_PAID = 12
};

enum CALENDAR_EXCHANGE_ACTION_ORDER
{
	CEAO_NOTHING = 0, 
	CEAO_ADD_DATE1 = 1, CEAO_DEL_DATE1 = 2, 
	CEAO_ADD_DATE2 = 3, CEAO_DEL_DATE2 = 4,
	CEAO_ADD_PRICE_DATE1 = 5, CEAO_DEL_PRICE_DATE1 = 6, CEAO_EDIT_PRICE_DATE1 = 7,
	CEAO_ADD_PRICE_DATE2 = 8, CEAO_DEL_PRICE_DATE2 = 9, CEAO_EDIT_PRICE_DATE2 = 10
};

struct CALENDAR_EXCHANGE
{
	vmNumber date, price, price2;
	CALENDAR_EXCHANGE_ACTION_ORDER action;
	int extra_info;

	CALENDAR_EXCHANGE () : action ( CEAO_NOTHING ), extra_info ( -1 ) {}
};

class Job;
class Payment;
class Buy;

class dbCalendar final : public DBRecord
{

friend class VivenciaDB;

public:
	dbCalendar ();
	virtual ~dbCalendar () final = default;

	void updateCalendarWithJobInfo ( const Job* const job );
	void updateCalendarWithPayInfo ( const Payment* const pay );
	void updateCalendarWithBuyInfo ( const Buy* const buy );
	void updateCalendarWithBuyPayInfo ( const Buy* const buy, pointersList<CALENDAR_EXCHANGE*>& ce_list );
	
	const stringTable& dateLog ( const vmNumber& date, const uint search_field, const uint requested_field,
								 QString& price, const uint requested_field_price, const bool bIncludeDates = false );

	void addDate ( const vmNumber& date, const uint field, const stringRecord& id_trio );
	void delDate ( const vmNumber& date, const uint field, const stringRecord& id_trio );
	void addPrice ( const vmNumber& date, const vmNumber& price, const uint field );
	void editPrice ( const vmNumber& date, const vmNumber& new_price, const vmNumber& old_price, const uint field );
	void delPrice ( const vmNumber& date, const vmNumber& price, const uint field );

	void addCalendarExchangeRule ( pointersList<CALENDAR_EXCHANGE*>& ce_list, const CALENDAR_EXCHANGE_ACTION_ORDER action,
								   const vmNumber&date, const vmNumber& price = vmNumber::emptyNumber, const int extra_info = -1 );
	static const TABLE_INFO t_info;

protected:
	void updateCalendarDB ( const DBRecord* dbrec, pointersList<CALENDAR_EXCHANGE *>& ce_list );
	friend bool updateCalendarTable ( const unsigned char current_table_version );

	RECORD_FIELD m_RECFIELDS[CALENDAR_FIELD_COUNT];
	void ( *helperFunction[CALENDAR_FIELD_COUNT] ) ( const DBRecord* );

private:
	mutable stringTable mStrTable;
};

#endif // DBCALENDAR_H
