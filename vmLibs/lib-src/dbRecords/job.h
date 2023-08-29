#ifndef JOB_H
#define JOB_H

#include "vmlibs.h"
#include "dbrecord.h"

class Client;
class jobListItem;

const uint JOB_FIELD_COUNT ( 14 );
const uint JOB_REPORT_FIELDS_COUNT ( 5 );

enum
{
	FLD_JOB_ID = 0, FLD_JOB_CLIENTID = 1, FLD_JOB_TYPE = 2, FLD_JOB_STARTDATE = 3, FLD_JOB_ENDDATE = 4,
	FLD_JOB_TIME = 5, FLD_JOB_PRICE = 6, FLD_JOB_PROJECT_PATH = 7, FLD_JOB_PROJECT_ID = 8,
	FLD_JOB_PICTURE_PATH = 9, FLD_JOB_ADDRESS = 10, FLD_JOB_KEYWORDS = 11, FLD_JOB_REPORT = 12,
	FLD_JOB_LAST_VIEWED = 13
};

class Job : public DBRecord
{

friend class VivenciaDB;
friend class MainWindow;

friend void updateCalendarJobInfo ( const DBRecord* db_rec );

public:

	enum JOB_REPORT_FIELDS
	{
		JRF_DATE = 0, JRF_START_TIME = 1, JRF_END_TIME = 2, JRF_WHEATHER = 3, JRF_REPORT = 4, JRF_EXTRA = 5
		/*Possible JRF_EXTRA values: CHR_ONE: information added / CHR_ZERO: information removed*/
	};

	explicit Job ( const bool connect_helper_funcs = false );
	virtual ~Job () override;

	int searchCategoryTranslate ( const SEARCH_CATEGORIES sc ) const override;
	void copySubRecord ( const uint subrec_field, const stringRecord& subrec ) override;

	inline const QString projectIDTemplate () const
	{
		return date ( FLD_JOB_STARTDATE ).toDate ( vmNumber::VDF_FILE_DATE ) +
			CHR_HYPHEN + recStrValue ( this, FLD_JOB_ID );
	}

	inline const QString jobSummary () const { return jobSummary ( recStrValue ( this, FLD_JOB_ID ) ); }
	static const QString jobAddress ( const Job* const job = nullptr, Client* client = nullptr );
	static QString jobSummary ( const QString& jobid );
	static QString concatenateJobInfo ( const Job& job, const bool b_skip_report );
	static inline const QString jobTypeWithDate ( const int id ) {
		return ( id > 0 ) ? jobTypeWithDate ( QString::number ( id ) ) : QString ();
	}
	static QString jobTypeWithDate ( const QString& id );
	const QString jobTypeWithDate () const;

	void setListItem ( jobListItem* job_item );
	jobListItem* jobItem () const;
	static void fillJobTypeList ( QStringList& jobList, const QString& clientid );

	static const TABLE_INFO t_info;

protected:
	friend bool updateJobTable ( const unsigned char current_table_version );

	RECORD_FIELD m_RECFIELDS[JOB_FIELD_COUNT];
	void ( *helperFunction[JOB_FIELD_COUNT] ) ( const DBRecord* );
};

#endif // JOB_H
