#ifndef PASSWORDSESSIONMANAGER_H
#define PASSWORDSESSIONMANAGER_H

#include "keychain.h"

#include <QtCore/QHash>

class passwordSessionManager
{

public:
	
	enum PSM_SAVE_OPTIONS {
		SaveSession,
		SavePermanment,
		DoNotSave
	};

	passwordSessionManager () = default;
	~passwordSessionManager ();
	bool getPassword ( QString& passwd, const QString& service, const QString& id );
	bool getPassword_UserInteraction ( QString& passwd, const QString& service, const QString& id, const QString& message );

	bool savePassword ( PSM_SAVE_OPTIONS save_option, const QString& passwd, const QString& service, const QString& id );
	
private:
	vmPasswordManager::Error err_code { vmPasswordManager::NoError };
	QHash<QString,QString> tempPasswords;
};
#endif // PASSWORDSESSIONMANAGER_H
