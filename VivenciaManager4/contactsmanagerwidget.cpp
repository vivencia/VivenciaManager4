#include "contactsmanagerwidget.h"
#include "system_init.h"

#include <vmWidgets/vmwidgets.h>
#include <vmUtils/fileops.h>
#include <vmNotify/vmnotify.h>

#include <QtWidgets/QToolButton>
#include <QtWidgets/QHBoxLayout>

const uint CMW_N_TYPES ( 2 );

static const QString BTN_ADD_TOOLTIP_CHECKED[CMW_N_TYPES] = {
	APP_TR_FUNC ( "Click again to save phone number" ), APP_TR_FUNC ( "Click again to save the email address" )
};

static const QString BTN_ADD_TOOLTIP_UNCHECKED[CMW_N_TYPES] = {
	APP_TR_FUNC ( "Insert a new phone number" ), APP_TR_FUNC ( "Insert a new email address" )
};

static const QString BTN_DEL_TOOLTIP[CMW_N_TYPES] = {
	APP_TR_FUNC ( "Remove selected telephone number" ), APP_TR_FUNC ( "Remove selected address" )
};

static const QString BTN_CANCEL_TOOLTIP[CMW_N_TYPES] = {
	APP_TR_FUNC ( "Cancel insertion of telephone number" ), APP_TR_FUNC ( "Cancel insertion of email or site address" )
};

contactsManagerWidget::contactsManagerWidget ( QWidget* parent, const CMW_TYPE type )
	: QFrame ( parent ), vmWidget ( WT_COMBO ), cboInfoData ( nullptr ), btnExtra ( nullptr ),
	  m_contact_type ( type ), insertFunc ( nullptr ), removeFunc ( nullptr ) {}

void contactsManagerWidget::initInterface ()
{
	if ( cboInfoData != nullptr )
		return;

	cboInfoData = new vmComboBox ( this );
	cboInfoData->setMinimumWidth ( 200 );
	if ( m_contact_type == CMW_PHONES )
		cboInfoData->setTextType ( vmWidget::TT_PHONE );
	cboInfoData->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) { return cbo_textAltered ( sender->text () ); });
	cboInfoData->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const sender ) { return keyPressedSelector ( ke, sender ); } );
	cboInfoData->setCallbackForEnterKeyPressed ( [&] () { return btnAdd_clicked ( false ); } );
	cboInfoData->setCallbackForEscKeyPressed ( [&] () { return btnDel_clicked (); } );

	btnAdd = new QToolButton ( this );
	btnAdd->setCheckable ( true );
	btnAdd->setIcon ( ICON ( "browse-controls/add.png" ) );
	connect ( btnAdd, static_cast<void (QToolButton::*)( bool )>( &QToolButton::clicked ), this, [&] ( const bool checked ) {
		return btnAdd_clicked ( checked ); } );
	btnDel = new QToolButton ( this );
	btnDel->setIcon ( ICON ( "browse-controls/remove.png" ) );
	connect ( btnDel, &QToolButton::clicked, this, [&] () { return btnDel_clicked (); } );

	QHBoxLayout* mainLayout ( new QHBoxLayout );
	mainLayout->setMargin ( 0 );
	mainLayout->setSpacing ( 1 );
	mainLayout->addWidget ( cboInfoData, 1 );
	mainLayout->addWidget ( btnAdd );
	mainLayout->addWidget ( btnDel );

	if ( m_contact_type == CMW_EMAIL )
	{
		btnExtra = new QToolButton ( this );
		btnExtra->setIcon ( ICON ( "email" ) );
		connect ( btnExtra, &QToolButton::clicked, this, [&] () {
			return btnExtra_clicked (); } );
		btnExtra->setEnabled ( false );
		mainLayout->addWidget ( btnExtra );
	}
	setLayout ( mainLayout );
}

void contactsManagerWidget::setEditable ( const bool editable )
{
	if ( !editable )
	{
		btnAdd->setChecked ( false );
		btnAdd->setToolTip ( BTN_ADD_TOOLTIP_CHECKED[static_cast<uint> ( m_contact_type )] );
		btnDel->setToolTip ( BTN_DEL_TOOLTIP[static_cast<uint> ( m_contact_type )] );
	}
	else
	{
		btnAdd->setChecked ( cboInfoData->currentText ().isEmpty () ? false : cboInfoData->findText ( cboInfoData->currentText () ) == -1 );
		btnAdd->setToolTip ( BTN_ADD_TOOLTIP_UNCHECKED[static_cast<uint> ( m_contact_type )] );
		btnDel->setToolTip ( btnAdd->isChecked () ? BTN_CANCEL_TOOLTIP[static_cast<uint> ( m_contact_type )] : BTN_DEL_TOOLTIP[static_cast<uint> ( m_contact_type )] );
	}
	btnAdd->setEnabled ( editable );
	cboInfoData->setEditable ( btnAdd->isChecked () );
	btnDel->setEnabled ( editable );
	vmWidget::setEditable ( editable );
}

void contactsManagerWidget::cbo_textAltered ( const QString& text )
{
	//QString new_text;
	/*if ( m_contact_type == CMW_PHONES )
	{
		const vmNumber phone ( cboInfoData->text (), VMNT_PHONE );
		input_ok = phone.isPhone ();
		if ( input_ok )
			new_text = phone.toPhone ();
	}
	else
	{*/
	if ( m_contact_type == CMW_EMAIL )
	{
		if ( isEmailAddress ( text ) )
			cboInfoData->setText ( text.toLower () );
			//new_text = text.toLower ();
	}
	//if ( input_ok )
	//	cboInfoData->setEditText ( new_text );
}

void contactsManagerWidget::keyPressedSelector ( const QKeyEvent* const ke, const vmWidget* const )
{
	if ( keypressed_func )
		keypressed_func ( ke, this );
}

void contactsManagerWidget::btnAdd_clicked ( const bool checked )
{
	cboInfoData->setEditable ( checked );
	if ( checked )
	{
		cboInfoData->setText ( emptyString );
		cboInfoData->setFocus ();
		btnAdd->setToolTip ( BTN_ADD_TOOLTIP_CHECKED[static_cast<uint> ( m_contact_type )] );
		btnDel->setToolTip ( BTN_CANCEL_TOOLTIP[static_cast<uint> ( m_contact_type )] );
		btnDel->setEnabled ( true );
	}
	else
	{
		//cboInfoData->editor ()->updateText ();
		bool input_ok ( false );
		if ( m_contact_type == CMW_PHONES )
		{
			cboInfoData->editor ()->updateText ( false );
			input_ok = true;
		}
		else
		{
			input_ok = isEmailAddress ( cboInfoData->text () );
			if ( input_ok )
				cboInfoData->setText ( cboInfoData->text () );
		}
		if ( input_ok )
		{
			btnDel->setToolTip ( BTN_DEL_TOOLTIP[static_cast<uint> ( m_contact_type )] );
			btnDel->setEnabled ( true );
			btnAdd->setToolTip ( BTN_ADD_TOOLTIP_UNCHECKED[static_cast<uint> ( m_contact_type )] );
			insertItem ();
			if ( insertFunc )
				insertFunc ( cboInfoData->text (), this );
		}
		else
		{
			NOTIFY ()->notifyMessage ( TR_FUNC ( "Error" ), cboInfoData->text () + TR_FUNC ( "is not a valid " ) +
										  ( m_contact_type == CMW_PHONES ? TR_FUNC ( "phone number." ) : TR_FUNC ( "email or site address." ) ) +
										  TR_FUNC ( "\nCannot add it." ) );
		}
	}
	btnAdd->setChecked ( checked ); // for indirect calls, i.e. enter or esc keys pressed when within combo's line edit
}

void contactsManagerWidget::btnDel_clicked ()
{
	if ( cboInfoData->vmWidget::isEditable () )
	{ // now adding, then do cancel operation
		cboInfoData->setEditable ( false );
		cboInfoData->setCurrentIndex ( 0 );
		btnAdd->setChecked ( false );
		btnAdd->setToolTip ( BTN_ADD_TOOLTIP_UNCHECKED[static_cast<uint> ( m_contact_type )] );
		btnDel->setToolTip ( BTN_DEL_TOOLTIP[static_cast<uint> ( m_contact_type )] );
	}
	else
	{ // now viewing, then do del operation
		int removed_idx ( -1 );
		const bool ok ( removeCurrent ( removed_idx ) );
		btnDel->setEnabled ( cboInfoData->count () > 0 );
		if ( ok && removeFunc )
			removeFunc ( removed_idx, this );
	}
}

void contactsManagerWidget::btnExtra_clicked ()
{
	switch ( m_contact_type )
	{
		case CMW_PHONES:
		break;
		case CMW_EMAIL:
			fileOps::openAddress ( cboInfoData->currentText () );
		break;
	}
}

void contactsManagerWidget::decodePhones ( const stringRecord& phones, const bool bClear )
{
	if ( bClear )
	{
		clearAll ();
	}
	if ( !phones.isNull () )
	{
		phones.first ();
		vmNumber phone ( phones.curValue (), VMNT_PHONE, 1 );
		cboInfoData->setIgnoreChanges ( true );
		cboInfoData->setText ( phone.toPhone () );
		do
		{
			cboInfoData->addItem ( phone.toPhone () );
			if ( phones.next () )
				phone.fromTrustedStrPhone ( phones.curValue () );
			else
				break;
		} while ( true );
		cboInfoData->setIgnoreChanges ( false );
	}
}

void contactsManagerWidget::decodeEmails ( const stringRecord& emails, const bool bClear )
{
	if ( bClear )
	{
		clearAll ();
	}
	if ( emails.first () )
	{
		cboInfoData->setIgnoreChanges ( true );
		cboInfoData->setText ( emails.curValue () );
		do
		{
			cboInfoData->addItem ( emails.curValue () );
		} while ( emails.next () );
		cboInfoData->setIgnoreChanges ( false );
	}
	btnExtra->setEnabled ( cboInfoData->count () > 0 );
}

void contactsManagerWidget::insertItem ()
{
	cboInfoData->setIgnoreChanges ( true );
	cboInfoData->setEditable ( false );
	btnAdd->setEnabled ( true );
	cboInfoData->addItem ( cboInfoData->text () );
	if ( btnExtra )
		btnExtra->setEnabled ( true );
	cboInfoData->setIgnoreChanges ( false );
}

bool contactsManagerWidget::removeCurrent ( int& removed_idx )
{
	removed_idx = cboInfoData->currentIndex ();
	if ( removed_idx >= 0 )
	{
		cboInfoData->setIgnoreChanges ( true );
		cboInfoData->removeItem ( removed_idx );
		if ( btnExtra )
			btnExtra->setEnabled ( cboInfoData->count () > 0 );
		cboInfoData->setIgnoreChanges ( false );
	}
	return ( removed_idx >= 0 );
}


void contactsManagerWidget::clearAll ()
{
	cboInfoData->setIgnoreChanges ( true );
	cboInfoData->clear ();
	cboInfoData->setText ( emptyString );
	if ( btnExtra )
		btnExtra->setEnabled ( false );
	cboInfoData->setIgnoreChanges ( false );
}
