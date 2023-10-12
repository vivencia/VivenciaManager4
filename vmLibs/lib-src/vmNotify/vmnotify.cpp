#include "vmnotify.h"
#include "heapmanager.h"

#include <vmWidgets/vmwidgets.h>

#include <vmTaskPanel/vmactiongroup.h>
#include <vmTaskPanel/vmtaskpanel.h>
#include <VivenciaManager4/system_init.h>

#include <QtCore/QTimer>
#include <QtCore/QRect>
#include <QtCore/QPointer>
#include <QtCore/QEvent>
#include <QtCore/QPoint>
#include <QtGui/QPixmap>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QHBoxLayout>

extern "C"
{
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
}

// X11/X.h defines a KeyPress constant which conflicts with QEvent's.
// But we don't need X's definition, so we discard it
#undef KeyPress

const char* const PROPERTY_BUTTON_ID ( "pbid" );
const char* const PROPERTY_BUTTON_RESULT ( "pbr" );

const uint FADE_TIMER_TIMEOUT ( 20 );
static const QString urgency_tag[3] = { QStringLiteral ( "<b>" ), QStringLiteral ( "<b><u>" ), QStringLiteral ( "<b><u><font color=red>" ) };

Message::Message ( vmNotify* parent )
	: timeout ( -1 ), isModal ( false ), mbClosable ( true ), mbAutoRemove ( true ), widgets ( 5 ),
	  m_parent ( parent ), mBtnID ( -1 ), icon ( nullptr ), mGroup ( nullptr ), timer ( nullptr ),
	  messageFinishedFunc ( nullptr ), buttonClickedfunc ( nullptr )
{}

Message::~Message ()
{
	if ( m_parent != nullptr )
		m_parent->mPanel->removeGroup ( mGroup, true );
	heap_del ( timer );
}

void Message::addWidget ( QWidget* widget, const uint row, const Qt::Alignment alignment, const bool is_button )
{
	auto st ( new st_widgets );
	st->widget = widget;
	st->row = row;
	st->alignment = alignment;
	st->isButton = is_button;
	if ( row != 0 )
		widgets.append ( st );
	else
		widgets.prepend ( st );
}

void Message::inputFormKeyPressed ( const QKeyEvent* ke )
{
	mBtnID = ke->key () == Qt::Key_Escape ? MESSAGE_BTN_CANCEL : MESSAGE_BTN_OK;
	if ( messageFinishedFunc )
		messageFinishedFunc ( this );
}

vmNotify::vmNotify ( const QString& position, QWidget* const parent )
	: QDialog ( nullptr, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint ),
	  mbDeleteWhenStackIsEmpty ( false ), mbButtonClicked ( false ), mPanel ( new vmTaskPanel ( emptyString, this ) ),
	  fadeTimer ( new QTimer ), m_parent ( parent ), mPos ( position ), mEventLoop ( nullptr ), messageStack ( 5 )
{
	auto mainLayout ( new QHBoxLayout );
	mainLayout->setContentsMargins ( 0, 0, 0, 0 );
	mainLayout->setSpacing ( 0 );
	mainLayout->addWidget ( mPanel, 1 );
	setLayout ( mainLayout );
	connect ( fadeTimer, &QTimer::timeout, this, [&] () { return fadeWidget (); } );
}

vmNotify::~vmNotify ()
{
	messageStack.clear ( true );
}

void vmNotify::enterEventLoop ()
{
	setWindowModality ( Qt::ApplicationModal );
	show ();
	activateWindow ();
	QPointer<QDialog> guard = this;
	QEventLoop eventLoop;
	mEventLoop = &eventLoop;
	QTimer::singleShot ( 0, this, [&] () { mEventLoop->processEvents ( QEventLoop::AllEvents );
		return mEventLoop->exit ( 0 ); } );
	(void) eventLoop.exec ( QEventLoop::DialogExec );
	if ( guard.isNull () )
		return;

	mEventLoop = nullptr;
	setAttribute ( Qt::WA_ShowModal, false );
	setAttribute( Qt::WA_SetWindowModality, false );
	setWindowModality ( Qt::NonModal );
}

void vmNotify::addMessage ( Message* message )
{
	setupWidgets ( message );
	messageStack.append ( message );
	if ( MAINWINDOW () != nullptr )
		MAINWINDOW ()->appMainStyle ( mPanel );
	adjustSizeAndPosition ();
	setWindowOpacity ( 0.9 );

	startMessageTimer ( message );
	if ( message->isModal )
	{
		if ( messageStack.count () == 1  ) // only the one message, we can change the window modality
		{
			setWindowModality ( Qt::ApplicationModal );
			exec ();
		}
	}
	else
		show ();
}

void vmNotify::removeMessage ( Message* message )
{
	if ( message != nullptr )
	{
		/* A modal notification will block the main thread and only return when a button is explicitly clicked (specially because we
		   prevent the dialog to close based on key events by overriding reject() and accept () slots ). But when the notification
		   is non modal (i.e. time based) the calling thread will regain control before we have a result for the dialog. The callback method
		   is the only way to get the resulting button clicked in those instances. But we recommend to use the callback function everytime
		   for the sake of consistency */
		if ( message->buttonClickedfunc )
			message->buttonClickedfunc ( message->mBtnID );

		messageStack.removeOne ( message, 0, message->mbAutoRemove );
		if ( messageStack.isEmpty () )
		{
			fadeTimer->start ( FADE_TIMER_TIMEOUT );
			if ( mbDeleteWhenStackIsEmpty )
				this->deleteLater ();
		}

		adjustSizeAndPosition ();
		if ( mEventLoop )
			mEventLoop->exit ();
	}
}

//Ignore key presses, namely ENTER and ESC keys
void vmNotify::accept ()
{
	if ( mbButtonClicked )
		QDialog::accept ();
}

void vmNotify::reject ()
{
	if ( mbButtonClicked )
		QDialog::reject ();
}

void vmNotify::buttonClicked ( QPushButton* btn, Message* const message )
{
	mbButtonClicked = true;
	int result ( QDialog::Rejected );
	if ( btn != nullptr )
	{
		message->mBtnID = btn->property ( PROPERTY_BUTTON_ID ).toInt ();
		result = btn->property ( PROPERTY_BUTTON_RESULT ).toInt ();
	}
	else
		result = message->mBtnID;

	if ( message->isModal )
	{
		if ( result == QDialog::Accepted )
			accept ();
		else
			reject ();
	}
	else
		setResult ( result );
	message->mbAutoRemove = true; //Now that a button was clicked, remove the message from the stack

	if ( windowModality () != Qt::NonModal && messageStack.count () == 1 )
		setWindowModality ( Qt::NonModal );
	removeMessage ( message );
	mbButtonClicked = false;
}

void vmNotify::setupWidgets ( Message* const message )
{
	QHBoxLayout* rowLayout ( nullptr );
	uint row ( UINT_MAX );

	if ( !message->iconName.isEmpty () )
		message->icon = new QPixmap ( message->iconName );

	message->mGroup = mPanel->createGroup ( message->icon != nullptr ? QIcon ( *message->icon ) : QIcon (), message->title, false, false, message->mbClosable );
	message->mGroup->setCallbackForClosed ( [&, message] () { return buttonClicked ( nullptr, message ); } );
	if ( !message->bodyText.isEmpty () )
	{
		auto lblMessage ( new QLabel ( message->bodyText ) );
		lblMessage->setWordWrap ( true );
		lblMessage->setFrameStyle ( QFrame::StyledPanel | QFrame::Raised );
		lblMessage->setSizePolicy ( QSizePolicy::Minimum, QSizePolicy::Minimum );
		message->addWidget ( lblMessage , 0 );
	}

	QWidget* widget ( nullptr );
	for ( uint i ( 0 ); i < message->widgets.count (); ++i )
	{
		if ( row != message->widgets.at ( i )->row )
		{
			if ( rowLayout != nullptr )
				rowLayout->addStretch ( 1 ); // inserts a stretchable space at the end of the previous row
			rowLayout = new QHBoxLayout;
			rowLayout->setSpacing ( 2 );
			rowLayout->insertStretch ( 0, 1 );
			row = message->widgets.at ( i )->row;
		}
		widget = message->widgets.at ( i )->widget;
		message->mGroup->addQEntry ( widget, rowLayout );
		if ( message->widgets.at ( i )->isButton )
		{
			static_cast<void>(connect ( dynamic_cast<QPushButton*>( widget ), &QPushButton::clicked, this,
					  [&, widget, message] () { return buttonClicked ( dynamic_cast<QPushButton*>( widget ), message ); } ));
		}
	}
	if ( rowLayout != nullptr )
		rowLayout->addStretch ( 1 ); // inserts a stretchable space at the end of the last row
}

void vmNotify::startMessageTimer ( Message* const message )
{
	if ( message->timeout > 0 )
	{
		message->timer = new QTimer ();
		message->timer->setSingleShot ( true );
		connect ( message->timer, &QTimer::timeout, this, [&,message] () { return removeMessage ( message ); } );
		message->timer->start ( message->timeout );
	}
}


void vmNotify::fadeWidget ()
{
	if ( windowOpacity () > 0 && isVisible () )
	{
		setWindowOpacity ( windowOpacity () - 0.08 );
		fadeTimer->start ( FADE_TIMER_TIMEOUT );
	}
	else {
		hide ();
		fadeTimer->stop ();
	}
}

void vmNotify::adjustSizeAndPosition ()
{
	adjustSize ();

	QBitmap objBitmap ( size () );
	QPainter painter ( &objBitmap );
	painter.fillRect ( rect (), Qt::white );
	painter.setBrush ( QColor ( 0, 0, 0 ) );
	painter.drawRoundedRect ( rect (), 10, 10 );
	setMask ( objBitmap );

	const QPoint pos ( displayPosition ( size () ) );
	int x ( pos.x () - rect ().size ().width () - 10 );
	int y ( pos.y () - rect ().size ().height () - 10 );
	if ( x < 0 )
		x = 0;
	if ( y < 0 )
		y = 0;
	move ( x, y );
}

QPoint vmNotify::displayPosition ( const QSize& widgetSize )
{
	QRect parentGeometry;

	if ( m_parent == nullptr || m_parent == Sys_Init::mainwindow_instance )
	{
		parentGeometry = QGuiApplication::primaryScreen ()->availableGeometry ();
		if ( mPos.isEmpty () )
		{
			const QRect displayRect ( QGuiApplication::primaryScreen ()->geometry () );
			const QRect availableRect ( parentGeometry );

			if ( availableRect.height () < displayRect.height () )
			{
				mPos = QStringLiteral ( "BR" );
			}
			else
			{
				mPos = ( availableRect.x () > displayRect.x () ) ? QStringLiteral ( "LC" ) : QStringLiteral ( "BC" );
			}
		}
	}
	else
	{
		int x ( 0 ), y ( 0 );
		if ( m_parent->parent () == Sys_Init::mainwindow_instance )
		{ // mapTo: first argument must be a parent of caller object
			const QPoint mainwindowpos ( Sys_Init::mainwindow_instance->pos () );
			x = mainwindowpos.x () + m_parent->mapTo ( Sys_Init::mainwindow_instance, m_parent->pos () ).x () - m_parent->x ();
			y = mainwindowpos.y () + m_parent->mapTo ( Sys_Init::mainwindow_instance, m_parent->pos () ).y () - m_parent->y ();
		}
		else
		{
			x = m_parent->x ();
			y = m_parent->y ();
		}
		parentGeometry.setCoords ( x, y, x + width (), y + height () );
		if ( mPos.isEmpty () )
			mPos = CHR_ZERO;
	}

	QPoint p;
	if ( mPos == QStringLiteral ( "0" ) || mPos == QStringLiteral ( "BL" ) )
		p = parentGeometry.bottomLeft ();
	else if ( mPos == QStringLiteral ( "1" ) || mPos == QStringLiteral ( "BR" ) )
		p = parentGeometry.bottomRight ();
	else if ( mPos == QStringLiteral ( "2" ) || mPos == QStringLiteral ( "TR" ) )
		p = parentGeometry.topRight ();
	else if ( mPos == QStringLiteral ( "3" ) || mPos == QStringLiteral ( "TL" ) )
		p = parentGeometry.topLeft ();
	else if ( mPos == QStringLiteral ( "4" ) || mPos == QStringLiteral ( "C" ) )
	{
		p = parentGeometry.bottomRight ();
		p.setX ( p.x () / 2 + widgetSize.width () / 2 );
		p.setY ( p.y () / 2 + widgetSize.height () / 2 );
	}
	else if ( mPos == QStringLiteral ( "5" ) || mPos == QStringLiteral ( "RC" ) )
	{
		p = parentGeometry.bottomRight ();
		p.setY ( p.y () / 2 + widgetSize.height () / 2 );
	}
	else if ( mPos == QStringLiteral ( "6" ) || mPos == QStringLiteral ( "TC" ) )
	{
		p = parentGeometry.topRight ();
		p.setX ( p.x () / 2 + widgetSize.width () / 2 );
	}
	else if ( mPos == QStringLiteral ( "7" ) || mPos == QStringLiteral ( "LC" ) )
	{
		p = parentGeometry.bottomLeft ();
		p.setY ( p.y () / 2 + widgetSize.height () / 2 );
	}
	else if ( mPos == QStringLiteral ( "8" ) || mPos == QStringLiteral ( "BC" ) )
	{
		p = parentGeometry.bottomRight ();
		p.setX ( p.x () / 2 + widgetSize.width () / 2 );
	}
	else
		p = parentGeometry.bottomRight ();

	return p;
}

void vmNotify::notifyMessage ( const QString& title, const QString& msg, const int msecs, const bool b_critical )
{
	auto message ( new Message ( this ) );
	message->title = title;
	message->bodyText = msg;
	message->timeout = msecs;
	message->mbAutoRemove = true;
	message->iconName = QStringLiteral ( ":/resources/notify-" ) + (b_critical ? QStringLiteral( "3" ) : CHR_TWO);
	addMessage ( message );
}

void vmNotify::notifyMessage ( QWidget* referenceWidget, const QString& title, const QString& msg, const int msecs, const bool b_critical )
{
	vmNotify* newNotify ( new vmNotify ( referenceWidget != nullptr ? QStringLiteral ( "C" ) : QString (), referenceWidget ) );
	newNotify->mbDeleteWhenStackIsEmpty = true;
	newNotify->notifyMessage ( title, msg, msecs, b_critical );
}

int vmNotify::messageBox ( const QString& title, const QString& msg, const MESSAGE_BOX_ICON icon,
						  const QStringList& btnsText, const int m_sec, const std::function<void ( const int btn_idx )>& messageFinished_func )
{
	QStringList::const_iterator itr ( btnsText.constBegin () );
	const QStringList::const_iterator itr_end ( btnsText.constEnd () );

	if ( !static_cast<QString> (*itr ).isEmpty () )
	{
		QPushButton* btn ( nullptr );
		auto message ( new Message ( this ) );
		message->title = title;
		message->bodyText = msg;
		message->timeout = m_sec;
		message->isModal = (m_sec == -1);
		message->iconName = QLatin1String ( ":/resources/notify-" ) + QString::number ( icon ) + QLatin1String ( "" );
		message->setButtonClickedFunc ( messageFinished_func );
		uint i ( 0 );

		for ( ; itr != itr_end; ++itr, ++i )
		{
			btn = new QPushButton ( static_cast<QString> (*itr) );
			btn->setProperty ( PROPERTY_BUTTON_ID, i );
			//The first button is the default, any kind of OK button. The second button is any kind of Cancel, it rejects the dialog, equivalent of
			//presing ESC. All other buttons accept the dialog
			btn->setProperty ( PROPERTY_BUTTON_RESULT, i != 1 ? QDialog::Accepted : QDialog::Rejected );
			message->addWidget ( btn, 1, Qt::AlignCenter, true );
		}
		addMessage ( message ); // now we wait for control return to this event loop
		return message->mBtnID;
	}
	return -1;
}

int vmNotify::messageBox ( QWidget* const referenceWidget, const QString& title, const QString& msg,
					const MESSAGE_BOX_ICON icon, const  QStringList& btnsText, const int m_sec,
						   const std::function<void ( const int btn_idx )>& messageFinished_func )
{
	auto newNotify ( new vmNotify ( referenceWidget != nullptr ? QStringLiteral ( "C" ) : QString (), referenceWidget ) );
	newNotify->mbDeleteWhenStackIsEmpty = true;
	return newNotify->messageBox ( title, msg, icon, btnsText, m_sec, messageFinished_func );
}

bool vmNotify::questionBox ( const QString& title, const QString& msg, const int m_sec, const std::function<void ( const int btn_idx )>& messageFinished_func )
{
	const QStringList btnsText ( QStringList () << TR_FUNC ( "Yes" ) <<  TR_FUNC ( "No" ) );
	const bool bAnswer ( messageBox ( title, msg, vmNotify::QUESTION, btnsText, m_sec, messageFinished_func ) );
	return (bAnswer == MESSAGE_BTN_OK);
}

bool vmNotify::questionBox ( QWidget* const referenceWidget, const QString& title, const QString& msg, const int m_sec,
							 const std::function<void ( const int btn_idx )>& messageFinished_func )
{
	auto newNotify ( new vmNotify ( referenceWidget != nullptr ? QStringLiteral ( "C" ) : QString (), referenceWidget ) );
	newNotify->mbDeleteWhenStackIsEmpty = true;
	return newNotify->questionBox ( title, msg, m_sec, messageFinished_func );
}

int vmNotify::criticalBox ( const QString& title, const QString& msg, const int m_sec,
							const std::function<void ( const int btn_idx )>& messageFinished_func )
{
	const QStringList btnsText ( QStringList () << TR_FUNC ( "Yes" ) <<  TR_FUNC ( "No" ) << TR_FUNC ( "Retry" ) );
	return messageBox ( title, msg, vmNotify::CRITICAL, btnsText, m_sec, messageFinished_func );
}

int vmNotify::criticalBox ( QWidget* const referenceWidget, const QString& title, const QString& msg, const int m_sec,
							const std::function<void ( const int btn_idx )>& messageFinished_func )
{
	auto newNotify ( new vmNotify ( referenceWidget != nullptr ? QStringLiteral ( "C" ) : QString (), referenceWidget ) );
	newNotify->mbDeleteWhenStackIsEmpty = true;
	return newNotify->criticalBox ( title, msg, m_sec, messageFinished_func );
}

bool vmNotify::inputBox ( QString& result, QWidget* const referenceWidget, const QString& title, const QString& label_text,
						  const QString& initial_text, const QString& icon, QCompleter* completer )
{
	auto newNotify ( new vmNotify ( referenceWidget != nullptr ? QStringLiteral ( "C" ) : emptyString, referenceWidget ) );
	newNotify->mbDeleteWhenStackIsEmpty = true;
	newNotify->setWindowFlags ( Qt::FramelessWindowHint );
	uint row ( 1 );
	auto message ( new Message ( newNotify ) );
	message->title = title;
	message->bodyText = label_text;
	message->timeout = -1;
	message->isModal = true;
	message->iconName = icon;
	message->mbAutoRemove = false;

	auto inputForm ( new vmLineEdit );
	inputForm->setEditable ( true );
	inputForm->setCompleter ( completer );
	inputForm->setText ( initial_text );
	inputForm->setMaximumHeight ( 30 );
	inputForm->setMinimumWidth ( 200 );

	inputForm->setCallbackForRelevantKeyPressed ( [&, message] ( const QKeyEvent* ke, const vmWidget* const ) { return message->inputFormKeyPressed ( ke ); } );
	inputForm->setCallbackForContentsAltered ([&] ( const vmWidget* const form ) { result = dynamic_cast<vmLineEdit*>(const_cast<vmWidget*>(form))->text (); } );
	message->setMessageFinishedCallback ( [&newNotify ] ( Message* msg ) { return newNotify->buttonClicked ( nullptr, msg ); } );
	message->addWidget ( inputForm, row );

	auto btn0 ( new QPushButton ( QStringLiteral ( "OK" ) ) );
	btn0->setProperty ( PROPERTY_BUTTON_ID, MESSAGE_BTN_OK );
	btn0->setProperty ( PROPERTY_BUTTON_RESULT, QDialog::Accepted );
	message->addWidget ( btn0, ++row, Qt::AlignCenter, true );
	auto btn1 ( new QPushButton ( APP_TR_FUNC ( "Cancel" ) ) );
	btn1->setProperty ( PROPERTY_BUTTON_ID, MESSAGE_BTN_CANCEL );
	btn1->setProperty ( PROPERTY_BUTTON_RESULT, QDialog::Rejected );
	message->addWidget ( static_cast<QWidget*> ( btn1 ), row, Qt::AlignCenter, true );

	newNotify->addMessage ( message );
	return ( message->mBtnID == MESSAGE_BTN_OK );
}

bool vmNotify::passwordBox ( QString& result, QWidget* const referenceWidget, const QString& title,
							 const QString& label_text, const QString& icon )
{
	auto newNotify ( new vmNotify ( referenceWidget != nullptr ? QStringLiteral ( "C" ) : emptyString, referenceWidget ) );
	newNotify->mbDeleteWhenStackIsEmpty = true;
	newNotify->setWindowFlags ( Qt::FramelessWindowHint );
	uint row ( 1 );
	auto message ( new Message ( newNotify ) );
	message->title = title;
	message->bodyText = label_text;
	message->timeout = -1;
	message->isModal = true;
	message->iconName = icon;
	message->mbAutoRemove = false;

	auto inputForm ( new vmLineEdit );
	inputForm->setEditable ( true );
	inputForm->setMaximumHeight ( 30 );
	inputForm->setMinimumWidth ( 200 );
	inputForm->setEchoMode ( QLineEdit::Password );
	inputForm->setCallbackForRelevantKeyPressed ( [&, message] ( const QKeyEvent* ke, const vmWidget* const ) { return message->inputFormKeyPressed ( ke ); } );
	inputForm->setCallbackForContentsAltered ([&] ( const vmWidget* const form ) { result = dynamic_cast<vmLineEdit*>(const_cast<vmWidget*>(form))->text (); } );
	message->setMessageFinishedCallback ( [&newNotify ] ( Message* msg ) { return newNotify->buttonClicked ( nullptr, msg ); } );
	message->addWidget ( inputForm, row );

	auto btn0 ( new QPushButton ( QStringLiteral ( "OK" ) ) );
	btn0->setProperty ( PROPERTY_BUTTON_ID, MESSAGE_BTN_OK );
	btn0->setProperty ( PROPERTY_BUTTON_RESULT, QDialog::Accepted );
	message->addWidget ( btn0, ++row, Qt::AlignCenter, true );
	auto btn1 ( new QPushButton ( APP_TR_FUNC ( "Cancel" ) ) );
	btn1->setProperty ( PROPERTY_BUTTON_ID, MESSAGE_BTN_CANCEL );
	btn1->setProperty ( PROPERTY_BUTTON_RESULT, QDialog::Rejected );
	message->addWidget ( static_cast<QWidget*> ( btn1 ), row, Qt::AlignCenter, true );

	newNotify->addMessage ( message );

	return ( message->mBtnID == MESSAGE_BTN_OK );
}

bool vmNotify::logProgress ( const uint max_value, const uint value, const QString& label )
{
	Message* message ( messageStack.at ( 0 ) );
	if ( message )
	{
		auto labelWdg ( dynamic_cast<QLabel*>( message->widgets.at ( 0 )->widget ) );
		labelWdg->setText ( label );
		QProgressBar* pBar ( dynamic_cast<QProgressBar*>( message->widgets.at ( 1 )->widget ) );
		pBar->setValue ( static_cast<int>(value) );
		enterEventLoop ();

		if ( value >= max_value || message->mBtnID == MESSAGE_BTN_CANCEL )
		{
			removeMessage ( message );
			this->deleteLater ();
		}
		else
			return true;
	}
	return false;
}

vmNotify* vmNotify::progressBox ( QWidget* parent, const uint max_value, const QString& title )
{
	auto box ( new vmNotify ( QStringLiteral ( "C" ), parent ) );
	box->mbDeleteWhenStackIsEmpty = true;
	auto message ( new Message ( box ) );
	message->title = title;
	message->timeout = -1;
	message->bodyText = QStringLiteral ( "   " );
	message->isModal = false;
	message->mbAutoRemove = false;

	auto pBar ( new QProgressBar () );
	pBar->setTextVisible ( true );
	pBar->setMinimumWidth ( 200 );
	pBar->setMaximum ( static_cast<int>(max_value) );
	message->addWidget ( pBar, 1 );
	auto btnCancel ( new QPushButton ( APP_TR_FUNC ( "Cancel" ) ) );
	btnCancel->setProperty ( PROPERTY_BUTTON_ID, MESSAGE_BTN_CANCEL );
	btnCancel->setProperty ( PROPERTY_BUTTON_RESULT, QDialog::Rejected );
	//message->setMessageFinishedCallback ( [&] ( Message* msg ) { return buttonClicked ( nullptr, msg ); } );
	message->addWidget ( btnCancel, 2, Qt::AlignCenter, true );
	box->addMessage ( message );
	return box;
}
