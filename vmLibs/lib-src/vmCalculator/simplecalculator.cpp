#include "simplecalculator.h"
#include "calculator.h"
#include "heapmanager.h"

#include <vmUtils/fast_library_functions.h>
#include <vmWidgets/vmwidgets.h>

#include <QtGui/QCloseEvent>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QLabel>

//------------------------------------------DIALOG-----------------------------------------------
simpleCalculator::simpleCalculator ( QWidget* parent )
	: QDialog ( parent ), m_calc ( new vmCalculator )
{
	setupUi ();
}

simpleCalculator::~simpleCalculator ()
{
	heap_del ( m_calc );
}

void simpleCalculator::setupUi ()
{
	auto gridLayout ( new QGridLayout );
	gridLayout->setSpacing ( 2 );
	gridLayout->setContentsMargins ( 2, 2, 2, 2 );
	txtResult = new QPlainTextEdit;
	txtResult->setReadOnly ( true );
	gridLayout->addWidget ( txtResult, 0, 0, 1, 2 );

	auto lblExpression ( new QLabel );
	lblExpression->setTextFormat ( Qt::PlainText );
	gridLayout->addWidget ( lblExpression, 1, 0, 1, 1 );

	txtInput = new vmLineEdit;
	txtInput->setEditable ( true );
	txtInput->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* qe, const vmWidget* const ) {
				return relevantKeyPressed ( qe ); } );
	gridLayout->addWidget ( txtInput, 1, 1, 1, 1 );

	auto horizontalLayout ( new QHBoxLayout );
	btnCalc = new QPushButton;
	btnCalc->setText ( APP_TR_FUNC ( "Calculate" ) );
	btnCalc->setIcon ( ICON ( "kcalc" ) );
	horizontalLayout->addWidget ( btnCalc );

	btnCopyResult = new QPushButton;
	btnCopyResult->setText ( APP_TR_FUNC ( "Copy result" ) );
	btnCopyResult->setIcon ( ICON ( "edit-copy" ) );
	horizontalLayout->addWidget ( btnCopyResult );
	btnClose = new QPushButton;
	btnClose->setText ( APP_TR_FUNC ( "Close" ) );
	btnClose->setIcon (	ICON ( "window-close" ) );
	horizontalLayout->addWidget ( btnClose );

	gridLayout->addLayout ( horizontalLayout, 2, 0, 1, 2 );

	static_cast<void>( connect ( btnCalc, &QPushButton::clicked , this, [&] () { return calculate (); } ) );
	static_cast<void>( connect ( btnClose, &QPushButton::clicked , this, [&] () { txtReceiveResult = nullptr; hide (); return; } ) );
	static_cast<void>( connect ( btnCopyResult, &QPushButton::clicked , this, [&] () { return btnCopyResultClicked (); } ) );
	btnCopyResult->setDefault ( true );

	setLayout ( gridLayout );
}

void simpleCalculator::showCalc ( const QPoint& pos, vmLineEdit* line, QWidget* parentWindow )
{
	txtReceiveResult = line;
	setParent ( parentWindow );
	show ();
	move ( pos.x () + width () , pos.y () );
#ifdef USING_QT6
    activateWindow ();
#else
    qApp->setActiveWindow ( this );
#endif
	txtInput->setFocus ( Qt::ActiveWindowFocusReason );
}

void simpleCalculator::showCalc ( const QString& input, const QPoint& pos, vmLineEdit* line, QWidget* parentWindow )
{
	txtInput->setText ( input );
	showCalc ( pos, line, parentWindow );
}

void simpleCalculator::calculate ()
{
	if ( !txtInput->text ().isEmpty () )
	{
		QString formula ( txtInput->text () );
 		if ( !formula.at ( 0 ).isDigit () )
		{
			const Token::Op& op ( Token::matchOperator ( formula.at ( 0 ) ) );
			if ( op == Token::InvalidOp )
				return;
			if ( op != Token::Equal )
				formula.prepend ( mStrResult );
		}

		formula = m_calc->autoFix ( formula );
		if ( formula.isEmpty () )
			return;

		m_calc->setExpression ( formula );
		m_calc->eval ( mStrResult );
		mStrResult.replace ( CHR_DOT, CHR_COMMA );
		txtResult->appendPlainText ( formula + CHR_EQUAL + mStrResult + CHR_NEWLINE );
		txtInput->clear ();
	}
}

void simpleCalculator::relevantKeyPressed ( const QKeyEvent* qe )
{
	if ( qe->key () == Qt::Key_Escape )
		hide ();
	else
		calculate ();
}

void simpleCalculator::btnCopyResultClicked ()
{
	calculate ();
	VM_LIBRARY_FUNCS::copyToClipboard ( mStrResult );
	if ( txtReceiveResult )
		txtReceiveResult->setText ( mStrResult, true );
}
//------------------------------------------DIALOG-----------------------------------------------
