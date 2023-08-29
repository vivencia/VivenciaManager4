#ifndef DBLISTITEM_H
#define DBLISTITEM_H

#include <vmWidgets/vmlistitem.h>

#include "db_defs.h"

#include "client.h"
#include "job.h"
#include "payment.h"
#include "purchases.h"

class vmListWidget;
class clientListItem;
class jobListItem;
class payListItem;
class buyListItem;
class crashRestore;
class QListWidgetItem;
class QIcon;

static const uint client_nBadInputs ( 1 );
static const uint job_nBadInputs ( 2 );
static const uint pay_nBadInputs ( 6 );
static const uint buy_nBadInputs ( 4 );

/* Some switches mix dbRecord fields and infoTable fields, which might inccur in overlaps.
 * To avoid such compile errors, both callers and callees must add this offset whenever using INFO_TABLE_COLUMNS (Payment and Buy, so far)
 */
static const uint INFO_TABLE_COLUMNS_OFFSET ( 100 );

enum SEARCH_STATUS {
	SS_NOT_SEARCHING = TRI_UNDEF, SS_SEARCH_FOUND = TRI_ON, SS_NOT_FOUND = TRI_UNDEF
};

enum RELATED_LIST_ITEMS {
	RLI_CLIENTPARENT = 0, RLI_JOBPARENT = 1, RLI_CLIENTITEM = 2, RLI_JOBITEM = 3, 
	RLI_CALENDARITEM = 4, RLI_EXTRAITEMS = 5
};

constexpr uint PAY_ITEM_OVERDUE_CLIENT (static_cast<uint>(RLI_EXTRAITEMS) + 0 );
constexpr uint PAY_ITEM_OVERDUE_ALL ( static_cast<uint>(RLI_EXTRAITEMS) + 1 );

enum CRASH_FIELDS {
	CF_SUBTYPE = 0, CF_ACTION, CF_CRASHID, CF_DBRECORD
};

class dbListItem : public vmListItem
{

friend void db_item_swap ( dbListItem& item1, dbListItem& item2 );

public:
	dbListItem ();
	inline dbListItem ( const dbListItem& other ) : dbListItem () { copy ( other ); }
	
	dbListItem ( const uint type_id, const uint nbadInputs = 0, bool* const badinputs_ptr = nullptr );
	explicit dbListItem ( const QString& label );
	virtual ~dbListItem () override;
	
	inline const dbListItem& operator= ( dbListItem item )
	{
		db_item_swap ( *this, item );
		return *this;
	}
	
	inline dbListItem ( dbListItem&& other ) : dbListItem ()
	{
		db_item_swap ( *this, other );
	}

	static void appStartingProcedures ();
	static void appExitingProcedures ();

	inline RECORD_ACTION action () const { return m_action; }
	void setRelation ( const uint relation  );
	inline uint relation () const { return mRelation; }
	void disconnectRelation ( const uint start_relation, dbListItem* item );
	void syncSiblingWithThis ( dbListItem* sibling );

	inline DBRecord* dbRec () const { return m_dbrec; }
	void setDBRec ( DBRecord* dbrec, const bool self_only = false );

	inline uint dbRecID () const { return static_cast<uint>(id ()); }
	inline void setDBRecID ( const uint recid ) { setID ( static_cast<int>(recid) ); }

	void saveCrashInfo ( crashRestore* crash );

	void setAction ( const RECORD_ACTION action, const bool bSetDBRec = false, const bool bSelfOnly = false );
	virtual void createDBRecord ();
	virtual bool loadData ( const bool b_read_all_data );
	inline virtual void update () override { setLabel ( text () ); }
	virtual void relationActions ( dbListItem* = nullptr ) { ; }
	void setRelatedItem ( const uint rel_idx, dbListItem* const item );
	dbListItem* relatedItem ( const uint rel_idx ) const;
	
	virtual uint translatedInputFieldIntoBadInputField ( const uint field ) const;

	inline int crashID () const { return m_crashid; }
	inline void setCrashID ( const int crashid ) { m_crashid = crashid; }
	inline bool isGoodToSave () const { return n_badInputs <= 0; }
	inline bool fieldInputStatus ( const uint field ) const { return badInputs_ptr[translatedInputFieldIntoBadInputField( field )]; }

	void setFieldInputStatus ( const uint field, const bool ok, const vmWidget* const widget );

	// Will not check field boundary; trust caller
	inline void setSearchFieldStatus ( const uint field, const SEARCH_STATUS ss ) {
			if ( searchFields ) searchFields[field] = ss; }
	inline SEARCH_STATUS searchFieldStatus ( const uint field ) const {
			return searchFields ? static_cast<SEARCH_STATUS> ( searchFields[field].state () ) : SS_NOT_SEARCHING; }

	void setSearchArray ();
	inline uint lastRelation () const { return mLastRelation; }
	inline void setLastRelation ( const uint relation ) { mLastRelation = static_cast<RELATED_LIST_ITEMS>(relation); }

protected:
	void copy ( const dbListItem& src_item );
	void setLabel ( const QString& text );
	void changeAppearance () override;
	void deleteRelatedItem ( const uint rel_idx );

	int m_crashid;
	DBRecord* m_dbrec;
	uint mRelation;
	uint mLastRelation;
	triStateType* searchFields;

private:
	dbListItem ( const QListWidgetItem& );
	dbListItem& operator=( const QListWidgetItem& );

	dbListItem* item_related[20];
	
	RECORD_ACTION m_action;
	vmListWidget* m_list;

	bool* badInputs_ptr;
	int n_badInputs;
	uint mTotal_badInputs;
	bool mbSearchCreated;
	
	static const QString actionSuffix[6];
	static QIcon* listIndicatorIcons[4];
	static bool APP_EXITING;
};

class clientListItem : public dbListItem
{

public:
	explicit inline clientListItem ()
		: dbListItem ( CLIENT_TABLE, client_nBadInputs, badInputs ), jobs ( nullptr ), pays ( nullptr ), buys ( nullptr ) {}
	virtual ~clientListItem () override;

	// Prevent Qt from deleting these objects
	inline void operator delete ( void* ) { return; }

	inline Client* clientRecord () const { return static_cast<Client*> ( dbRec () ); }

	void update () override;
	void createDBRecord () override;
	bool loadData ( const bool b_read_all_data ) override;
	void relationActions ( dbListItem* subordinateItem = nullptr ) override;

	pointersList<jobListItem*>* jobs;
	pointersList<payListItem*>* pays;
	pointersList<buyListItem*>* buys;

private:
	bool badInputs[client_nBadInputs];
};

class jobListItem : public dbListItem
{
public:
	explicit inline jobListItem ()
		: dbListItem ( JOB_TABLE, job_nBadInputs, badInputs ),
			buys ( nullptr ), daysList ( nullptr ), mSearchSubFields ( nullptr ), 
			m_payitem ( nullptr ), m_newproject_opt ( INT_MIN ) {}

	virtual ~jobListItem () override;

	// Prevent Qt from deleting these objects
	inline void operator delete ( void* ) { return; }
	
	inline Job* jobRecord () const { return static_cast<Job*>( dbRec () ); }

	uint translatedInputFieldIntoBadInputField ( const uint field ) const override;

	void createDBRecord () override;
	bool loadData ( const bool b_read_all_data ) override;
	void update () override;
	void relationActions ( dbListItem* subordinateItem = nullptr ) override;

	inline void setPayItem ( payListItem* const pay ) { m_payitem = pay; }
	inline payListItem* payItem () const { return m_payitem; }

	inline void setNewProjectOpt ( const int opt ) { m_newproject_opt = opt; }
	inline int newProjectOpt () const { return m_newproject_opt; }

	podList<uint>* searchSubFields () const;
	void setReportSearchFieldFound ( const uint report_field, const uint day );

	pointersList<buyListItem*>* buys;
	pointersList<vmListItem*>* daysList;

private:
	podList<uint>* mSearchSubFields;
	payListItem* m_payitem;
	int m_newproject_opt;

	bool badInputs[job_nBadInputs];
};

class payListItem : public dbListItem
{

public:
	explicit inline payListItem ()
		: dbListItem ( PAYMENT_TABLE, pay_nBadInputs, badInputs ) {}
	virtual ~payListItem () override;

	// Prevent Qt from deleting these objects
	inline void operator delete ( void* ) { return; }
		
	inline Payment* payRecord () const { return static_cast<Payment*> ( dbRec () ); }

	uint translatedInputFieldIntoBadInputField ( const uint field ) const override;

	void createDBRecord () override;
	bool loadData ( const bool b_read_all_data ) override;
	void update () override;
	void updatePayCalendarItem ();
	void updatePayExtraItems ( uint relation );
	void relationActions ( dbListItem* subordinateItem = nullptr ) override;

private:
	bool badInputs[pay_nBadInputs];
};

class buyListItem : public dbListItem
{

public:
	explicit inline buyListItem ()
		: dbListItem ( PURCHASE_TABLE, buy_nBadInputs, badInputs ) {}
	virtual ~buyListItem () override;

	// Prevent Qt from deleting these objects
	inline void operator delete ( void* ) { return; }
		
	inline Buy* buyRecord () const { return static_cast<Buy*> ( dbRec () ); }

	void createDBRecord () override;
	bool loadData ( const bool b_read_all_data ) override;
	void update () override;
	void updateBuyExtraItem ( const QString& label );
	void updateBuyCalendarItem ();
	void relationActions ( dbListItem* subordinateItem = nullptr ) override;

	uint translatedInputFieldIntoBadInputField ( const uint field ) const override;

private:
	bool badInputs[buy_nBadInputs];
};

#endif // DBLISTITEM_H
