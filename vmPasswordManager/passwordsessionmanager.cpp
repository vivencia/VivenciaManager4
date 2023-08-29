#include "passwordsessionmanager.h"
#include "vmlibs.h"
#include "heapmanager.h"

#include <vmNotify/vmnotify.h>

passwordSessionManager::~passwordSessionManager ()
{
	QHash<QString,QString>::const_iterator itr ( tempPasswords.constBegin () );
	const QHash<QString,QString>::const_iterator& itr_end ( tempPasswords.constEnd () );
	for ( ; itr != itr_end; ++itr )
	{
		vmPasswordManager::passwdManager_Delete pwd_d ( itr.key () );
		pwd_d.setKey ( itr.value () );
		pwd_d.start ();
	}
}

bool passwordSessionManager::getPassword ( QString& passwd, const QString& service, const QString& id )
{
	vmPasswordManager::passwdManager_Read pwd_r ( service );
	pwd_r.setKey ( id );
	pwd_r.start ();
	err_code = pwd_r.error ();
	if ( err_code == vmPasswordManager::NoError )
		passwd = pwd_r.textData ();
	return ( err_code == vmPasswordManager::NoError );
}

bool passwordSessionManager::getPassword_UserInteraction ( QString& passwd, const QString& service, const QString& id, const QString& message )
{
	if ( !getPassword ( passwd, service, id ) )
	{
		if ( err_code == vmPasswordManager::EntryNotFound )
		{
			if ( vmNotify::passwordBox ( passwd, nullptr, APP_TR_FUNC ( "Password required" ), message ) )
				return true;
		}
	}
	else
		return true;
	return false;
}

bool passwordSessionManager::savePassword ( PSM_SAVE_OPTIONS save_option, const QString& passwd, 
											const QString& service, const QString& id )
{
	if ( save_option != DoNotSave )
	{
		vmPasswordManager::passwdManager_Write pwd_w ( service );
		pwd_w.setKey ( id );
		pwd_w.setTextData ( passwd );
		pwd_w.start ();
		
		if ( save_option == SaveSession )
		{
			tempPasswords.insert ( service, id );
		}
		return ( err_code == vmPasswordManager::NoError );
	}
	return false;
}
