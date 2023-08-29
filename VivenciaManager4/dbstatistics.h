#ifndef DBSTATISTICS_H
#define DBSTATISTICS_H

#include <QtCore/QObject>

#include <functional>

class textEditWithCompleter;
class QVBoxLayout;

#ifdef USE_THREADS
class dbStatistics final : public QObject
#else
class dbStatistics final
#endif
{

#ifdef USE_THREADS
Q_OBJECT
#endif

friend class VivenciaDB;

public:
	#ifdef USE_THREADS
	explicit dbStatistics ( QObject* parent = nullptr );
	#else
	explicit dbStatistics ();
	#endif
	virtual ~dbStatistics () final = default;

	inline QVBoxLayout* layout () const { return mainLayout; }
	void reload ();

#ifdef USE_THREADS
public slots:
	void writeInfo ( const QString& );
#endif

private:	
	textEditWithCompleter* m_textinfo;
	QVBoxLayout* mainLayout;
};

class dbStatisticsWorker : public QObject
{

#ifdef USE_THREADS
Q_OBJECT
#endif

public:
	explicit dbStatisticsWorker ();
	virtual ~dbStatisticsWorker () = default;
	
#ifdef USE_THREADS
public slots:
	void startWorks ();
	
signals:
	void infoProcessed ( const QString& );
	void finished ();

#else
	void startWorks ();
	void setCallbackForInfoReady ( const std::function<void( const QString& )>& func ) { m_readyFunc = func; }
#endif
	
private:
	void countClients ();
	void clientsPerYear ( const bool b_start );
	void activeClientsPerYear ();
	void clientsPerCity ();
	void countJobs ();
	void jobPrices ();
	void biggestJobs ();
	void countPayments ();
	
#ifndef USE_THREADS
	std::function<void( const QString& )> m_readyFunc;
#endif
};

#endif // DBSTATISTICS_H
