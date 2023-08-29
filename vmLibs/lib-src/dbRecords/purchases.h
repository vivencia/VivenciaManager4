#ifndef PURCHASES_H
#define PURCHASES_H

#include "dbrecord.h"

class buyListItem;

const uint BUY_FIELD_COUNT ( 14 );

enum
{
	FLD_BUY_ID = 0, FLD_BUY_CLIENTID = 1, FLD_BUY_JOBID = 2, FLD_BUY_DATE = 3, FLD_BUY_DELIVERDATE = 4,
	FLD_BUY_PRICE = 5, FLD_BUY_TOTALPAID = 6, FLD_BUY_TOTALPAYS = 7, FLD_BUY_DELIVERMETHOD = 8,
	FLD_BUY_SUPPLIER = 9, FLD_BUY_NOTES = 10, FLD_BUY_REPORT = 11, FLD_BUY_PAYINFO = 12, FLD_BUY_LAST_VIEWED = 13
};

class Buy : public DBRecord
{

friend class VivenciaDB;
friend class MainWindow;

friend void updateCalendarBuyInfo ( const DBRecord* db_rec );

public:

	explicit Buy ( const bool connect_helper_funcs = false );
	virtual ~Buy () override;

	uint isrRecordField ( const ITEMS_AND_SERVICE_RECORD ) const override;
	QString isrValue ( const ITEMS_AND_SERVICE_RECORD isr_field, const int sub_record = -1 ) const override;
	int searchCategoryTranslate ( const SEARCH_CATEGORIES sc ) const override;
	void copySubRecord ( const uint subrec_field, const stringRecord& subrec ) override;

	void setListItem ( buyListItem* buy_item );
	buyListItem* buyItem () const;

	static const TABLE_INFO t_info;

protected:
	friend bool updatePurchaseTable ( const unsigned char current_table_version );

	void ( *helperFunction[BUY_FIELD_COUNT] ) ( const DBRecord* );
	RECORD_FIELD m_RECFIELDS[BUY_FIELD_COUNT];
};

#endif // PURCHASES_H
