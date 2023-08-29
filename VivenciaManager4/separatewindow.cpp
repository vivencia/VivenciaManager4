#include "separatewindow.h"
#include "system_init.h"
#include "mainwindow.h"

#include <QtGui/QCloseEvent>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

separateWindow::separateWindow ( QWidget *w_child )
	: QDialog (), m_child ( w_child ), mainLayout ( new QVBoxLayout ),
	  mb_Active ( false ), mb_Visible ( false ), w_funcReturnToParent ( nullptr )
{
	btnReturn = new QPushButton ( tr ( "Return" ), this );
	connect ( btnReturn, &QPushButton::clicked, this, [&] () { return returnToParent (); } );
	mainLayout->addWidget ( btnReturn, 0, Qt::AlignRight );
	setLayout ( mainLayout );
	MAINWINDOW ()->installEventFilter ( this );
}

void separateWindow::closeEvent ( QCloseEvent* e )
{
	e->ignore ();
}

// Hides and shows this windows whenever the parent window is minimized/hidden or visible/shown
bool separateWindow::eventFilter ( QObject* o, QEvent* e )
{
	if ( o == MAINWINDOW () )
	{
		switch ( e->type () )
		{
			default:
				e->ignore ();
			return false;
			
			case QEvent::WindowStateChange:
				if ( MAINWINDOW ()->isMinimized () || MAINWINDOW ()->isHidden () )
					childHideRequested ();
				else
					childShowRequested ();
			break;
			case QEvent::Hide:
				childHideRequested ();
			break;
			case QEvent::Show:
				childShowRequested ();
			break;
		}
		e->accept ();
		return true;
	}
	e->ignore ();
	return false;
}

// The next functions are here to avoid exploitation of the multi show/hide
// capabilities of QDialog; only one function is allowed to show the dialog:
// void separateWindow::showSeparate () , and only one to close it void separateWindow::returnToParent ()
int separateWindow::exec ()
{
	return QDialog::Rejected;
}

void separateWindow::open ()
{
	setResult ( QDialog::Rejected );
}

void separateWindow::show ()
{
	setResult ( QDialog::Rejected );
}

void separateWindow::accept ()
{
	setResult ( QDialog::Rejected );
}

void separateWindow::reject ()
{
	setResult ( QDialog::Rejected );
}

void separateWindow::done ( int )
{
	setResult ( QDialog::Rejected );
}

void separateWindow::hide ()
{
	if ( mb_Visible && !mb_Active && ( result () == QDialog::Accepted ) )
	{
		mb_Visible = false;
		QDialog::hide ();
	}
}

void separateWindow::showSeparate ( const QString& window_title, const bool b_exec, const Qt::WindowStates w_state )
{
	if ( mb_Active )
	{
		if ( w_state & windowState () )
			return;
	}

	if ( !mb_Active )
	{
		mb_Active = true;
		if ( m_child )
		{
			mainLayout->insertWidget ( 0, m_child, 2 );
			m_child->show ();
			m_child->setFocus ();
			resize ( mainLayout->sizeHint () );
			mb_Visible = true;
			setWindowTitle ( window_title );
		}
	}
	//toggle state. The first if check makes sure that w_state is different from current windowState (),
	// so now we only have to toggle it
	setWindowState ( windowState () ^ Qt::WindowMaximized );
	if ( !b_exec )
		QDialog::show ();
	else
		QDialog::exec ();
}

void separateWindow::childHideRequested ()
{
	if ( mb_Visible && mb_Active )
	{
		mb_Visible = false;
		QDialog::hide ();
	}
}

void separateWindow::childShowRequested ()
{
	if ( !mb_Visible && mb_Active )
	{
		mb_Visible = true;
		QDialog::show ();
	}
}

void separateWindow::childCloseRequested ( const int )
{
	returnToParent ();
}

void separateWindow::returnToParent ()
{
	if ( mb_Visible && mb_Active )
	{
		mb_Active = false;
		setResult ( QDialog::Accepted );
		if ( m_child )
		{
			mainLayout->removeWidget ( m_child );
			if ( w_funcReturnToParent )
				w_funcReturnToParent ( m_child );
		}
		mainLayout->removeWidget ( btnReturn );
		hide ();
	}
}
