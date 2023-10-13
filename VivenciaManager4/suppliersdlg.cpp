#include "suppliersdlg.h"
#include "mainwindow.h"
#include "searchui.h"
#include "system_init.h"
#include "contactsmanagerwidget.h"

#include <heapmanager.h>
#include <dbRecords/supplierrecord.h>
#include <dbRecords/completers.h>
#include <dbRecords/dblistitem.h>
#include <dbRecords/vivenciadb.h>
#include <vmTaskPanel/vmtaskpanel.h>
#include <vmTaskPanel/vmactiongroup.h>
#include <vmDocumentEditor/documenteditor.h>
#include <vmDocumentEditor/texteditor.h>

#include <QtGui/QKeyEvent>
#include <QtSql/QSqlQuery>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>

suppliersDlg::suppliersDlg ()
	: QDialog ( nullptr ), supRec ( new supplierRecord ( true ) ),
	  widgetList ( SUPPLIER_FIELD_COUNT + 1 )
{
	setWindowTitle ( TR_FUNC ( "Suppliers" ) );
	setWindowFlags ( Qt::Tool | Qt::WindowStaysOnTopHint );
	setupUI ();
}

suppliersDlg::~suppliersDlg ()
{
	heap_del ( supRec );
}

void suppliersDlg::saveWidget ( vmWidget* widget, const int id )
{
	widget->setID ( id );
	widgetList[id] = widget;
}

void suppliersDlg::setupUI ()
{
	txtSupName = new vmLineEdit;
	saveWidget ( txtSupName, FLD_SUPPLIER_NAME );
	txtSupName->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtSupplier_textAltered ( sender ); } );
	txtSupName->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
		return keyPressedSelector ( ke ); } );

	txtSupID = new vmLineEdit;
	saveWidget ( txtSupID, FLD_SUPPLIER_ID );
	txtSupID->setEditable ( false );

	auto hLayout1 ( new QHBoxLayout );
	hLayout1->addWidget ( new QLabel ( TR_FUNC ( "Name: " ) ) );
	hLayout1->addWidget ( txtSupName, 1 );
	hLayout1->addWidget ( new QLabel ( QStringLiteral ( "ID" ) ) );
	hLayout1->addWidget ( txtSupID );

	txtSupStreet = new vmLineEdit;
	saveWidget ( txtSupStreet, FLD_SUPPLIER_STREET );
	txtSupStreet->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtSupplier_textAltered ( sender ); } );
	txtSupStreet->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
		return keyPressedSelector ( ke ); } );
	COMPLETERS ()->setCompleterForWidget ( txtSupStreet, CC_ADDRESS );

	txtSupNbr = new vmLineEdit;
	saveWidget ( txtSupNbr, FLD_SUPPLIER_NUMBER );
	txtSupNbr->setTextType ( vmWidget::TT_INTEGER );
	txtSupNbr->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtSupplier_textAltered ( sender ); } );
	txtSupNbr->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
		return keyPressedSelector ( ke ); } );

	auto hLayout2 ( new QHBoxLayout );
	hLayout2->addWidget ( new QLabel ( TR_FUNC ( "Address: " ) ), 0 );
	hLayout2->addWidget ( txtSupStreet, 1 );
	hLayout2->addWidget ( new QLabel ( QStringLiteral ( "#: " ) ), 0 );
	hLayout2->addWidget ( txtSupNbr, 0 );

	txtSupDistrict = new vmLineEdit;
	saveWidget ( txtSupDistrict, FLD_SUPPLIER_DISTRICT );
	txtSupDistrict->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtSupplier_textAltered ( sender ); } );
	txtSupDistrict->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
		return keyPressedSelector ( ke ); } );
	COMPLETERS ()->setCompleterForWidget ( txtSupDistrict, CC_ADDRESS );

	txtSupCity = new vmLineEdit;
	saveWidget ( txtSupCity, FLD_SUPPLIER_CITY );
	txtSupCity->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
		return txtSupplier_textAltered ( sender ); } );
	txtSupCity->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
		return keyPressedSelector ( ke ); } );
	COMPLETERS ()->setCompleterForWidget ( txtSupCity, CC_ADDRESS );

	auto hLayout3 ( new QHBoxLayout );
	hLayout3->addWidget ( new QLabel ( TR_FUNC ( "District: " ) ), 0 );
	hLayout3->addWidget ( txtSupDistrict, 1 );
	hLayout3->addWidget ( new QLabel ( TR_FUNC ( "City: " ) ), 0 );
	hLayout3->addWidget ( txtSupCity, 1 );

	contactsPhones = new contactsManagerWidget ( nullptr );
	saveWidget ( contactsPhones, FLD_SUPPLIER_PHONES );
	contactsPhones->setContactType ( contactsManagerWidget::CMW_PHONES );
	contactsPhones->initInterface ();
	contactsPhones->setCallbackForInsertion ( [&] ( const QString& phone, const vmWidget* const sender ) {
		return contactsAdd ( phone, sender ); } );
	contactsPhones->setCallbackForRemoval ( [&] ( const int idx, const vmWidget* const sender ) {
		return contactsDel ( idx, sender ); } );
	auto lblPhone ( new QLabel ( TR_FUNC ( "Phones: " ) ) );
	lblPhone->setBuddy ( contactsPhones );

	auto vline ( new QFrame );
	vline->setFrameStyle ( QFrame::VLine|QFrame::Raised );

	contactsEmails = new contactsManagerWidget ( nullptr );
	saveWidget ( contactsEmails, FLD_SUPPLIER_EMAIL );
	contactsEmails->setContactType ( contactsManagerWidget::CMW_EMAIL );
	contactsEmails->initInterface ();
	contactsEmails->setCallbackForInsertion ( [&] ( const QString& addrs, const vmWidget* const sender ) {
		return contactsAdd ( addrs, sender ); } );
	contactsEmails->setCallbackForRemoval ( [&] ( const int idx, const vmWidget* const sender ) {
		return contactsDel ( idx, sender ); } );
	auto lblEMail ( new QLabel ( TR_FUNC ( "EMail/Site: " ) ) );
	lblEMail->setBuddy ( contactsEmails );

	auto hLayout4 ( new QHBoxLayout );
	hLayout4->addWidget ( lblPhone, 0 );
	hLayout4->addWidget ( contactsPhones, 1 );
	hLayout4->addWidget ( vline, 0 );
	hLayout4->addWidget ( lblEMail, 0 );
	hLayout4->addWidget ( contactsEmails, 1 );

	btnInsert = new QPushButton;
	btnInsert->setCheckable ( true );
	connect ( btnInsert, static_cast<void (QPushButton::*)( bool )>( &QPushButton::clicked ), this, [&] ( const bool checked ) {
		return btnInsertClicked ( checked ); } );

	btnEdit = new QPushButton;
	btnEdit->setCheckable ( true );
	connect ( btnEdit, static_cast<void (QPushButton::*)( bool )>( &QPushButton::clicked ), this, [&] ( const bool checked ) {
		return btnEditClicked ( checked ); } );

	btnCancel = new QPushButton ( ICON ( "cancel" ), TR_FUNC ( "Cancel" ) );
	connect ( btnCancel, &QPushButton::clicked, this, [&] () { return btnCancelClicked (); } );

	btnRemove = new QPushButton ( ICON ( "browse-controls/remove" ), TR_FUNC ( "Remove" ) );
	connect ( btnRemove, &QPushButton::clicked, this, [&] () { return btnRemoveClicked (); } );

	btnCopyToEditor = new QPushButton ( TR_FUNC ( "Open in editor" ) );
	connect ( btnCopyToEditor, &QPushButton::clicked, this, [&] () { return btnCopyToEditorClicked (); } );

	btnCopyAllToEditor = new QPushButton ( TR_FUNC ( "Open all in editor" ) );
	btnCopyToEditor = new QPushButton ( TR_FUNC ( "Open in editor" ) );
	connect ( btnCopyAllToEditor, &QPushButton::clicked, this, [&] () { return btnCopyAllToEditorClicked (); } );

	auto hLayout5 ( new QHBoxLayout );
	hLayout5->addWidget ( btnInsert, 1 );
	hLayout5->addWidget ( btnEdit, 1 );
	hLayout5->addWidget ( btnCancel, 1 );
	hLayout5->addWidget ( btnRemove, 1 );
	hLayout5->addWidget ( btnCopyToEditor, 1 );
	hLayout5->addWidget ( btnCopyAllToEditor, 1 );

	vmTaskPanel* panel ( new vmTaskPanel ( emptyString, this ) );
	auto mainLayout ( new QVBoxLayout );
	mainLayout->setContentsMargins ( 0, 0, 0, 0 );
	mainLayout->setSpacing ( 0 );
	mainLayout->addWidget ( panel, 1 );
	setLayout ( mainLayout );
	vmActionGroup* group ( panel->createGroup ( emptyString, false, true, false ) );
	group->addLayout ( hLayout1 );
	group->addLayout ( hLayout2 );
	group->addLayout ( hLayout3 );
	group->addLayout ( hLayout4 );
	group->addLayout ( hLayout5 );
	MAINWINDOW ()->appMainStyle ( panel );

	adjustSize ();
}

void suppliersDlg::retrieveInfo ( const QString& name )
{
	if ( supRec->readRecord ( FLD_SUPPLIER_NAME, name ) )
	{
		txtSupID->setText ( recStrValue ( supRec, FLD_SUPPLIER_ID ) );
		txtSupName->setText ( recStrValue ( supRec, FLD_SUPPLIER_NAME ) );
		txtSupStreet->setText ( recStrValue ( supRec, FLD_SUPPLIER_STREET ) );
		txtSupNbr->setText ( recStrValue ( supRec, FLD_SUPPLIER_NUMBER ) );
		txtSupDistrict->setText ( recStrValue ( supRec, FLD_SUPPLIER_DISTRICT ) );
		txtSupCity->setText ( recStrValue ( supRec, FLD_SUPPLIER_CITY ) );
		contactsPhones->decodePhones ( recStrValue ( supRec, FLD_SUPPLIER_PHONES ) );
		contactsEmails->decodeEmails ( recStrValue ( supRec, FLD_SUPPLIER_EMAIL ) );
	}
	else
	{
		supRec->clearAll ();
		clearForms ();
		txtSupName->setText ( name );
		txtSupplier_textAltered ( txtSupName );
	}
	controlForms ();
}

void suppliersDlg::controlForms ()
{
	const bool editing_action ( supRec->action () != ACTION_READ );

	txtSupName->setEditable ( editing_action );
	txtSupStreet->setEditable ( editing_action );
	txtSupNbr->setEditable ( editing_action );
	txtSupDistrict->setEditable ( editing_action );
	txtSupCity->setEditable ( editing_action );
	contactsPhones->setEditable ( editing_action );
	contactsEmails->setEditable ( editing_action );

	btnInsert->setEnabled ( supRec->action () != ACTION_EDIT );
	btnInsert->setText ( supRec->action () != ACTION_ADD ? TR_FUNC ( "Add" ) : TR_FUNC ( "Save" ) );
	btnInsert->setIcon ( supRec->action () != ACTION_ADD ?  ICON ( "browse-controls/add" ) : ICON ( "document-save" ) );

	btnEdit->setEnabled ( supRec->action () != ACTION_ADD );
	btnEdit->setText ( supRec->action () != ACTION_EDIT ? TR_FUNC ( "Edit" ) : TR_FUNC ( "Save" ) );
	btnEdit->setIcon ( supRec->action () != ACTION_EDIT ?  ICON ( "browse-controls/edit" ) : ICON ( "document-save" ) );

	btnCancel->setEnabled ( editing_action );
	btnRemove->setEnabled ( !editing_action );
	btnCopyToEditor->setEnabled ( !editing_action );
	btnCopyAllToEditor->setEnabled ( !editing_action );

	if ( editing_action )
		txtSupName->setFocus ();
}

void suppliersDlg::keyPressedSelector ( const QKeyEvent* ke )
{
	btnInsert->setChecked ( false );
	btnEdit->setChecked ( false );
	if ( ke->key () == Qt::Key_Escape )
		btnCancelClicked ();
	else
	{
		btnCopyAllToEditor->setFocus (); // trigger any pending editing_Finished or textAltered event
		if ( supRec->action () == ACTION_ADD )
			btnInsertClicked ( false );
		else
			btnEditClicked ( false );
	}
}

void suppliersDlg::clearForms ()
{
	txtSupName->clear ();
	txtSupID->clear ();
	txtSupStreet->clear ();
	txtSupDistrict->clear ();
	txtSupNbr->clear ();
	txtSupCity->clear ();
	contactsPhones->clearAll ();
	contactsEmails->clearAll ();
}

void suppliersDlg::txtSupplier_textAltered ( const vmWidget* const sender )
{
	setRecValue ( supRec, static_cast<uint>(sender->id ()), sender->text () );
}

void suppliersDlg::contactsAdd ( const QString& info, const vmWidget* const sender )
{
	setRecValue ( supRec, static_cast<uint>( sender->id () ),
				  stringRecord::joinStringRecords ( recStrValue ( supRec, static_cast<uint>( sender->id () ) ), info ) );
}

void suppliersDlg::contactsDel ( const int idx, const vmWidget* const sender )
{
	if ( idx >= 0 )
	{
		stringRecord info_rec ( recStrValue ( supRec, static_cast<uint>(sender->id ()) ) );
		info_rec.removeField ( static_cast<uint>( idx ) );
		setRecValue ( supRec, static_cast<uint>( sender->id () ), info_rec.toString () );
	}
}

void suppliersDlg::displaySupplier ( const QString& supName, const bool b_showdlg )
{
	if ( supName != recStrValue ( supRec, FLD_SUPPLIER_NAME ) )
	{
		retrieveInfo ( supName );
	}
	if ( b_showdlg )
	{
		show ();
	}
}

void suppliersDlg::btnInsertClicked ( const bool checked )
{
	if ( checked )
	{
		// if we have a name in txtSupName but the name cannot be found in the database, we use that name as the
		// intended new name for the inserting record. Elsewise, we clean everything
		if ( supRec->readRecord ( FLD_SUPPLIER_NAME, txtSupName->text () ) )
		{
			clearForms ();
		}
		supRec->setAction ( ACTION_ADD );
	}
	else
	{
		supRec->saveRecord ();
		supRec->setAction ( ACTION_READ );
		txtSupID->setText ( recStrValue ( supRec, FLD_SUPPLIER_ID ) );
	}
	controlForms ();
}

void suppliersDlg::btnEditClicked ( const bool checked )
{
	if ( checked )
	{
		supRec->setAction ( ACTION_EDIT );
	}
	else
	{
		supRec->saveRecord ();
		supRec->setAction ( ACTION_READ );
	}
	controlForms ();
}

void suppliersDlg::btnCancelClicked ()
{
	supRec->setAction ( ACTION_REVERT );
	retrieveInfo ( recStrValue ( supRec, FLD_SUPPLIER_NAME ) );
}

void suppliersDlg::btnRemoveClicked ()
{
	supRec->deleteRecord ();
	clearForms ();
}

void suppliersDlg::btnCopyToEditorClicked ()
{
	QString info;
	supplierInfo ( txtSupName->text (), info );
	if ( !info.isEmpty () )
	{
		hideDialog ();
		auto editor ( EDITOR ()->startNewTextEditor () );
		editor->displayText ( info );
		EDITOR ()->show ();
	}
}

void suppliersDlg::btnCopyAllToEditorClicked ()
{
	QString info;
	supplierInfo ( emptyString, info );
	if ( !info.isEmpty () )
	{
		hideDialog ();
		auto editor ( EDITOR ()->startNewTextEditor () );
		editor->displayText ( info );
		EDITOR ()->show ();
	}
}

void suppliersDlg::hideDialog ()
{
	if ( supRec->action () != ACTION_READ )
	{
		return;
	}
	hide ();
}

void suppliersDlg::supplierInfo ( const QString& name, QString& info )
{
	static const QString template_str ( QStringLiteral ( "%1%9%2, No.: %3 - %4 - %5%9Fone(s): %6 %7%9site/e=mail: %8%9%9" ) );

	QString cmd ( QStringLiteral ( "SELECT FROM %1 WHERE %2=\"%3\"" ) );
	cmd.replace ( QStringLiteral ( "%1" ), supplierRecord::t_info.table_name );

	if ( !name.isEmpty () )
	{
		cmd.replace ( QStringLiteral ( "%2" ), VivenciaDB::getTableColumnName ( &supplierRecord::t_info, FLD_SUPPLIER_NAME ) );
		cmd.replace ( QStringLiteral ( "%3" ), name );
	}
	else
	{
		cmd.truncate ( cmd.indexOf ( QStringLiteral ( "WHERE" ), 16 ) );
	}

	QSqlQuery query ( cmd, *( VDB ()->database () ) );
	query.setForwardOnly ( true );
	query.exec ( cmd );
	if ( query.first () )
	{
		do
		{
			info += template_str.arg (
						query.value ( FLD_SUPPLIER_NAME ).toString (),
						query.value ( FLD_SUPPLIER_STREET ).toString (),
						query.value ( FLD_SUPPLIER_NUMBER ).toString (),
						query.value ( FLD_SUPPLIER_DISTRICT ).toString (),
						query.value ( FLD_SUPPLIER_CITY ).toString (),
						query.value ( FLD_SUPPLIER_PHONES ).toString (),
						query.value ( FLD_SUPPLIER_EMAIL ).toString (),
						QString ( CHR_NEWLINE ) );
		} while ( query.next () );
	}
}
//--------------------------------------SLOTS---------------------------------------------
