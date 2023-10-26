#include "fixdatabase.h"
#include "vmlibs.h"
#include "heapmanager.h"

#include "vivenciadb.h"
#include "fixdatabaseui.h"

#include <vmUtils/fileops.h>

#include <vmNotify/vmnotify.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QApplication>

static const QString strOutError ( QStringLiteral ( "myisamchk: error:" ) );
static const QString strOutTableUpperLimiter ( QStringLiteral ( "Checking MyISAM file" ) );
static const QLatin1String strOutTableLowerLimiter ( "------" );
static const QLatin1String fixApp ( "myisamchk" );
static const QLatin1String fixAppVerify ( " -e " );
static const QLatin1String fixAppSafeRecover ( " --safe-recover --correct-checksum --backup " );
static const QString mysqlLibDir ( QStringLiteral ( "/var/lib/mysql/%1/%2.MYI > %3 2>&1" ) );
static const QLatin1String tempFile ( "/tmp/.vm-fixdb" );

static void commandQueue ( QString& main_command )
{
	main_command += QLatin1String ( "; " ) + _mysql_init_script + CHR_SPACE + QStringLiteral ( "stop" ) +
					QLatin1String ( "; " ) + _mysql_init_script + CHR_SPACE + QStringLiteral ( "start" );
}

fixDatabase::fixDatabase ()
    : m_badtables ( 5 ), b_needsfixing ( false )
{}

fixDatabase::~fixDatabase ()
{
	m_badtables.clear ( true );
}

bool fixDatabase::readOutputFile ( const QString& r_passwd )
{
	bool ok ( false );
	if ( fileOps::exists ( tempFile ).isOn () )
	{
		QFile file ( tempFile );
		if ( file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
		{
			mStrOutput = file.readAll ();
			file.close ();
			ok = true;
		}
		static_cast<void>( fileOps::sysExec ( sudoCommand.arg ( r_passwd, QStringLiteral ( "rm -f " ) + tempFile ) ) );
	}
	return ok;
}

bool fixDatabase::checkDatabase ()
{
	// The server should be stopped for this operation to avoid making more errors when trying to fix them
    QString r_passwd ( QStringLiteral( "percival" ) );
    QString command ( fixApp + fixAppVerify + mysqlLibDir.arg ( DB_NAME, QStringLiteral ( "*" ), tempFile ) );
		commandQueue ( command );
		static_cast<void>( fileOps::sysExec ( sudoCommand.arg ( r_passwd, QLatin1String ( "rm -f " ) + tempFile ) ) );

		if ( readOutputFile ( r_passwd ) )
		{
			b_needsfixing = false;
			m_badtables.clear ( true );
			int idx ( mStrOutput.indexOf ( strOutTableUpperLimiter ) );
			int idx2 ( 0 );
			int idx3 ( 0 );
			const int len1 ( strOutTableUpperLimiter.length () + 1 );
			const int len2 ( DB_NAME.count () + 1 );
			const int len3 ( strOutError.length () + 1 );
			badTable* bt ( nullptr );

			while ( idx != -1 )
			{
				idx = mStrOutput.indexOf ( QStringLiteral ( ".MYI" ), idx2 + len1 );
				if ( idx == -1 )
					break;
				idx2 = mStrOutput.indexOf ( DB_NAME, idx2 + len1 ) + len2;
				if ( idx2 == -1 )
					break;
				bt = new badTable;
				bt->table = mStrOutput.mid ( idx2, idx - idx2 );
				idx = mStrOutput.indexOf ( strOutError, idx2 + 1 );
				if ( idx != -1 ) {
					idx3 = mStrOutput.indexOf ( strOutTableLowerLimiter, idx2 );
					if ( idx < idx3 )
					{
						idx2 = mStrOutput.indexOf ( CHR_NEWLINE, idx + len3 );
						idx += len3;
						bt->err = mStrOutput.mid ( idx , idx2 - idx );
						bt->result = CR_TABLE_CORRUPTED;
						b_needsfixing = true;
					}
					else
					{
						bt->err = QStringLiteral ( "OK" );
						bt->result = CR_OK;
					}
				}
				else
				{
					bt->err = QStringLiteral ( "OK" );
					bt->result = CR_OK;
				}
				m_badtables.append ( bt );
				idx = idx2 = mStrOutput.indexOf ( strOutTableUpperLimiter, idx2 + 1 );
			}
			return true;
		}
	return false;
}

bool fixDatabase::fixTables ( const QString& tables )
{
    QString r_passwd ( QStringLiteral ( "percival " ) );
    QString command ( fixApp + fixAppSafeRecover + mysqlLibDir.arg ( DB_NAME, tables, tempFile ) );
    commandQueue ( command );
    static_cast<void>( fileOps::sysExec ( sudoCommand.arg ( r_passwd, command ), QString () ) );
    if ( readOutputFile ( r_passwd ) )
        return ( !mStrOutput.contains ( QStringLiteral ( "err" ) ) && mStrOutput.contains ( QStringLiteral ( "recovering" ) ) );
	return false;
}
