#include "crashrestore.h"
#include "vmlibs.h"
#include "heapmanager.h"

#include "vmtextfile.h"
#include "fileops.h"
#include "configops.h"

bool crashRestore::bNewDBSettion ( false );

restoreManager::restoreManager ()
	: crashInfoList ( 5 ), mb_newDBSession ( false )
{}

restoreManager::~restoreManager ()
{
	crashInfoList.clear ( true );
}

void restoreManager::appExitingProcedures ()
{
	pointersList<fileOps::st_fileInfo*> temp_files;
	configOps config;
	fileOps::lsDir ( temp_files, config.appDataDir (), QStringList () << QStringLiteral ( ".crash" ) );
	for ( uint i ( 0 ); i < temp_files.count (); ++i )
	{
		if ( fileOps::fileSize ( temp_files.at ( i )->fullpath ) <= 0 )
			fileOps::removeFile ( temp_files.at ( i )->fullpath );
	}
	temp_files.clear ( true );
}

void restoreManager::saveSession ()
{
	qDebug () << "Saving session ... ";
	/* We do not know the nature of the exeption that led here. So we attempt to save the current state information and then
	 * resume with program exiting process which, if successfull, will undo the saving state but better be safe than sorry
	 */
	crashRestore* crash ( crashInfoList.first () );
	while ( crash )
	{
		crash->done ();
		crash = crashInfoList.next ();
	}
}

crashRestore::crashRestore ( const QString& str_id )
	: m_statepos ( -1 ), mbInfoLoaded ( false )
{
	m_filename = configOps::appDataDir () + str_id + QStringLiteral ( ".crash" );
	fileCrash = new dataFile ( m_filename );
	// by using different separators we make sure there is no conflict with the data saved
	fileCrash->setRecordSeparationChar ( recordSeparatorForTable (), fieldSeparatorForRecord () );
}

crashRestore::~crashRestore ()
{
	heap_del ( fileCrash );
}

void crashRestore::done ()
{
	fileCrash->remove ();
	crashInfoLoaded.setOff ();
	//heap_del ( fileCrash ); //TEST
}

bool crashRestore::needRestore ()
{
	if ( crashRestore::newDBSession () )
		return false;
	
	if ( crashInfoLoaded.isUndefined () )
		crashInfoLoaded = fileCrash->load ();
	return crashInfoLoaded.isOn ();
}

int crashRestore::commitState ( const int id, const stringRecord& value )
{
	int ret ( id );
	if ( id == -1 )
	{
		ret = static_cast<int>( fileCrash->recCount () );
		fileCrash->appendRecord ( value );
	}
	else
	{
		stringRecord temp_rec;
		temp_rec.setFieldSeparationChar ( fieldSeparatorForRecord () );
		if ( fileCrash->getRecord ( temp_rec, id ) )
		{
			if ( temp_rec.toString () != value.toString () )
				fileCrash->changeRecord ( id, value );
		}
		else
			fileCrash->insertRecord ( id, value );
	}
	fileCrash->commit ();
	if ( fileCrash->recCount () > 0 )
		crashInfoLoaded.setOn ();
	return ret;
}

// The rationale here is to check for the deletion only. When it fails, signal the caller. When id == -1, there is no
// record deletion, but whole file deletion. That will not fail because done () is a cleanup routine. When the record deletion
// fails, the caller must then call this procedure with id = 1, because the crash file must be corrupted
bool crashRestore::eliminateRestoreInfo ( const int id )
{
	bool b_deleted_ok ( true );
	
	if ( id != -1 )
		b_deleted_ok = fileCrash->deleteRecord ( id );

	if ( ( id == -1 ) || ( fileCrash->recCount () == 0 ) )
		done ();
	
	return b_deleted_ok;
}

const stringRecord& crashRestore::restoreState () const
{
	if ( crashInfoLoaded.isOn () )
	{
		fileCrash->getRecord ( m_stateinfo, m_statepos );
		return m_stateinfo;
	}
	return emptyStrRecord;
}

const stringRecord& crashRestore::restoreFirstState () const
{
	m_statepos = 0;
	return restoreState ();
}

const stringRecord& crashRestore::restoreNextState () const
{
	if ( m_statepos < static_cast<int>( fileCrash->recCount () - 1 ) )
	{
		++m_statepos;
		restoreState ();
		if ( m_stateinfo.isOK () )
		{
			mbInfoLoaded = ( static_cast<uint>( m_statepos ) == fileCrash->recCount () - 1 );
			return m_stateinfo;
		}
	}
	return emptyStrRecord;
}

const stringRecord& crashRestore::restorePrevState () const
{
	if ( m_statepos > 0 )
	{
		--m_statepos;
		return restoreState ();
	}
	return emptyStrRecord;
}

const stringRecord& crashRestore::restoreLastState () const
{
	m_statepos = static_cast<int>( fileCrash->recCount () );
	return restoreState ();
}
