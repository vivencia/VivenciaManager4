#ifndef FIXDATABASE_H
#define FIXDATABASE_H

#include "vmlist.h"

#include <QtCore/QString>

class passwordSessionManager;
class QFile;

class fixDatabase
{

public:
	explicit fixDatabase ();
	~fixDatabase ();

	enum CheckResult {
		CR_UNDEFINED = -1, CR_OK = 0, CR_TABLE_CORRUPTED = 1, CR_TABLE_CRASHED = 2
	};

	struct badTable {
		QString table;
		QString err;
		CheckResult result;
	};

	inline bool needsFixing () const {
		return b_needsfixing;
	}

	bool checkDatabase ();
	bool fixTables ( const QString& table = QLatin1String ( "*" ) );

	inline void badTables ( pointersList<fixDatabase::badTable*>& tables ) const {
		tables = m_badtables;
	}

private:
	bool readOutputFile ( const QString& r_passwd );

	pointersList<fixDatabase::badTable*> m_badtables;
	bool b_needsfixing;
	QString mStrOutput;
    //passwordSessionManager* m_pwdMngr;
};

#endif // FIXDATABASE_H
