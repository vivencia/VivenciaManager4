/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -m -p kwallet_interface /home/guilherme/Software/qtkeychain/org.kde.KWallet.xml
 *
 * qdbusxml2cpp is Copyright (C) 2016 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#include "kwallet_interface.h"

/*
 * Implementation of interface class OrgKdeKWalletInterface
 */

OrgKdeKWalletInterface::OrgKdeKWalletInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
	: QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

#include "moc_kwallet_interface.cpp"
