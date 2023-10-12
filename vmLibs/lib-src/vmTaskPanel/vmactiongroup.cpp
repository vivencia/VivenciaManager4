#include "vmactiongroup.h"
#include "heapmanager.h"
#include "actionpanelscheme.h"

#include <vmWidgets/vmwidgets.h>

#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QApplication>

//--------------------------------------TASK-GROUP-----------------------------------------------
TaskGroup::TaskGroup ( QWidget *parent, const bool stretchContents )
	: QFrame ( parent ), mScheme ( nullptr ), mbStretchContents ( stretchContents )
{
	setProperty ( "class", QStringLiteral ( "content" ) );
	setProperty ( "header", QStringLiteral ( "true" ) );
	auto vbl ( new QVBoxLayout () );
	vbl->setContentsMargins ( 2, 2, 2, 2 );
	vbl->setSpacing ( 4 );
	setLayout ( vbl );
	setSizePolicy ( QSizePolicy::Minimum, QSizePolicy::Minimum );
}

void TaskGroup::setScheme ( ActionPanelScheme* scheme )
{
	if ( scheme )
	{
		mScheme = scheme;
		setStyleSheet ( mScheme->actionStyle );
		update ();
	}
}

void TaskGroup::addQEntry ( QWidget* widget, QLayout* l, const bool addStretch )
{
	if ( !widget )
		return;

	if ( !mbStretchContents )
		setMinimumHeight ( minimumHeight () + widget->minimumHeight () );

	if ( l )
	{
		if ( !l->property ( "tg_added" ).toBool () )
		{
			l->setProperty ( "tg_added", true  );
			groupLayout ()->addLayout ( l );
		}
		l->addWidget ( widget );
	}
	else
	{
		if ( addStretch )
		{
			auto hbl ( new QHBoxLayout () );
			hbl->setContentsMargins ( 0, 0, 0, 0 );
			hbl->setSpacing ( 0 );
			hbl->addWidget ( widget );
			groupLayout ()->addLayout ( hbl );
		}
		else
			groupLayout ()->addWidget ( widget, 1, Qt::AlignLeft );
	}
}

void TaskGroup::addLayout ( QLayout* layout )
{
	if ( !layout )
		return;

	if ( !mbStretchContents )
		setMinimumHeight ( minimumHeight () + layout->minimumSize ().height () );
	groupLayout ()->addLayout ( layout, 1 );
}

QPixmap TaskGroup::transparentRender ()
{
	QPixmap pm ( size () );
	pm.fill ( Qt::transparent );
	render ( &pm, QPoint ( 0, 0 ), rect (), DrawChildren | IgnoreMask );
	return pm;
}
//--------------------------------------TASK-GROUP-----------------------------------------------

//--------------------------------------TASK-HEADER-----------------------------------------------
TaskHeader::TaskHeader ( const QIcon& icon, const QString& title,
						 const bool expandable, const bool closable, QWidget *parent )
	: QFrame ( parent ), mTitle ( nullptr ), mExpandableButton ( nullptr ), mClosableButton ( nullptr ),
	  timerSlide ( nullptr ), mb_expandable ( expandable ), mb_closable ( closable ), mb_over ( false ),
	  mb_buttonOver ( false ), mb_fold ( true ), md_opacity ( 0.1 ),
	  funcFoldButtonClicked ( nullptr ), funcCloseButtonClicked ( nullptr )
{
	setProperty ( "class", QStringLiteral ( "header" ) );
	auto hbl ( new QHBoxLayout () );
	hbl->setContentsMargins ( 2, 2, 2, 2 );
	setLayout ( hbl );

	if ( !title.isEmpty () )
	{
		mTitle = new vmActionLabel ( this );
		mTitle->setProperty ( "class", QStringLiteral ( "header" ) );
		mTitle->setFontAttributes ( true, true );
		mTitle->setText ( title );
		mTitle->setIcon ( icon );
		mTitle->setSizePolicy ( QSizePolicy::Minimum, QSizePolicy::Preferred );
		hbl->addWidget ( mTitle );
	}

	setSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Maximum );
	setScheme ( ActionPanelScheme::defaultScheme () );
	setExpandable ( mb_expandable );
	setClosable ( mb_closable );
	setFrameStyle ( QFrame::StyledPanel | QFrame::Raised );
	installEventFilter ( this );
}

TaskHeader::~TaskHeader ()
{
	if ( timerSlide )
		delete timerSlide;
}

void TaskHeader::setExpandable ( const bool expandable )
{
	if ( expandable )
	{
		mb_expandable = true;
		if ( mExpandableButton )
			return;
		mExpandableButton = new vmActionLabel ( this );
		mExpandableButton->setFixedSize ( mScheme->headerButtonSize );
		layout ()->addWidget ( mExpandableButton );
		if ( mTitle )
			connect ( mTitle, static_cast<void (QToolButton::*)( const bool )>( &QToolButton::clicked ), this, [&] () { return fold (); } );
		connect ( mExpandableButton, static_cast<void (QToolButton::*)( const bool )>( &QToolButton::clicked ), this, [&] () { return fold (); } );
		changeIcons ();

		timerSlide = new QTimer ( this );
		connect ( timerSlide, &QTimer::timeout, this, [&] () { return animate (); } );
	}
	else
	{
		mb_expandable = false;
		if ( !mExpandableButton )
			return;
		mExpandableButton->setParent ( nullptr );
		delete mExpandableButton;
		mExpandableButton = nullptr;
		if ( mTitle )
			disconnect ( mTitle, nullptr, nullptr, nullptr );
		changeIcons ();
	}
}

void TaskHeader::setClosable ( const bool closable )
{
	if ( closable )
	{
		mb_closable = true;
		if ( mClosableButton )
			return;

		mClosableButton = new vmActionLabel ( this );
		mClosableButton->setFixedSize ( mScheme->headerButtonSize );
		layout ()->addWidget ( mClosableButton );
		connect ( mClosableButton, static_cast<void (QToolButton::*)( const bool )>( &QToolButton::clicked ), this,
				  [&] () { return close (); } );
		changeIcons ();
	}
	else
	{
		mb_closable = false;
		if ( !mClosableButton )
			return;

		mClosableButton->setParent ( nullptr );
		delete mClosableButton;
		mClosableButton = nullptr;
		changeIcons ();
	}
}

bool TaskHeader::eventFilter ( QObject* obj, QEvent* event )
{
	switch ( event->type () )
	{
		case QEvent::Enter:
			mb_buttonOver = true;
			changeIcons ();
		return true;

		case QEvent::Leave:
			mb_buttonOver = false;
			changeIcons ();
		return true;

		default:
		break;
	}
	return QFrame::eventFilter ( obj, event );
}

void TaskHeader::setScheme ( ActionPanelScheme* scheme )
{
	if ( scheme )
	{
		mScheme = scheme;
		setStyleSheet ( mScheme->actionStyle );

		if ( mb_expandable )
			changeIcons ();
		setFixedHeight ( scheme->headerSize );
		update ();
	}
}

void TaskHeader::setTitle ( const QString& new_title )
{
	mTitle->setText ( new_title );
}

void TaskHeader::setIcon ( const QIcon& icon )
{
	mTitle->setIcon ( icon );
}

void TaskHeader::paintEvent ( QPaintEvent* event )
{
	QPainter p ( this );

	if ( mScheme->headerAnimation )
		p.setOpacity ( md_opacity + 0.7 );

	QFrame::paintEvent ( event );
}

/*void TaskHeader::enterEvent ( QEvent* )
{
	qDebug () << "enter event";
	mb_over = true;
	if ( isEnabled () )
		timerSlide->start ( 100 );
	update ();
}

void TaskHeader::leaveEvent ( QEvent* )
{
	mb_over = false;
	if ( isEnabled () )
		timerSlide->start ( 100 );
	update ();
}*/

void TaskHeader::animate ()
{
	timerSlide->stop ();

	if ( !mScheme->headerAnimation )
		return;

	if ( !isEnabled () )
	{
		md_opacity = 0.1;
		update ();
		return;
	}

	if ( mb_over )
	{
		if ( md_opacity >= 0.3 )
		{
			md_opacity = 0.3;
			return;
		}
		md_opacity += 0.05;
	}
	else
	{
		if ( md_opacity <= 0.1 )
		{
			md_opacity = 0.1;
			return;
		}
		md_opacity = qMax ( 0.1, md_opacity - 0.05 );
	}

	timerSlide->start ();
	update ();
}

void TaskHeader::fold ()
{
	if ( funcFoldButtonClicked )
		funcFoldButtonClicked ();
	mb_fold = !mb_fold;
	changeIcons ();
}

void TaskHeader::close ()
{
	if ( mb_closable && funcCloseButtonClicked )
		funcCloseButtonClicked ();
}

void TaskHeader::changeIcons ()
{
	if ( mExpandableButton )
	{
		if ( mb_buttonOver )
			mExpandableButton->setIcon ( mb_fold ? mScheme->headerButtonFoldOver : mScheme->headerButtonUnfoldOver );
		else
			mExpandableButton->setIcon ( mb_fold ? mScheme->headerButtonFold : mScheme->headerButtonUnfold );
	}
	if ( mClosableButton )
		mClosableButton->setIcon ( mb_buttonOver ? mScheme->headerButtonCloseOver : mScheme->headerButtonClose );
}

void TaskHeader::keyPressEvent ( QKeyEvent* event )
{
	switch ( event->key () )
	{
		case Qt::Key_Down:
		{
			QKeyEvent ke ( QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier );
			QApplication::sendEvent ( this, &ke );
			return;
		}
		
		case Qt::Key_Up:
		{
			QKeyEvent ke ( QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier );
			QApplication::sendEvent ( this, &ke );
			return;
		}

		default:
		break;
	}
	QFrame::keyPressEvent ( event );
}

void TaskHeader::keyReleaseEvent ( QKeyEvent* event )
{
	switch ( event->key () )
	{
		case Qt::Key_Down:
		{
			QKeyEvent ke ( QEvent::Type::KeyRelease, Qt::Key_Tab, Qt::NoModifier );
			QApplication::sendEvent ( this, &ke );
			return;
		}

		case Qt::Key_Up:
		{
			QKeyEvent ke ( QEvent::KeyRelease, Qt::Key_Tab, Qt::ShiftModifier );
			QApplication::sendEvent ( this, &ke );
			return;
		}

		default:
		break;
	}
	QFrame::keyReleaseEvent ( event );
}
//--------------------------------------TASK-HEADER-----------------------------------------------

//--------------------------------------ACTION-GROUP-----------------------------------------------
vmActionGroup::vmActionGroup ( const QString& title, const bool expandable,
							   const bool stretchContents, const bool closable, QWidget* parent )
	: QWidget ( parent ), vmWidget ( WT_QWIDGET, WT_ACTION ),
	  mbStretchContents ( stretchContents ), timerShow ( nullptr ), timerHide ( nullptr )
{
	mHeader = new TaskHeader ( QIcon (), title, expandable, closable, this );
	init ();
}

vmActionGroup::vmActionGroup ( const QIcon& icon, const QString& title,
							   const bool expandable, const bool stretchContents,
							   const bool closable, QWidget* parent )
	: QWidget ( parent ), vmWidget ( WT_QWIDGET, WT_ACTION ),
	  mbStretchContents ( stretchContents ), timerShow ( nullptr ), timerHide ( nullptr )
{
	mHeader = new TaskHeader ( icon, title, expandable, closable, this );
	init ();
}

vmActionGroup::~vmActionGroup ()
{
	heap_del ( timerShow );
	heap_del ( timerHide );
	heap_del ( mDummy );
	heap_del ( mGroup );
	heap_del ( mHeader );
}

void vmActionGroup::init ()
{
	setWidgetPtr ( this );

	if ( mHeader->expandable () )
	{
		m_foldStep = 0;
		timerShow = new QTimer ( this );
		timerShow->setSingleShot ( true );
		connect ( timerShow, &QTimer::timeout, this, [&] () { return processShow (); } );
		timerHide = new QTimer ( this );
		timerHide->setSingleShot ( true );
		connect ( timerHide, &QTimer::timeout, this, [&] () { return processHide (); } );
	}
	if ( mHeader->closable () )
		mHeader->setCallbackForFoldButtonClicked ( [&] () { return showHide (); } );

	auto vbl ( new QVBoxLayout () );
	vbl->setContentsMargins ( 0, 0 ,0 ,0 );
	vbl->setSpacing ( 0 );
	setLayout ( vbl );

	mGroup = new TaskGroup ( this, mbStretchContents );
	mDummy = new QWidget ( this );
	mDummy->hide ();
	setScheme ( ActionPanelScheme::defaultScheme () );

	vbl->addWidget ( mHeader, 1 );
	vbl->addWidget ( mGroup, 1 );
	vbl->addWidget ( mDummy, 1 );
}

void vmActionGroup::setScheme ( ActionPanelScheme* pointer )
{
	mScheme = pointer;
	mHeader->setScheme ( pointer );
	mGroup->setScheme ( pointer );
	update ();
}

QBoxLayout* vmActionGroup::groupLayout () const
{
	return mGroup->groupLayout ();
}

bool vmActionGroup::addEntry ( vmWidget* entry, QLayout* l, const bool addStretch )
{
	if ( entry )
	{
		if ( entry->type () == WT_ACTION )
		{
			auto label ( new vmActionLabel ( dynamic_cast<vmAction*> ( entry ), this ) );
			mGroup->addQEntry ( label->toQWidget (), l, addStretch );
		}
		else
			mGroup->addQEntry ( entry->toQWidget (), l, addStretch );
		return true;
	}
	return false;
}

void vmActionGroup::addQEntry ( QWidget* widget, QLayout* l, const bool addStretch )
{
	mGroup->addQEntry ( widget, l, addStretch );
}

void vmActionGroup::addLayout ( QLayout* layout )
{
	mGroup->addLayout ( layout );
}

void vmActionGroup::showHide ()
{
	if ( m_foldStep != 0.0 )
		return;

	if ( !mHeader->expandable () )
		return;

	if ( mGroup->isVisible () )
	{
		m_foldPixmap = mGroup->transparentRender ();
		m_tempHeight = m_fullHeight = mGroup->height ();
		m_foldDelta = m_fullHeight / mScheme->groupFoldSteps;
		m_foldStep = mScheme->groupFoldSteps;
		m_foldDirection = -1;
		mGroup->hide ();
		mDummy->setFixedSize ( mGroup->size () );
		mDummy->show ();
		timerHide->start ( mScheme->groupFoldDelay );
	}
	else
	{
		m_foldStep = mScheme->groupFoldSteps;
		m_foldDirection = 1;
		m_tempHeight = 0.0;
		timerShow->start ( mScheme->groupFoldDelay );
	}
	mDummy->show ();
}

void vmActionGroup::processHide ()
{
	if ( --m_foldStep <= 0.0 )
	{
		m_foldStep = 0.0;
		mDummy->setFixedHeight ( 0 );
		mDummy->hide ();
		setFixedHeight ( mHeader->height () );
		setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );
		return;
	}
	setUpdatesEnabled ( false );
	m_tempHeight -= m_foldDelta;
	mDummy->setFixedHeight ( static_cast<int>(m_tempHeight) );
	setFixedHeight ( mDummy->height () + mHeader->height () );
	timerHide->start ( mScheme->groupFoldDelay );
	setUpdatesEnabled ( true );
}

void vmActionGroup::processShow ()
{
	if ( --m_foldStep <= 0.0 )
	{
		m_foldStep = 0.0;
		mDummy->hide ();
		m_foldPixmap = QPixmap ();
		mGroup->show ();
		setFixedHeight ( static_cast<int>(m_fullHeight) + mHeader->height () );
		setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );
		setMaximumHeight ( 9999 );
		setMinimumHeight ( 0 );
		return;
	}
	setUpdatesEnabled ( false );
	m_tempHeight += m_foldDelta;
	mDummy->setFixedHeight ( static_cast<int>(m_tempHeight) );
	setFixedHeight ( mDummy->height () + mHeader->height () );
	timerShow->start ( mScheme->groupFoldDelay );
	setUpdatesEnabled ( true );
}

void vmActionGroup::paintEvent ( QPaintEvent * )
{
	QPainter p ( this );

	if ( mDummy->isVisible () )
	{
		if ( mScheme->groupFoldThaw )
		{
			if ( m_foldDirection < 0 )
				p.setOpacity ( m_foldStep / static_cast<double>(mScheme->groupFoldSteps) );
			else
				p.setOpacity ( (static_cast<double>(mScheme->groupFoldSteps) - m_foldStep) / static_cast<double>(mScheme->groupFoldSteps) );
		}

		switch ( mScheme->groupFoldEffect )
		{
			case ActionPanelScheme::ShrunkFolding:
				p.drawPixmap ( mDummy->pos (), m_foldPixmap.scaled ( mDummy->size () ) );
			break;

			case ActionPanelScheme::SlideFolding:
				p.drawPixmap ( mDummy->pos (), m_foldPixmap,
						   QRect ( 0, m_foldPixmap.height () - mDummy->height (),
								   m_foldPixmap.width (), mDummy->width () ) );
			break;

			case ActionPanelScheme::NoFolding:
				p.drawPixmap ( mDummy->pos () , m_foldPixmap );
		}
	}
}

bool vmActionGroup::isExpandable () const
{
	return mHeader->expandable ();
}

void vmActionGroup::setExpandable ( const bool expandable )
{
	mHeader->setExpandable ( expandable );
}

bool vmActionGroup::hasHeader () const
{
	return mHeader->isVisible ();
}

void vmActionGroup::setHeader ( const bool enable )
{
	mHeader->setVisible ( enable );
}

QString vmActionGroup::headerText () const
{
	return mHeader->mTitle->text ();
}

void vmActionGroup::setHeaderText ( const QString& headerText )
{
	mHeader->mTitle->setText ( headerText );
}

QSize vmActionGroup::minimumSizeHint () const
{
	return { 200, ( mbStretchContents ? 100 : mGroup->minimumHeight () + mHeader->height () ) };
}
//--------------------------------------ACTION-GROUP-----------------------------------------------
