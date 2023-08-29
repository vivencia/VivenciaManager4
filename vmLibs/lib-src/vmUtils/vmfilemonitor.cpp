#include "vmfilemonitor.h"
#include "vmlibs.h"
#include "fileops.h"

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include <climits>

extern "C"
{
	#include <sys/inotify.h>
	#include <unistd.h>
}

struct vmFileMonitor::watched_files
{
	QString filename;
	int wd;
	uint event;
};

vmFileMonitor::vmFileMonitor ()
	: QObject ( nullptr ), filesList ( 3 ), m_inotifyFd ( -1 ), mb_paused ( false ), m_eventFunc ( nullptr )
{
	filesList.setAutoDeleteItem ( true );
	workerThread = new QThread;
	moveToThread ( workerThread );
	static_cast<void>( connect ( workerThread, SIGNAL ( started () ), this, SLOT ( startWorks () ) ) );
	static_cast<void>( connect ( this, &vmFileMonitor::wait, workerThread, [&] () { return workerThread->wait ( 1000 ); } ) );
	static_cast<void>( connect ( this, SIGNAL ( finished () ), workerThread, SLOT ( quit () ) ) );
	static_cast<void>( connect ( this, SIGNAL( finished () ), this, SLOT ( deleteLater () ) ) );
	static_cast<void>( connect ( workerThread, SIGNAL ( finished () ), workerThread, SLOT ( deleteLater () ) ) );
}

vmFileMonitor::~vmFileMonitor ()
{
	close ( m_inotifyFd );
	m_inotifyFd = -1;
	//emit finished ();
}

bool vmFileMonitor::paused () const
{
	if ( mb_paused )
	{
		QMutex mutex;
		mutex.lock ();
		QWaitCondition waitCondition;
		waitCondition.wait ( &mutex, 300 );
		mutex.unlock ();
		return paused ();
	}
	return false;
}

void vmFileMonitor::startWorks ()
{
	const size_t BUF_LEN ( 10 * (sizeof(struct inotify_event) + NAME_MAX + 1) );
	ssize_t numRead;
	char buf[BUF_LEN];
	char* p ( nullptr );
	struct inotify_event* event ( nullptr );

	// Read events forever
	for (;;)
	{
		numRead = read ( m_inotifyFd, buf, BUF_LEN );

		if ( numRead == -1 )
		{
			DBG_OUT ( "read() from inotify fd returned ", numRead, true, true )
			break;
		}
		/*if ( numRead == 0 )
		{
			DBG_OUT ( "read() from inotify fd returned 0", true, true )
		}

		if ( numRead == -1 )
		{
			DBG_OUT ( "read() from inotify fd returned -1", true, true )
			emit finished ();
		}*/

		if ( mb_paused )
		{
			//if ( !paused () )
			//{
			//	::memset ( static_cast<void*>( buf ), 0, sizeof ( buf ) );
				continue;
			//}
		}

		for ( p = buf; p < buf + numRead; )
		{
			event = reinterpret_cast<struct inotify_event*>( p );
			if ( event->len )
				handleInotifyEvent ( event );
			p += sizeof(struct inotify_event) + event->len;
		}
	}
}

void vmFileMonitor::startMonitoring ( const QString& filename, const uint event )
{
	if ( m_eventFunc == nullptr )
		return;

	//Create inotify instance
	if ( m_inotifyFd == -1 )
	{
		m_inotifyFd = inotify_init ();
		if ( m_inotifyFd == -1 )
		{
			DBG_OUT ( "inotify instance creation error", "", false, true )
			return;
		}
	}

	// Do not attempt to monitor a directory already being monitored
	if ( getWatchedFileInfo ( fileOps::fileNameWithoutPath ( filename ) ) != -1 )
		return;

	const QString dir ( fileOps::dirFromPath ( filename ) );
	const int wd ( inotify_add_watch ( m_inotifyFd, dir.toUtf8 ().constData(), IN_ALL_EVENTS ) );
	if ( wd == -1 )
	{
		DBG_OUT ( "inotify add watch failed: ", filename, true, true )
		return;
	}

	qDebug () << "inotify add watch succeeded. wd == " << wd;
	auto new_watch = new watched_files;
	new_watch->filename = fileOps::fileNameWithoutPath ( filename );
	new_watch->wd = wd;
	new_watch->event = event;
	filesList.append ( new_watch );
	workerThread->start ( QThread::HighestPriority );
}

void vmFileMonitor::stopMonitoring ( const QString& filename )
{
	const int idx ( getWatchedFileInfo ( fileOps::fileNameWithoutPath ( filename ) ) );
	if ( idx >= 0 )
	{
		if ( inotify_rm_watch ( m_inotifyFd, filesList.at ( idx )->wd ) == -1 )
		{
			DBG_OUT ( "inotify rm watch failed: ", filename, true, true )
			emit finished ();
		}
		else
			filesList.remove ( static_cast<int>( idx ), true );
	}
}

int vmFileMonitor::getWatchedFileInfo ( const QString& filename )
{
	for ( int i ( 0 ); i < static_cast<int>( filesList.count () ); ++i )
	{
		if ( filesList.at ( i )->filename == filename )
			return i;
	}
	return -1;
}

void vmFileMonitor::handleInotifyEvent ( struct inotify_event* i_ev )
{
	const watched_files* w_file ( filesList.at ( getWatchedFileInfo ( i_ev->name ) ) );
	if ( w_file == nullptr )
		return;

	//if ( i_ev->mask & w_file->event )
	m_eventFunc ( w_file->filename, i_ev->mask );
}
