#ifndef CRASHRESTORE_H
#define CRASHRESTORE_H

#include <vmUtils/tristatetype.h>
#include <vmStringRecord/stringrecord.h>

#include <QtCore/QString>

class dataFile;

class crashRestore
{

friend class restoreManager;

public:
	explicit crashRestore ( const QString& str_id );
	~crashRestore ();

	bool needRestore ();

	inline bool isRestoring () const { return crashInfoLoaded.isOn (); }
	inline void setInfoLoaded ( const bool b_loaded ) { mbInfoLoaded = b_loaded; }
	inline bool infoLoaded () const { return mbInfoLoaded; }
	inline static const QChar recordSeparatorForTable () { return public_table_sep; }
	inline static const QChar fieldSeparatorForRecord () { return public_rec_sep; }
	
	int commitState ( const int id, const stringRecord& value );
	bool eliminateRestoreInfo ( const int id = -1 );

	const stringRecord& restoreFirstState () const;
	const stringRecord& restoreNextState () const;
	const stringRecord& restorePrevState () const;
	const stringRecord& restoreLastState () const;

	static inline void setNewDatabaseSession ( const bool b_newdb ) { crashRestore::bNewDBSettion = b_newdb; }
	static inline bool newDBSession () { return crashRestore::bNewDBSettion; }

private:
	const stringRecord& restoreState () const;
	void done ();

	mutable int m_statepos;
	QString m_filename;
	dataFile* fileCrash;
	triStateType crashInfoLoaded;
	mutable stringRecord m_stateinfo;
	mutable bool mbInfoLoaded;
	static bool bNewDBSettion;
};

class restoreManager
{
public:
	explicit restoreManager ();
	~restoreManager ();

	static void appExitingProcedures ();

	// called upon an exception state
	void saveSession ();

	inline void addRestoreInfo ( crashRestore* const crash )
	{
		if ( crashInfoList.contains ( crash ) == -1 )
			crashInfoList.append ( crash );
	}

	inline void removeRestoreInfo ( crashRestore* const crash ) { crashInfoList.removeOne ( crash ); }

	inline bool newDBSession () const { return mb_newDBSession; }
	inline void setNewDBSession () { mb_newDBSession = true; }
	
private:
	pointersList<crashRestore*> crashInfoList;
	bool mb_newDBSession;
};

#endif // CRASHRESTORE_H
