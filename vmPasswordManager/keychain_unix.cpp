/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>	  *
 *																			*
 * This program is distributed in the hope that it will be useful, but		*
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution		*
 * details, check the accompanying file 'COPYING'.							*
 *****************************************************************************/

#include "kwallet_interface.h"
#include "keychain.h"
#include "keychain_p.h"
#include "gnomekeyring_p.h"

#include <vmUtils/fileops.h>

#include <QtDBus/QtDBus>
#include <QtCore/QSettings>
#include <QtCore/QScopedPointer>

static QString typeKey ( const QString& key )
{
	return QString::fromLatin1 ( "%1/type" ).arg ( key );
}

static QString dataKey ( const QString& key )
{
	return QString::fromLatin1 ( "%1/data" ).arg ( key );
}

enum KeyringBackend {
	Backend_GnomeKeyring,
	Backend_Kwallet4,
	Backend_Kwallet5
};

static KeyringBackend getKeyringBackend ()
{
	switch ( fileOps::detectDesktopEnvironment () )
	{
		case fileOps::DesktopEnv_Plasma5: return Backend_Kwallet5; break;
		case fileOps::DesktopEnv_Kde4: return Backend_Kwallet4; break;
		default:
			return GnomeKeyring::isAvailable () ? Backend_GnomeKeyring : Backend_Kwallet4; break;
	}
}

static void kwalletReadPasswordstartPwdProcessImpl ( const char* service, const char* path, passwdManagerPrivate_Read* priv )
{
	if ( QDBusConnection::sessionBus ().isConnected () )
	{
		priv->iface = new org::kde::KWallet ( QLatin1String ( service ), QLatin1String ( path ), QDBusConnection::sessionBus (), priv );
		const QDBusPendingReply<QString> reply = priv->iface->networkWallet ();
		auto watcher = new QDBusPendingCallWatcher ( reply, priv );
		priv->connect ( watcher, &QDBusPendingCallWatcher::finished, priv, [&, priv] ( QDBusPendingCallWatcher* self ) { return priv->kwalletWalletFound ( self ); } );
	}
	else
	{
		// D-Bus is not reachable so none can tell us something about KWalletd
		QDBusError err ( QDBusError::NoServer, passwdManagerPrivate_Read::tr ( "D-Bus is not running" ) );
		priv->fallbackOnError ( err );
	}
}

void passwdManagerPrivate_Read::startPwdProcess ()
{
	switch ( getKeyringBackend () )
	{
		case Backend_Kwallet5:
			kwalletReadPasswordstartPwdProcessImpl ( "org.kde.kwalletd5", "/modules/kwalletd5", this );
		break;
		case Backend_GnomeKeyring:
			this->mode = passwdManagerPrivate::Text;
            if ( !GnomeKeyring::find_network_password ( key.toUtf8().constData(),
                    q->service().toUtf8().constData(), "plaintext",
                    reinterpret_cast<GnomeKeyring::OperationGetStringCallback>( &passwdManagerPrivate::gnomeKeyring_readCb ), this, nullptr ) )
				q->emitFinishedWithError ( OtherError, tr ( "Unknown error" ) );
		break;
		case Backend_Kwallet4:
			kwalletReadPasswordstartPwdProcessImpl ( "org.kde.kwalletd", "/modules/kwalletd", this );
		break;
	}
}

void passwdManagerPrivate::kwalletWalletFound ( QDBusPendingCallWatcher* watcher )
{
	watcher->deleteLater ();
	const QDBusPendingReply<QString> reply ( *watcher );
	const QDBusPendingReply<int> pendingReply ( iface->open ( reply.value (), 0, q->service () ) );
	auto pendingWatcher = new QDBusPendingCallWatcher( pendingReply, this );
	connect ( pendingWatcher, &QDBusPendingCallWatcher::finished, this, [&] ( QDBusPendingCallWatcher* self )
			 { return kwalletOpenFinished ( self ); } );
}

static QPair<Error, QString> mapGnomeKeyringError( int result )
{
	Q_ASSERT( result != GnomeKeyring::RESULT_OK );

	switch ( result ) {
	case GnomeKeyring::RESULT_DENIED:
		return qMakePair( AccessDenied, QObject::tr("Access to keychain denied") );
	case GnomeKeyring::RESULT_NO_KEYRING_DAEMON:
		return qMakePair( NoBackendAvailable, QObject::tr("No keyring daemon") );
	case GnomeKeyring::RESULT_ALREADY_UNLOCKED:
		return qMakePair( OtherError, QObject::tr("Already unlocked") );
	case GnomeKeyring::RESULT_NO_SUCH_KEYRING:
		return qMakePair( OtherError, QObject::tr("No such keyring") );
	case GnomeKeyring::RESULT_BAD_ARGUMENTS:
		return qMakePair( OtherError, QObject::tr("Bad arguments") );
	case GnomeKeyring::RESULT_IO_ERROR:
		return qMakePair( OtherError, QObject::tr("I/O error") );
	case GnomeKeyring::RESULT_CANCELLED:
		return qMakePair( OtherError, QObject::tr("Cancelled") );
	case GnomeKeyring::RESULT_KEYRING_ALREADY_EXISTS:
		return qMakePair( OtherError, QObject::tr("Keyring already exists") );
	case GnomeKeyring::RESULT_NO_MATCH:
		return qMakePair(  EntryNotFound, QObject::tr("No match") );
	default:
		break;
	}

	return qMakePair( OtherError, QObject::tr("Unknown error") );
}

void passwdManagerPrivate::gnomeKeyring_readCb( int result, const char* string, passwdManagerPrivate* self )
{
	if ( result == GnomeKeyring::RESULT_OK ) {
		if (self->mode == passwdManagerPrivate::Text)
			self->data = QByteArray(string);
		else
			self->data = QByteArray::fromBase64(string);

		self->q->emitFinished();
	} else if (self->mode == passwdManagerPrivate::Text) {
		self->mode = passwdManagerPrivate::Binary;
		if ( !GnomeKeyring::find_network_password( self->key.toUtf8().constData(),
												   self->q->service().toUtf8().constData(),
												   "base64",
												   reinterpret_cast<GnomeKeyring::OperationGetStringCallback>( &passwdManagerPrivate::gnomeKeyring_readCb ),
												   self, nullptr ) )
			self->q->emitFinishedWithError( OtherError, tr("Unknown error") );
	} else {
		const QPair<Error, QString> errorResult = mapGnomeKeyringError( result );
		self->q->emitFinishedWithError( errorResult.first, errorResult.second );
	}
}

void passwdManagerPrivate_Read::fallbackOnError(const QDBusError& err )
{
	QScopedPointer<QSettings> local( !q->settings() ? new QSettings( q->service() ) : nullptr );
	QSettings* actual = q->settings() ? q->settings() : local.data();

	if ( q->insecureFallback() && actual->contains( dataKey( key ) ) )
	{
		mode = passwdManagerPrivate::stringToMode( actual->value( typeKey( key ) ).toString() );
		data = actual->value( dataKey( key ) ).toByteArray();

		q->emitFinished();
	}
	else
	{
		if ( err.type() == QDBusError::ServiceUnknown ) //KWalletd not running
			q->emitFinishedWithError( NoBackendAvailable, tr("No keychain service available") );
		else
			q->emitFinishedWithError( OtherError, tr("Could not open wallet: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
	}
}

void passwdManagerPrivate_Read::kwalletOpenFinished ( QDBusPendingCallWatcher* watcher )
{
	watcher->deleteLater ();
	const QDBusPendingReply<int> reply ( *watcher );

	QScopedPointer<QSettings> local ( !q->settings () ? new QSettings ( q->service () ) : nullptr );
	QSettings* actual = ( q->settings () ? q->settings () : local.data () );

	if ( reply.isError () )
	{
		fallbackOnError ( reply.error () );
		return;
	}

	if ( actual->contains ( dataKey ( key ) ) )
	{
		// We previously stored data in the insecure QSettings, but now have KWallet available.
		// Do the migration

		data = actual->value ( dataKey( key ) ).toByteArray ();
		const passwdManagerPrivate_Write::Mode mode ( passwdManagerPrivate_Write::stringToMode ( actual->value ( typeKey ( key ) ).toString() ) );
		actual->remove( key );
		q->emitFinished ();

		passwdManager_Write* j ( new passwdManager_Write ( q->service () ) );
		j->setSettings ( q->settings () );
		j->setKey ( key );
		j->setAutoDelete ( true );
		if ( mode == passwdManagerPrivate_Write::Binary )
			j->setBinaryData ( data );
		else if ( mode == passwdManagerPrivate_Write::Text )
			j->setTextData ( QString::fromUtf8 ( data ) );
		else
			Q_ASSERT( false );

		j->start ();

		return;
	}

	walletHandle = reply.value ();

	if ( walletHandle < 0 ) {
		q->emitFinishedWithError ( AccessDenied, tr ( "Access to keychain denied" ) );
		return;
	}

	const QDBusPendingReply<int> nextReply ( iface->entryType( walletHandle, q->service (), key, q->service() ) );
	auto nextWatcher ( new QDBusPendingCallWatcher ( nextReply, this ) );
	connect( nextWatcher, &QDBusPendingCallWatcher::finished, this, [&] ( QDBusPendingCallWatcher* self ) { return kwalletEntryTypeFinished ( self ); } );
}

//Must be in sync with KWallet::EntryType (kwallet.h)
enum KWalletEntryType
{
	Unknown = 0,
	Password,
	Stream,
	Map
};

void passwdManagerPrivate_Read::kwalletEntryTypeFinished ( QDBusPendingCallWatcher* watcher )
{
	watcher->deleteLater ();
	if ( watcher->isError () )
	{
		const QDBusError err ( watcher->error () );
		q->emitFinishedWithError ( OtherError, tr ( "Could not determine data type: %1; %2" ).arg ( QDBusError::errorString ( err.type () ), err.message() ) );
		return;
	}

	const QDBusPendingReply<int> reply ( *watcher );
	const int value ( reply.value () );

	switch ( value )
	{
		case Unknown:
			q->emitFinishedWithError ( EntryNotFound, tr ( "Entry not found" ) );
		return;
		case Password:
			mode = Text;
		break;
		case Stream:
			mode = Binary;
		break;
		case Map:
			q->emitFinishedWithError ( EntryNotFound, tr ( "Unsupported entry type 'Map'" ) );
		return;
		default:
			q->emitFinishedWithError( OtherError, tr ( "Unknown kwallet entry type '%1'" ).arg ( value ) );
		return;
	}

	const QDBusPendingCall nextReply = ( mode == Text )
			? QDBusPendingCall ( iface->readPassword ( walletHandle, q->service (), key, q->service () ) )
			: QDBusPendingCall ( iface->readEntry ( walletHandle, q->service (), key, q->service () ) );
	auto nextWatcher ( new QDBusPendingCallWatcher ( nextReply, this ) );
	connect ( nextWatcher, &QDBusPendingCallWatcher::finished, this, [&] ( QDBusPendingCallWatcher* self ) { return kwalletFinished ( self ); } );
}

void passwdManagerPrivate_Read::kwalletFinished ( QDBusPendingCallWatcher* watcher )
{
	if ( !watcher->isError () )
	{
		if ( mode == Binary )
		{
			QDBusPendingReply<QByteArray> reply ( *watcher );
			if ( reply.isValid () )
				data = reply.value ();
		} 
		else
		{
			QDBusPendingReply<QString> reply ( *watcher );
			if ( reply.isValid () )
				data = reply.value ().toUtf8 ();
		}
	}

	passwdManagerPrivate::kwalletFinished ( watcher );
}

static void kwalletWritePasswordstartPwdProcess ( const char* service, const char* path, passwdManagerPrivate* priv )
{
	if ( QDBusConnection::sessionBus().isConnected () )
	{
		priv->iface = new org::kde::KWallet ( QLatin1String ( service ), QLatin1String ( path ), QDBusConnection::sessionBus (), priv );
		const QDBusPendingReply<QString> reply ( priv->iface->networkWallet () );
		auto watcher ( new QDBusPendingCallWatcher( reply, priv ) );
		priv->connect ( watcher, &QDBusPendingCallWatcher::finished, priv, [&, priv] ( QDBusPendingCallWatcher* self ) { return priv->kwalletWalletFound ( self ); } );
	}
	else
	{
		// D-Bus is not reachable so none can tell us something about KWalletd
		QDBusError err ( QDBusError::NoServer, passwdManagerPrivate_Write::tr ( "D-Bus is not running" ) );
		priv->fallbackOnError ( err );
	}
}

void passwdManagerPrivate_Write::startPwdProcess ()
{
	switch ( getKeyringBackend () )
	{
		case Backend_Kwallet5:
			kwalletWritePasswordstartPwdProcess ( "org.kde.kwalletd5", "/modules/kwalletd5", this );
		break;

		case Backend_GnomeKeyring:
		{
			QString type;
			QByteArray password;
			if ( mode == passwdManagerPrivate::Text )
			{
				type = "plaintext";
				password = data;
			}
			else
			{
				type = "base64";
				password = data.toBase64();
			}

			QByteArray service = q->service ().toUtf8 ();
			if ( !GnomeKeyring::store_network_password ( GnomeKeyring::GNOME_KEYRING_DEFAULT,
				service.constData (), key.toUtf8 ().constData (), service.constData (), type.toUtf8().constData (),
				password.constData (), reinterpret_cast<GnomeKeyring::OperationDoneCallback>( &passwdManagerPrivate::gnomeKeyring_writeCb ), this, nullptr ) )
					q->emitFinishedWithError( OtherError, tr("Unknown error") );
		}
		break;

		case Backend_Kwallet4:
			kwalletWritePasswordstartPwdProcess ( "org.kde.kwalletd", "/modules/kwalletd", this );
		break;
	}
}

void passwdManagerPrivate_Write::fallbackOnError(const QDBusError &err)
{
	QScopedPointer<QSettings> local( !q->settings() ? new QSettings( q->service() ) : nullptr );
	QSettings* actual = q->settings() ? q->settings() : local.data();

	if ( !q->insecureFallback() ) {
		q->emitFinishedWithError( OtherError, tr("Could not open wallet: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
		return;
	}

	actual->setValue( QString::fromLatin1( "%1/type" ).arg( key ), mode );
	actual->setValue( QString::fromLatin1( "%1/data" ).arg( key ), data );
	actual->sync();

	q->emitFinished();
}

void passwdManagerPrivate::gnomeKeyring_writeCb(int result, passwdManagerPrivate* self )
{
	if ( result == GnomeKeyring::RESULT_OK ) {
		self->q->emitFinished();
	} else {
		const QPair<Error, QString> errorResult = mapGnomeKeyringError( result );
		self->q->emitFinishedWithError( errorResult.first, errorResult.second );
	}
}

void passwdManagerPrivate::kwalletOpenFinished ( QDBusPendingCallWatcher* watcher )
{
	watcher->deleteLater ();
	QDBusPendingReply<int> reply ( *watcher );

	QScopedPointer<QSettings> local( !q->settings () ? new QSettings( q->service () ) : nullptr );
	QSettings* actual ( q->settings () ? q->settings () : local.data () );

	if ( reply.isError () )
	{
		fallbackOnError ( reply.error () );
		return;
	}

	if ( actual->contains ( key ) )
	{
		// If we had previously written to QSettings, but we now have a kwallet available, migrate and delete old insecure data
		actual->remove ( key );
		actual->sync ();
	}

	const int handle = reply.value();

	if ( handle < 0 ) {
		q->emitFinishedWithError( AccessDenied, tr("Access to keychain denied") );
		return;
	}

	QDBusPendingReply<int> nextReply;

	if ( mode == Text )
		nextReply = iface->writePassword( handle, q->service(), key, QString::fromUtf8(data), q->service() );
	else if ( mode == Binary )
		nextReply = iface->writeEntry( handle, q->service(), key, data, q->service() );
	else
		nextReply = iface->removeEntry( handle, q->service(), key, q->service() );

	auto nextWatcher ( new QDBusPendingCallWatcher( nextReply, this ) );
	connect ( nextWatcher, &QDBusPendingCallWatcher::finished, this, [&] ( QDBusPendingCallWatcher* self ) { return kwalletFinished (self ); } );
}

void passwdManagerPrivate::kwalletFinished ( QDBusPendingCallWatcher* watcher )
{
	if ( !watcher->isError () )
	{
		if ( mode == Binary )
		{
			QDBusPendingReply<QByteArray> reply ( *watcher );
			if ( reply.isValid () )
				data = reply.value ();
		} 
		else
		{
			QDBusPendingReply<QString> reply ( *watcher );
			if ( reply.isValid () )
				data = reply.value ().toUtf8 ();
		}
	}
	q->emitFinished ();
}

void passwdManagerPrivate_Delete::startPwdProcess ()
{
	switch ( getKeyringBackend () )
	{
		case Backend_Kwallet5:
			kwalletWritePasswordstartPwdProcess ( "org.kde.kwalletd5", "/modules/kwalletd5", this );
		break;

		case Backend_GnomeKeyring:
		{
			if ( !GnomeKeyring::delete_network_password(
				 key.toUtf8 ().constData (), q->service ().toUtf8 ().constData (),
				 reinterpret_cast<GnomeKeyring::OperationDoneCallback>( &passwdManagerPrivate::gnomeKeyring_writeCb ),
				 this, nullptr ) )
			q->emitFinishedWithError ( OtherError, tr ("Unknown error" ) );
		}
		break;

		case Backend_Kwallet4:
			kwalletWritePasswordstartPwdProcess ( "org.kde.kwalletd", "/modules/kwalletd", this );
		break;
	}
}

void passwdManagerPrivate_Delete::fallbackOnError ( const QDBusError& err )
{
	QScopedPointer<QSettings> local ( !q->settings() ? new QSettings( q->service () ) : nullptr );
	QSettings* actual ( q->settings () ? q->settings () : local.data () );

	if ( !q->insecureFallback () )
	{
		q->emitFinishedWithError ( OtherError, tr ( "Could not open wallet: %1; %2" )
								  .arg ( QDBusError::errorString ( err.type() ), err.message () ) );
		return;
	}

	actual->remove( key );
	actual->sync ();
	q->emitFinished ();
	q->emitFinished ();
}
