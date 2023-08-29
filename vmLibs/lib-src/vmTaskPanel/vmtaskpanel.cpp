#include "vmtaskpanel.h"
#include "vmactiongroup.h"
#include "actionpanelscheme.h"

#include <vmWidgets/vmwidgets.h>

vmTaskPanel::vmTaskPanel ( const QString& title, QWidget* parent )
	: QFrame ( parent ), mScheme ( nullptr ), mTitle ( nullptr ), mGroups ( 8 )
{
	setProperty ( "class", QStringLiteral ( "panel" ) );
	setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );

	mLayout = new QVBoxLayout;
	mLayout->setMargin ( 5 );
	mLayout->setSpacing ( 5 );
	mLayout->setSizeConstraint ( QLayout::SetMinAndMaxSize );
	setLayout ( mLayout );

	if ( !title.isEmpty () )
	{
		mTitle = new vmActionLabel ( title );
		mLayout->insertWidget( 0, mTitle );
	}
}

void vmTaskPanel::setTitle ( const QString& new_title )
{
	if ( !mTitle )
	{
		mTitle = new vmActionLabel ( new_title );
		mLayout->insertWidget( 0, mTitle );
	}
	else
		mTitle->setText ( new_title );
}

void vmTaskPanel::setScheme ( const QString& style )
{
	ActionPanelScheme* scheme ( ActionPanelScheme::newScheme ( style ) );
	if ( scheme )
	{
		setStyleSheet ( scheme->actionStyle );
		for ( uint i ( 0 ); i < mGroups.count (); ++i )
			mGroups.at ( i )->setScheme ( scheme );
		
		delete mScheme;
		mScheme = scheme;
		update ();
	}
}

void vmTaskPanel::addWidget ( QWidget* w )
{
	if ( w )
		mLayout->addWidget ( w );
}

void vmTaskPanel::addStretch ( const int s )
{
	mLayout->addStretch ( s );
}

vmActionGroup* vmTaskPanel::createGroup (const QString& title, const bool expandable,
		const bool stretchContents, const bool closable )
{
	auto box ( new vmActionGroup ( title, expandable, stretchContents, closable, this ) );
	addWidget ( box );
	mGroups.append ( box );
	return box;
}

vmActionGroup* vmTaskPanel::createGroup ( const QIcon& icon, const QString& title,
		const bool expandable, const bool stretchContents, const bool closable )
{
	auto box ( new vmActionGroup ( icon, title, expandable, stretchContents, closable, this ) );
	addWidget ( box );
	mGroups.append ( box );
	return box;
}

void vmTaskPanel::removeGroup ( vmActionGroup* group, const bool bDelete )
{
	if ( group )
	{
		mLayout->removeWidget ( group );
		mGroups.removeOne ( group, 0, bDelete );
	}
}

QSize vmTaskPanel::minimumSizeHint () const
{
	return { 200, 150 };
}
