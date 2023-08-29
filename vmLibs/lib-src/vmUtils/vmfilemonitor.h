#ifndef VMFILEMONITOR_H
#define VMFILEMONITOR_H

#include "vmlist.h"

#include <QtCore/QObject>
#include <QtCore/QString>

#include <functional>

// Events supported by inotify copied verbatim from sys/inotify.h. Avoids the inclusion of that header here or in all modules that use our class
#define VM_IN_ACCESS	 0x00000001	/* File was accessed.  */
#define VM_IN_MODIFY	 0x00000002	/* File was modified.  */
#define VM_IN_ATTRIB	 0x00000004	/* Metadata changed.  */
#define VM_IN_CLOSE_WRITE	 0x00000008	/* Writtable file was closed.  */
#define VM_IN_CLOSE_NOWRITE 0x00000010	/* Unwrittable file closed.  */
#define VM_IN_CLOSE	 (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE) /* Close.  */
#define VM_IN_OPEN		 0x00000020	/* File was opened.  */
#define VM_IN_MOVED_FROM	 0x00000040	/* File was moved from X.  */
#define VM_IN_MOVED_TO      0x00000080	/* File was moved to Y.  */
#define VM_IN_MOVE		 (IN_MOVED_FROM | IN_MOVED_TO) /* Moves.  */
#define VM_IN_CREATE	 0x00000100	/* Subfile was created.  */
#define VM_IN_DELETE	 0x00000200	/* Subfile was deleted.  */
#define VM_IN_DELETE_SELF	 0x00000400	/* Self was deleted.  */
#define VM_IN_MOVE_SELF	 0x00000800	/* Self was moved.  */

struct inotify_event;

class vmFileMonitor : public QObject
{

Q_OBJECT

public:
	vmFileMonitor ();
	virtual ~vmFileMonitor ();

	inline void setCallbackForEvent ( const std::function<void( const QString& filename, const uint event )>& func ) {
		m_eventFunc = func;
	}

public slots:
	void startWorks ();// __attribute__ ((noreturn));
	void startMonitoring ( const QString& filename, const uint event );
	void stopMonitoring ( const QString& filename );
	inline void pauseMonitoring () { mb_paused = true; }
	inline void resumeMonitoring () { mb_paused = false; }

signals:
	void wait ();
	void fileEventCaught ( const QString& );
	void finished ();

private:
	bool paused () const;
	int getWatchedFileInfo ( const QString& filename );
	void handleInotifyEvent ( struct inotify_event* i_ev );

	QThread* workerThread;
	struct watched_files;
	pointersList<watched_files*> filesList;
	int m_inotifyFd;
	bool mb_paused;
	std::function<void( const QString& filename, const uint event )> m_eventFunc;
};

#endif // VMFILEMONITOR_H
