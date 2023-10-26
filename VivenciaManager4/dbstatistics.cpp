#include "dbstatistics.h"
#include "system_init.h"

#include <heapmanager.h>

#include <vmWidgets/texteditwithcompleter.h>
#include <dbRecords/vivenciadb.h>
#include <dbRecords/client.h>
#include <dbRecords/job.h>
#include <dbRecords/payment.h>
#include <dbRecords/purchases.h>

#include <QtCore/QMap>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QApplication>
#include <QtCore/QProcess>
#include <QtCore/QFuture>
#include <QtCore/QScopedPointer>
#include <QtConcurrent/QtConcurrent>

/*
#include <chrono>
#include <thread>
std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );
std::this_thread::sleep_for ( std::chrono::seconds ( 1 ) );
*/

#define PROCESS_EVENTS qApp->processEvents ();

static const QString HTML_BOLD_ITALIC_UNDERLINE_11 ( QStringLiteral ( "<span style=\" font-size:11pt;text-decoration:underline;font-style:italic;font-weight:600;\">%1</span>") );
static const QString HTML_BOLD_FONT_11 ( QStringLiteral ( "<span style=\" font-size:11pt;font-weight:600;\">%1</span>" ) );

dbStatistics::dbStatistics ()
	: m_textinfo ( new textEditWithCompleter ), mainLayout ( new QVBoxLayout )
{
	m_textinfo->setUtilitiesPlaceLayout ( mainLayout );
	m_textinfo->setEditable ( false );
	mainLayout->addWidget ( m_textinfo );
}

dbStatistics::~dbStatistics () {}

void dbStatistics::reload ()
{
	m_textinfo->clear ();
	auto worker ( new dbStatisticsWorker );
	worker->setCallbackForInfoReady ( [&] ( const QString& more_info ) { return m_textinfo->append ( more_info ); } );
#ifdef USING_QT6
    QFuture<void> future = QtConcurrent::run ( &dbStatisticsWorker::startWorks, worker );
#else
    QFuture<void> future ( QtConcurrent::run ( worker, &dbStatisticsWorker::startWorks ) );
#endif
	future.waitForFinished ();
}

dbStatisticsWorker::dbStatisticsWorker ( QObject* parent )
	: QObject ( parent ), mVDB ( nullptr ), m_readyFunc ( nullptr )
{}

dbStatisticsWorker::~dbStatisticsWorker ()
{
	if ( mVDB != nullptr )
	{
		delete mVDB;
		mVDB = nullptr;
	}
}

void dbStatisticsWorker::startWorks ()
{
	this->deleteLater ();
	mVDB = new VivenciaDB;
	mVDB->openDataBase ( "statistics_connection" );

	countClients ();
	clientsPerYear ( true );
	clientsPerYear ( false );
	PROCESS_EVENTS
	activeClientsPerYear ();
	clientsPerCity ();
	countJobs ();
	PROCESS_EVENTS
	jobPrices ();
	biggestJobs ();
	countPayments ();
}

void dbStatisticsWorker::countClients ()
{
	m_readyFunc ( TR_FUNC ( "Total number of clients: " ) +
				HTML_BOLD_ITALIC_UNDERLINE_11.arg ( mVDB->recordCount ( &Client::t_info, mVDB ) ) + QStringLiteral ( "\n" ) );
	
	QSqlQuery queryRes ( *(mVDB->database ()) );
	queryRes.setForwardOnly ( true );
	if ( queryRes.exec ( QStringLiteral ( "SELECT COUNT(*) FROM CLIENTS WHERE STATUS='1'" ) ) )
	{
		queryRes.first ();
		m_readyFunc ( TR_FUNC ( "Total number of active clients: " ) + HTML_BOLD_ITALIC_UNDERLINE_11.arg ( queryRes.value ( 0 ).toString () ) + QStringLiteral ( "\n" ) );
	}
}

void dbStatisticsWorker::clientsPerYear ( const bool b_start )
{
	QSqlQuery queryRes ( *(mVDB->database ()) );
	queryRes.setForwardOnly ( true );
	if ( queryRes.exec ( b_start ? QStringLiteral ( "SELECT BEGINDATE FROM CLIENTS" ) : QStringLiteral ( "SELECT ENDDATE,STATUS FROM CLIENTS" ) ) )
	{
		queryRes.first ();
		vmNumber date, db_date;
		uint year ( 2005 );
		podList<uint> count_years ( 0, vmNumber::currentDate ().year () - year + 1 );
		do
		{
			date.setDate ( 1, 1, year = 2005 );
			db_date.fromTrustedStrDate ( queryRes.value ( 0 ).toString (), vmNumber::VDF_DB_DATE, false );
			do {
				if ( date.year () == db_date.year () )
				{
					if ( year == vmNumber::currentDate ().year () )
					{
						if ( !b_start )
						{
							if ( queryRes.value ( 1 ) == CHR_ONE )
							break;
						}
					}
					count_years[year-2005]++;
					break;
				}
				date.setDate ( 1, 1, static_cast<int>( ++year ), false );
			} while ( year <= vmNumber::currentDate ().year () );
		} while ( queryRes.next () );
		
		m_readyFunc ( b_start ? TR_FUNC ( "\nNumber of new clients per year since 2005:\n" ) : TR_FUNC ( "\nNumber of lost clients per year since 2005:\n" ) );
		const QString strtemplate ( b_start ? TR_FUNC ( "\t%1: %2 new additions;" ) : TR_FUNC ( "\t%1: %2 clients lost;" ) );
		year = 2005;
		uint count ( count_years.first () );
		while ( year <= vmNumber::currentDate ().year () )
		{
			
			m_readyFunc ( strtemplate.arg ( HTML_BOLD_FONT_11.arg ( QString::number ( year ) ), HTML_BOLD_ITALIC_UNDERLINE_11.arg ( QString::number ( count ) ) ) );
			count = count_years.next ();
			++year;
		}
	}
}

void dbStatisticsWorker::activeClientsPerYear ()
{
	QSqlQuery queryRes ( *(mVDB->database ()) );
	queryRes.setForwardOnly ( true );
	if ( queryRes.exec ( QStringLiteral ( "SELECT BEGINDATE,ENDDATE FROM CLIENTS" ) ) )
	{
		queryRes.first ();
		vmNumber date, start_date, end_date;
		podList<uint> count_clients ( 0, vmNumber::currentDate ().year () - 2005 + 1 );
		do
		{
			start_date.fromTrustedStrDate ( queryRes.value ( 0 ).toString (), vmNumber::VDF_DB_DATE, false );
			end_date.fromTrustedStrDate ( queryRes.value ( 1 ).toString (), vmNumber::VDF_DB_DATE, false );
			date = start_date;
			if ( date.year () >= 2005 ) // a corrupt database might yield a result we are not expecting, and the result will be nasty
			{
				do
				{
					count_clients[date.year ()-2005]++;
					date.setDate ( 31, 12, static_cast<int>( date.year () + 1 ) );				
				} while ( date.year () <= end_date.year () );
			}
		} while ( queryRes.next () );
		
		m_readyFunc ( TR_FUNC ( "\nNumber of active clients per year since 2005:\n" ) );
		const QString strtemplate ( TR_FUNC ( "\t%1: %2;" ) );
		uint year ( 2005 );
		uint count ( count_clients.first () );
		while ( year <= vmNumber::currentDate ().year () )
		{
			
			m_readyFunc ( strtemplate.arg ( HTML_BOLD_FONT_11.arg ( QString::number ( year ) ), HTML_BOLD_ITALIC_UNDERLINE_11.arg ( QString::number ( count ) ) ) );
			count = count_clients.next ();
			++year;
		}
	}
}

void dbStatisticsWorker::clientsPerCity ()
{
	QSqlQuery queryRes ( *(mVDB->database ()) );
	queryRes.setForwardOnly ( true );
	if ( queryRes.exec ( QStringLiteral ( "SELECT CITY FROM CLIENTS"  ) ) )
	{
		queryRes.first ();
		QMap<QString,uint> city_count;
		QString city;
		uint count ( 0 );
		do
		{
			city = queryRes.value ( 0 ).toString ();
			const QMap<QString, uint>::const_iterator& i ( city_count.constFind ( city ) );
			if ( i != city_count.constEnd () )
			{
				count = i.value ();
			}
			else
				count = 0;
			++count;
			city_count.insert ( city, count );
		} while ( queryRes.next () );
		
		m_readyFunc ( TR_FUNC ( "\nNumber of clients per city:\n" ) );
		QMap<QString,uint>::const_iterator itr ( city_count.constBegin () );
		const QMap<QString,uint>::const_iterator& itr_end ( city_count.constEnd () );
		const QString strtemplate ( TR_FUNC ( "\t%1: %2;" ) );
		while ( itr != itr_end )
		{
			m_readyFunc ( strtemplate.arg ( HTML_BOLD_FONT_11.arg ( itr.key () ), HTML_BOLD_ITALIC_UNDERLINE_11.arg ( itr.value () ) ) );
			++itr;
		}
	}
}

void dbStatisticsWorker::countJobs ()
{
	m_readyFunc ( TR_FUNC ( "\n\n\nTotal number of Jobs: " ) +
		HTML_BOLD_ITALIC_UNDERLINE_11.arg ( mVDB->recordCount ( &Job::t_info, mVDB ) ) + QStringLiteral ( "\n" ) );
	
	QSqlQuery queryRes ( *(mVDB->database ()) );
	queryRes.setForwardOnly ( true );
	if ( queryRes.exec ( QStringLiteral ( "SELECT STARTDATE,TIME,REPORT FROM JOBS"  ) ) )
	{
		queryRes.first ();
		vmNumber db_date;
		podList<uint> count_years ( 0, vmNumber::currentDate ().year () - 2009 + 1 );
		vmList<vmNumber> count_hours ( vmNumber (), count_years.preallocNumber () );
		podList<uint> count_days ( 0, count_years.preallocNumber () );
		do
		{
			db_date.fromTrustedStrDate ( queryRes.value ( 0 ).toString (), vmNumber::VDF_DB_DATE, false );
			if ( db_date.year () < 2009 ) continue;
			count_years[db_date.year () - 2009]++;
			count_hours[db_date.year () - 2009] += vmNumber ( queryRes.value ( 1 ).toString (), VMNT_TIME, vmNumber::VTF_DAYS );
			count_days[db_date.year () - 2009] += stringTable ( queryRes.value ( 2 ).toString () ).countRecords ();
		} while ( queryRes.next () );
		
		m_readyFunc ( TR_FUNC ( "\nNumber of jobs per year since 2009:\n" ) );
		const QString strtemplate ( TR_FUNC ( "\tJobs in %1: %2. Total number of worked hours: %3 ( %4 ). Worked days: %5;" ) );
		uint year ( 2009 );
		while ( year <= vmNumber::currentDate ().year () )
		{
			m_readyFunc ( strtemplate.arg ( HTML_BOLD_FONT_11.arg ( QString::number ( year ) ),
										  HTML_BOLD_ITALIC_UNDERLINE_11.arg ( count_years.at ( year-2009 ) ),
										  HTML_BOLD_ITALIC_UNDERLINE_11.arg ( count_hours.at ( year-2009 ).toTime ( vmNumber::VTF_DAYS ) ),
										  HTML_BOLD_ITALIC_UNDERLINE_11.arg ( count_hours.at ( year-2009 ).toTime ( vmNumber::VTF_FANCY ) ),
										  HTML_BOLD_ITALIC_UNDERLINE_11.arg ( count_days.at ( year-2009 ) ) ) );
			++year;
		}
	}
}

void dbStatisticsWorker::jobPrices ()
{
	QSqlQuery queryRes ( *(mVDB->database ()) );
	queryRes.setForwardOnly ( true );
	if ( queryRes.exec ( QStringLiteral ( "SELECT STARTDATE,PRICE FROM JOBS"  ) ) )
	{
		queryRes.first ();
		vmNumber db_date, price, total_income;
		podList<uint> count_jobs ( 0, vmNumber::currentDate ().year () - 2009 + 1 );
		vmList<vmNumber> count_price ( vmNumber (), count_jobs.preallocNumber () );
		
		do
		{
			db_date.fromTrustedStrDate ( queryRes.value ( 0 ).toString (), vmNumber::VDF_DB_DATE, false );
			if ( db_date.year () < 2009 ) continue;
			price = vmNumber ( queryRes.value ( 1 ).toString (), VMNT_PRICE, 1 );
			total_income += price;
			count_price[db_date.year () - 2009] += price;
			count_jobs[db_date.year () - 2009]++;
		} while ( queryRes.next () );
		
		m_readyFunc ( "" );
		m_readyFunc ( TR_FUNC ( "Total estimated income since 2009: " ) + HTML_BOLD_ITALIC_UNDERLINE_11.arg ( total_income.toPrice () ) );
		m_readyFunc ( TR_FUNC ( "\nTotal estimated income per year since 2009:\n" ) );
		
		const QString strtemplate ( TR_FUNC ( "\tEstimated Income for the year %1: %2, in a total of %3 jobs:" ) );
		uint year ( 2009 );
		while ( year <= vmNumber::currentDate ().year () )
		{
			
			m_readyFunc ( strtemplate.arg ( HTML_BOLD_FONT_11.arg ( QString::number ( year ) ),
										  HTML_BOLD_ITALIC_UNDERLINE_11.arg ( count_price.at ( year-2009 ).toPrice () ),
										  HTML_BOLD_ITALIC_UNDERLINE_11.arg ( count_jobs.at ( year-2009 ) ) ) );
			++year;
		}
	}
}

void dbStatisticsWorker::biggestJobs ()
{
	QSqlQuery queryRes ( *(mVDB->database ()) );
	queryRes.setForwardOnly ( true );
	if ( queryRes.exec ( QStringLiteral ( "SELECT ID,PRICE,TIME FROM JOBS"  ) ) )
	{
		queryRes.first ();
		vmNumber price ( 0.0 ), highest_price ( 0.0 ), n_days ( 0 ), n_days_max ( 0 );
		uint jobid_most_value ( 0 ), jobid_longest ( 0 );
		
		do
		{
			price = vmNumber ( queryRes.value ( 1 ).toString (), VMNT_PRICE, 1 );
			if ( price > highest_price )
			{
				highest_price = price;
				jobid_most_value = queryRes.value ( 0 ).toUInt ();
			}
			
			n_days = vmNumber ( queryRes.value ( 2 ).toString (), VMNT_TIME, vmNumber::VTF_DAYS );
			if ( n_days > n_days_max )
			{
				n_days_max = n_days;
				jobid_longest = queryRes.value ( 0 ).toUInt ();
			}
		} while ( queryRes.next () );
		
		m_readyFunc ( "" );
		Job job;
		if ( job.readRecord ( jobid_most_value ) )
		{
			m_readyFunc ( TR_FUNC ( "Highest income job:\n" ) );
			m_readyFunc ( Job::concatenateJobInfo ( job, true ) );
			m_readyFunc ( "" );
		}
		if ( job.readRecord ( jobid_longest ) )
		{
			m_readyFunc ( TR_FUNC ( "Longest job:\n" ) );
			m_readyFunc ( Job::concatenateJobInfo ( job, true ) );
			m_readyFunc ( "" );
		}
	}
}

void dbStatisticsWorker::countPayments ()
{
	QSqlQuery queryRes ( *(mVDB->database ()) );
	queryRes.setForwardOnly ( true );
	if ( queryRes.exec ( QStringLiteral ( "SELECT INFO FROM PAYMENTS"  ) ) )
	{
		queryRes.first ();
		vmNumber db_date, price, total_income;
		vmList<vmNumber> count_price ( vmNumber (), vmNumber::currentDate ().year () - 2009 + 1 );
		stringTable payInfo;
		stringRecord* payRecord ( nullptr );
		uint n_pays ( 0 );
		
		do
		{
			payInfo.fromString ( queryRes.value ( 0 ).toString () );
			n_pays = payInfo.countRecords ();
			for ( uint i ( 0 ); i < n_pays ; ++i )
			{
				payRecord = const_cast<stringRecord*>( &payInfo.readRecord ( i ) );
				db_date.fromTrustedStrDate ( payRecord->fieldValue ( PHR_DATE ), vmNumber::VDF_DB_DATE );
				if ( db_date.year () >= 2009 )
				{
					price.fromTrustedStrPrice ( payRecord->fieldValue ( PHR_VALUE ) );
					total_income += price;
					count_price[db_date.year () - 2009] += price;
				}
			}
		} while ( queryRes.next () );
		
		m_readyFunc ( "" );
		m_readyFunc ( TR_FUNC ( "Total actual income since 2009: " ) + HTML_BOLD_ITALIC_UNDERLINE_11.arg ( total_income.toPrice () ) );
		m_readyFunc ( TR_FUNC ( "\nTotal actual income per year since 2009:\n" ) );
		
		const QString strtemplate ( TR_FUNC ( "\tActual income for the year %1: %2" ) );
		uint year ( 2009 );
		while ( year <= vmNumber::currentDate ().year () )
		{
			
			m_readyFunc ( strtemplate.arg ( HTML_BOLD_FONT_11.arg ( QString::number ( year ) ),
										  HTML_BOLD_ITALIC_UNDERLINE_11.arg ( count_price.at ( year-2009 ).toPrice () )
						) );
			++year;
		}
	}
}
