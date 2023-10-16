#ifndef DBSTATISTICS_H
#define DBSTATISTICS_H

#include <functional>

#include <QtCore/QObject>

class textEditWithCompleter;
class QVBoxLayout;
class VivenciaDB;

class dbStatistics
{

friend class VivenciaDB;

public:
	explicit dbStatistics ();
	~dbStatistics ();

	inline QVBoxLayout* layout () const { return mainLayout; }
	void reload ();

private:	
	textEditWithCompleter* m_textinfo;
	QVBoxLayout* mainLayout;
};

class dbStatisticsWorker final : public QObject
{

public:
	explicit dbStatisticsWorker ( QObject* parent = nullptr );
	virtual ~dbStatisticsWorker () final;
	
	void startWorks ();
	void setCallbackForInfoReady ( const std::function<void( const QString& )>& func ) { m_readyFunc = func; }
	
private:
	void countClients ();
	void clientsPerYear ( const bool b_start );
	void activeClientsPerYear ();
	void clientsPerCity ();
	void countJobs ();
	void jobPrices ();
	void biggestJobs ();
	void countPayments ();
	
	VivenciaDB* mVDB;
	std::function<void( const QString& )> m_readyFunc;
};

#endif // DBSTATISTICS_H
