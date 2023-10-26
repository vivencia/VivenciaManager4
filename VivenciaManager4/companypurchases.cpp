#include "global.h"
#include "companypurchases.h"
#include "suppliersdlg.h"
#include "searchui.h"
#include "system_init.h"

#include <heapmanager.h>

#include <dbRecords/vivenciadb.h>
#include <dbRecords/inventory.h>
#include <dbRecords/companypurchases.h>
#include <dbRecords/dblistitem.h>
#include <dbRecords/dbtablewidget.h>
#include <dbRecords/completers.h>

#include <vmTaskPanel/vmtaskpanel.h>
#include <vmTaskPanel/vmactiongroup.h>
#include <vmUtils/fast_library_functions.h>
#include <vmWidgets/vmwidgets.h>
#include <vmNumbers/vmnumberformats.h>
#include <vmNotify/vmnotify.h>

#include <QtGui/QKeyEvent>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>

enum btnNames { BTN_FIRST = 0, BTN_PREV = 1, BTN_NEXT = 2, BTN_LAST = 3, BTN_ADD = 4, BTN_EDIT = 5, BTN_DEL = 6, BTN_SEARCH = 7 };
static bool bEnableStatus[4] = { false };

auto searchCPRecord = [] ( companyPurchases* cp_rec, const QString& searchTerm ) ->int
{
	for ( uint i ( 1 ); i < COMPANY_PURCHASES_FIELD_COUNT; ++i )
	{
		if ( static_cast<QString>(recStrValue ( cp_rec, i )).contains ( searchTerm, Qt::CaseInsensitive ) )
			return i;
	}
	return -1;
};

companyPurchasesUI::companyPurchasesUI ( QWidget* parent )
	: QDialog ( parent ), mbSearchIsOn ( false ), mFoundField ( -1 ),  mOldFoundField ( -1 ), cp_rec ( new companyPurchases ( true ) ), widgetList ( INVENTORY_FIELD_COUNT + 1 )
{
	setupUI ();
	const bool have_items ( VDB ()->getHighestID ( TABLE_CP_ORDER, VDB () ) > 0 );
	std::fill ( bEnableStatus, bEnableStatus+4, have_items );

	cp_rec->readLastRecord ();
	fillForms ();
}

companyPurchasesUI::~companyPurchasesUI ()
{
	heap_del ( cp_rec );
}

void companyPurchasesUI::showSearchResult ( const uint id )
{
	if ( cp_rec->readRecord ( id ) )
	{
		if ( !isVisible () )
			showNormal ();
		fillForms ();
	}
}

void companyPurchasesUI::setTotalPriceAsItChanges ( const vmTableItem* const item )
{
	setRecValue ( cp_rec, FLD_CP_TOTAL_PRICE, item->text () );
}

void companyPurchasesUI::setPayValueAsItChanges ( const vmTableItem* const item )
{
	txtCPPayValue->setText ( item->text () );
	setRecValue ( cp_rec, FLD_CP_PAY_VALUE, item->text () );
}

void companyPurchasesUI::tableItemsCellChanged ( const vmTableItem* const item )
{
	if ( item )
	{
		stringTable items ( recStrValue ( cp_rec, FLD_CP_ITEMS_REPORT ) );
		items.changeRecord ( static_cast<uint>(item->row ()), static_cast<uint>(item->column ()), item->text () );
		setRecValue ( cp_rec, FLD_CP_ITEMS_REPORT, items.toString () );
	}
}

bool companyPurchasesUI::tableRowRemoved ( const uint row )
{
	//TODO: maybe ask if user is certain about this action. Maybe, put the asking code under vmTableWidget::removeRow_slot ()
	stringTable items ( recStrValue ( cp_rec, FLD_CP_ITEMS_REPORT ) );
	items.removeRecord ( row );
	setRecValue ( cp_rec, FLD_CP_ITEMS_REPORT, items.toString () );
	return true;
}

void companyPurchasesUI::tablePaymentsCellChanged ( const vmTableItem* const item )
{
	if ( item )
	{
		stringTable items ( recStrValue ( cp_rec, FLD_CP_PAY_REPORT ) );
		items.changeRecord ( static_cast<uint>( item->row () ), static_cast<uint>( item->column () ), item->text () );
		setRecValue ( cp_rec, FLD_CP_PAY_REPORT, items.toString () );
	}
}

void companyPurchasesUI::saveWidget ( vmWidget* widget, const int id )
{
	widget->setID ( id );
	widgetList[id] = widget;
}

void companyPurchasesUI::setupUI ()
{
	//companyPurchasesresize(731, 544);
	auto gLayout ( new QGridLayout () );
	gLayout->setSpacing ( 5 );
	gLayout->setContentsMargins ( 5, 5, 5, 5 );

	dteCPDate = new vmDateEdit ( this );
	dteCPDeliveryDate = new vmDateEdit ( this );
	auto lblCPNotes ( new QLabel ( this ) );
	lblCPNotes->setText ( APP_TR_FUNC ( "Notes:" ) );
	auto lblDeliveryDate ( new QLabel ( this ) );
	lblDeliveryDate->setText ( APP_TR_FUNC ( "Delivery date:" ) );
	auto lblCPDate ( new QLabel ( this ) );
	lblCPDate->setText ( APP_TR_FUNC ( "Purchase date:" ) );
	auto lblCPPayValue ( new QLabel ( this ) );
	lblCPPayValue->setText ( APP_TR_FUNC ( "Price paid:" ) );
	auto lblCPDeliveryMethod ( new QLabel ( this ) );
	lblCPDeliveryMethod->setText ( APP_TR_FUNC ( "Delivery method:" ) );

	gLayout->addWidget ( dteCPDate, 6, 2, 1, 1 );
	gLayout->addWidget ( dteCPDeliveryDate, 6, 3, 1, 1 );
	gLayout->addWidget ( lblCPNotes, 8, 3, 1, 1 );
	gLayout->addWidget ( lblDeliveryDate, 5, 3, 1, 1 );
	gLayout->addWidget ( lblCPDate, 5, 2, 1, 1 );
	gLayout->addWidget ( lblCPPayValue, 8, 1, 1, 1 );
	gLayout->addWidget ( lblCPDeliveryMethod, 8, 2, 1, 1 );


	btnCPFirst = new QPushButton ( this );
	btnCPFirst->setIcon ( ICON ( ":/resources/browse-controls/first_rec.png" ) );
	btnCPPrev = new QPushButton ( this );
	btnCPPrev->setIcon ( ICON ( ":/resources/browse-controls/prev_rec.png" ) );
	btnCPNext = new QPushButton ( this );
	btnCPNext->setIcon ( ICON ( ":/resources/browse-controls/next_rec.png" ) );
	btnCPLast = new QPushButton ( this );
	btnCPLast->setIcon ( ICON ( ":/resources/browse-controls/last_rec.png" ) );

	auto line_1 ( new QFrame ( this ) );
	line_1->setFrameShape(QFrame::VLine);
	line_1->setFrameShadow(QFrame::Sunken);

	btnCPAdd = new QPushButton ( this );
	btnCPAdd->setCheckable ( true );
	btnCPAdd->setIcon ( ICON ( ":/resources/browse-controls/add_rec.png" ) );
	btnCPEdit = new QPushButton ( this );
	btnCPEdit->setIcon ( ICON ( ":/resources/browse-controls/edit_rec.png" ) );
	btnCPEdit->setCheckable ( true );
	btnCPRemove = new QPushButton ( this );
	btnCPRemove->setIcon ( ICON ( ":/resources/browse-controls/remove.png" ) );

	auto line_2 ( new QFrame ( this ) );
	line_2->setFrameShape ( QFrame::VLine );
	line_2->setFrameShadow ( QFrame::Sunken );

	auto lblCPID ( new QLabel ( this ) );
	lblCPID->setText ( APP_TR_FUNC ( "ID:" ) );

	txtCPID = new vmLineEdit ( this );
	QSizePolicy sizePolicy ( QSizePolicy::Fixed, QSizePolicy::Fixed );
	sizePolicy.setHorizontalStretch ( 0 );
	sizePolicy.setVerticalStretch ( 0 );
	sizePolicy.setHeightForWidth ( txtCPID->sizePolicy().hasHeightForWidth () );
	txtCPID->setSizePolicy(sizePolicy);
	txtCPID->setMaximumSize ( QSize ( 80, 30 ) );

	txtCPSearch = new vmLineEdit ( this );

	btnCPSearch = new QPushButton ( this );
	btnCPSearch->setIcon ( ICON ( ":/resources/replace-all.png" ) );
	btnCPSearch->setCheckable ( true );

	txtCPSupplier = new vmLineEdit ( this );
	btnCPShowSupplier = new QToolButton ( this );
	btnCPShowSupplier->setIcon ( ICON ( ":/resources/arrow-down.png" ) );
	btnCPShowSupplier->setCheckable ( true );

	txtCPNotes = new vmLineEdit ( this );

	auto lblItems ( new QLabel ( this ) );
	lblItems->setText ( APP_TR_FUNC ( "Items:" ) );
	tableItems = new dbTableWidget ( this );
	auto lblPayHistory ( new QLabel ( this ) );
	lblPayHistory->setText ( APP_TR_FUNC ( "Payment history:" ) );
	tablePayments = new dbTableWidget ( this );
	btnClose = new QPushButton ( this );
	btnClose->setText ( APP_TR_FUNC ( "Close" ) );

	txtCPDeliveryMethod = new vmLineEdit ( this );
	auto lblCPSupplier ( new QLabel ( this ) );
	lblCPSupplier->setText ( APP_TR_FUNC ( "Supplier:" ) );
	txtCPPayValue = new vmLineEditWithButton ( this );

	auto layoutBrowseControls ( new QHBoxLayout () );
	layoutBrowseControls->setSpacing ( 2 );
	layoutBrowseControls->addWidget ( btnCPFirst );
	layoutBrowseControls->addWidget ( btnCPPrev );
	layoutBrowseControls->addWidget ( btnCPNext );
	layoutBrowseControls->addWidget ( btnCPLast );
	layoutBrowseControls->addWidget ( line_1 );
	layoutBrowseControls->addWidget ( btnCPAdd );
	layoutBrowseControls->addWidget ( btnCPEdit );
	layoutBrowseControls->addWidget ( btnCPRemove );
	layoutBrowseControls->addWidget ( line_2 );
	layoutBrowseControls->addWidget ( lblCPID );
	layoutBrowseControls->addWidget ( txtCPID );
	layoutBrowseControls->addItem ( new QSpacerItem ( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ));
	layoutBrowseControls->addWidget ( txtCPSearch );
	layoutBrowseControls->addWidget ( btnCPSearch );

	auto layoutSupplierName ( new QHBoxLayout () );
	layoutSupplierName->setSpacing ( 2 );
	layoutSupplierName->addWidget ( txtCPSupplier );
	layoutSupplierName->addWidget( btnCPShowSupplier );

	auto layoutCPTable ( new QVBoxLayout () );
	layoutCPTable->setSpacing ( 5 );
	layoutCPTable->setContentsMargins ( 5, 5, 5, 5 );
	layoutCPTable->addWidget ( lblItems );
	layoutCPTable->addWidget ( tableItems );
	layoutCPTable->addWidget ( lblPayHistory );
	layoutCPTable->addWidget ( tablePayments );

	gLayout->addLayout ( layoutBrowseControls, 0, 1, 1, 3 );
	gLayout->addLayout ( layoutSupplierName, 6, 1, 1, 1 );
	gLayout->addWidget ( txtCPNotes, 9, 3, 1, 1 );
	gLayout->addLayout ( layoutCPTable, 10, 1, 1, 3 );
	gLayout->addWidget ( txtCPDeliveryMethod, 9, 2, 1, 1 );
	gLayout->addWidget ( lblCPSupplier, 5, 1, 1, 1 );
	gLayout->addWidget ( txtCPPayValue, 9, 1, 1, 1 );
	gLayout->addWidget ( btnClose, 11, 3, 1, 1 );

	QWidget::setTabOrder(btnCPFirst, btnCPPrev);
	QWidget::setTabOrder(btnCPPrev, btnCPNext);
	QWidget::setTabOrder(btnCPNext, btnCPLast);
	QWidget::setTabOrder(btnCPLast, txtCPSupplier);
	QWidget::setTabOrder(txtCPSupplier, btnCPShowSupplier);

	vmTaskPanel* panel ( new vmTaskPanel ( emptyString, this ) );
	auto mainLayout ( new QVBoxLayout );
	mainLayout->setContentsMargins ( 0, 0, 0, 0 );
	mainLayout->setSpacing ( 0 );
	mainLayout->addWidget ( panel, 1 );
	setLayout ( mainLayout );
	vmActionGroup* group = panel->createGroup ( emptyString, false, true, false );
	group->addLayout ( gLayout );
	MAINWINDOW ()->appMainStyle ( panel );
	setMinimumWidth ( tableItems->preferredSize ().width () );

	static_cast<void>( btnCPAdd->connect ( btnCPAdd, static_cast<void (QPushButton::*)( const bool )>( &QPushButton::clicked ),
			this, [&] ( const bool checked ) { return btnCPAdd_clicked ( checked ); } ) );
	static_cast<void>( btnCPEdit->connect ( btnCPEdit, static_cast<void (QPushButton::*)( const bool )>( &QPushButton::clicked ),
			this, [&] ( const bool checked ) { return btnCPEdit_clicked ( checked ); } ) );
	static_cast<void>( btnCPSearch->connect ( btnCPSearch, static_cast<void (QPushButton::*)( const bool )>( &QPushButton::clicked ),
			this, [&] ( const bool checked ) { return btnCPSearch_clicked ( checked ); } ) );
	static_cast<void>( btnCPShowSupplier->connect ( btnCPShowSupplier, static_cast<void (QToolButton::*)( const bool )>( &QToolButton::clicked ),
			this, [&] ( const bool checked ) { return btnCPShowSupplier_clicked ( checked ); } ) );
	static_cast<void>( btnCPFirst->connect ( btnCPFirst, &QPushButton::clicked, this, [&] () { return btnCPFirst_clicked (); } ) );
	static_cast<void>( btnCPPrev->connect ( btnCPPrev, &QPushButton::clicked, this, [&] () { return btnCPPrev_clicked (); } ) );
	static_cast<void>( btnCPNext->connect ( btnCPNext, &QPushButton::clicked, this, [&] () { return btnCPNext_clicked (); } ) );
	static_cast<void>( btnCPLast->connect ( btnCPLast, &QPushButton::clicked, this, [&] () { return btnCPLast_clicked (); } ) );
	static_cast<void>( btnCPRemove->connect ( btnCPRemove, &QPushButton::clicked, this, [&] () { return btnCPRemove_clicked (); } ) );
	static_cast<void>( btnClose->connect ( btnClose, &QPushButton::clicked, this, [&] () { return hide (); } ) );

	dbTableWidget::createPurchasesTable ( tableItems );
	saveWidget ( tableItems, FLD_CP_ITEMS_REPORT );
	tableItems->setKeepModificationRecords ( false );
	tableItems->setParentLayout ( layoutCPTable );
	tableItems->setCallbackForMonitoredCellChanged ( [&] ( const vmTableItem* const item ) {
			return setTotalPriceAsItChanges ( item ); } );
	tableItems->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );
	tableItems->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) {
			return tableItemsCellChanged ( item ); } );
	tableItems->setCallbackForRowRemoved ( [&] ( const uint row ) {
			return tableRowRemoved ( row ); } );

	dbTableWidget::createPayHistoryTable ( tablePayments, PHR_METHOD );
	saveWidget ( tablePayments, FLD_CP_PAY_REPORT );
	tablePayments->setParentLayout ( layoutCPTable );
	tablePayments->setCallbackForMonitoredCellChanged ( [&] ( const vmTableItem* const item ) {
			return setPayValueAsItChanges ( item ); } );
	tablePayments->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );
	tablePayments->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) {
			return tablePaymentsCellChanged ( item ); } );

	COMPLETERS ()->setCompleterForWidget ( txtCPSupplier, CC_SUPPLIER );
	saveWidget ( txtCPSupplier, FLD_CP_SUPPLIER );
	txtCPSupplier->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCP_textAltered ( sender ); } );
	txtCPSupplier->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	COMPLETERS ()->setCompleterForWidget ( txtCPDeliveryMethod, CC_DELIVERY_METHOD );
	saveWidget ( txtCPDeliveryMethod, FLD_CP_DELIVERY_METHOD );
	txtCPDeliveryMethod->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCP_textAltered ( sender ); } );
	txtCPDeliveryMethod->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	saveWidget ( txtCPPayValue, FLD_CP_PAY_VALUE );
	txtCPPayValue->lineControl ()->setTextType ( vmLineEdit::TT_PRICE );
	txtCPPayValue->setButtonType ( 0, vmLineEditWithButton::LEBT_CALC_BUTTON );
	txtCPPayValue->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCP_textAltered ( sender ); } );
	txtCPPayValue->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );
	
	saveWidget ( txtCPNotes, FLD_CP_NOTES );
	txtCPNotes->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCP_textAltered ( sender ); } );
	txtCPNotes->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	saveWidget ( dteCPDate, FLD_CP_DATE );
	dteCPDate->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return dteCP_dateAltered ( sender ); } );
	dteCPDate->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	saveWidget ( dteCPDeliveryDate, FLD_CP_DELIVERY_DATE );
	dteCPDeliveryDate->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return dteCP_dateAltered ( sender ); } );
	dteCPDeliveryDate->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	txtCPSearch->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCPSearch_textAltered ( sender->text () ); } );
	txtCPSearch->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const, const vmWidget* const ) {
			return btnCPSearch_clicked ( true ); } );
}

void companyPurchasesUI::fillForms ()
{
	txtCPID->setText ( recStrValue ( cp_rec, FLD_CP_ID ) );
	dteCPDate->setDate ( cp_rec->date ( FLD_CP_DATE ) );
	dteCPDeliveryDate->setDate ( cp_rec->date ( FLD_CP_DELIVERY_DATE ) );
	txtCPPayValue->setText ( recStrValue ( cp_rec, FLD_CP_PAY_VALUE ) );
	txtCPDeliveryMethod->setText ( recStrValue ( cp_rec, FLD_CP_DELIVERY_METHOD ) );
	txtCPSupplier->setText ( recStrValue ( cp_rec, FLD_CP_SUPPLIER ) );
	txtCPNotes->setText ( recStrValue ( cp_rec, FLD_CP_NOTES ) );

	tableItems->loadFromStringTable ( recStrValue ( cp_rec, FLD_CP_ITEMS_REPORT ) );
	tableItems->scrollToTop ();
	tablePayments->loadFromStringTable ( recStrValue ( cp_rec, FLD_CP_PAY_REPORT ) );
	tablePayments->scrollToTop ();

	controlForms ();
}

void companyPurchasesUI::controlForms ()
{
	const bool editable ( cp_rec->action () >= ACTION_ADD );
	dteCPDate->setEditable ( editable );
	dteCPDeliveryDate->setEditable ( editable );
	txtCPPayValue->setEditable ( editable );
	txtCPDeliveryMethod->setEditable ( editable );
	txtCPSupplier->setEditable ( editable );
	txtCPNotes->setEditable ( editable );
	tableItems->setEditable ( editable );
	tablePayments->setEditable ( editable );
	txtCPSearch->setEditable ( !editable );

	if ( editable )
	{
		bEnableStatus[BTN_FIRST] = btnCPFirst->isEnabled ();
		bEnableStatus[BTN_PREV] = btnCPPrev->isEnabled ();
		bEnableStatus[BTN_NEXT] = btnCPNext->isEnabled ();
		bEnableStatus[BTN_LAST] = btnCPLast->isEnabled ();
		btnCPFirst->setEnabled ( false );
		btnCPPrev->setEnabled ( false );
		btnCPNext->setEnabled ( false );
		btnCPLast->setEnabled ( false );
		btnCPAdd->setEnabled ( cp_rec->action () == ACTION_ADD );
		btnCPEdit->setEnabled ( cp_rec->action () == ACTION_EDIT );
		btnCPSearch->setEnabled ( false );
		btnCPRemove->setText ( tr ( "Cancel" ) );
		btnCPRemove->setIcon ( ICON ( "cancel" ) );
	}
	else
	{
		btnCPFirst->setEnabled ( bEnableStatus[BTN_FIRST] );
		btnCPPrev->setEnabled ( bEnableStatus[BTN_PREV] );
		btnCPNext->setEnabled ( bEnableStatus[BTN_NEXT] );
		btnCPLast->setEnabled ( bEnableStatus[BTN_LAST] );
		btnCPAdd->setEnabled ( true );
		btnCPEdit->setEnabled ( true );
		btnCPSearch->setEnabled ( true );
		
		btnCPEdit->setText ( emptyString );
		btnCPEdit->setIcon ( ICON ( "browse-controls/edit" ) );
		btnCPAdd->setText ( emptyString );
		btnCPAdd->setIcon ( ICON ( "browse-controls/add" ) );
		btnCPRemove->setText ( emptyString );
		btnCPRemove->setIcon ( ICON ( "browse-controls/remove" ) );
	}

	if ( mbSearchIsOn )
	{
		if ( mOldFoundField != mFoundField )
		{
			if ( widgetList.at ( mOldFoundField ) )
				widgetList.at ( mOldFoundField )->highlight ( vmDefault_Color );
			mOldFoundField = mFoundField;
			widgetList.at ( mFoundField )->highlight ( vmBlue, mSearchTerm );
		}
	}
}

void companyPurchasesUI::saveInfo ()
{
	if ( cp_rec->saveRecord () )
		cp_rec->exportToInventory ();
	controlForms ();
}

void companyPurchasesUI::searchCancel ()
{
	if ( widgetList.at ( mFoundField ) )
		widgetList.at ( mFoundField )->highlight ( vmDefault_Color );
	mOldFoundField = -1;
	mFoundField = -1;
	mSearchTerm.clear ();
	btnCPSearch->setText ( emptyString );
	mbSearchIsOn = false;
	cp_rec->resetQuery ();
}

void companyPurchasesUI::prepareSearch ( const QString& searchterm )
{
	static const QString searchQueryTemplate ( QStringLiteral ( "SELECT * FROM #2# WHERE MATCH (#3#) AGAINST (\"#4#\" IN BOOLEAN MODE)" ) );
	static QString strSearchColumns;
	if ( strSearchColumns.isEmpty () )
	{
		int sc ( -1 );
		for ( uint i ( SC_REPORT_1 ); i <= SC_EXTRA; ++i )
		{
			sc = cp_rec->searchCategoryTranslate ( static_cast<SEARCH_CATEGORIES> ( i ) );
			if ( sc != -1 )
			{
				strSearchColumns.append ( VDB ()->getTableColumnName ( cp_rec->tableInfo (), sc ) );
				strSearchColumns.append ( CHR_COMMA );
			}
		}
		strSearchColumns.chop ( 1 );
	}

	mSearchTerm = searchterm;
	QString strQuery ( searchQueryTemplate );
	strQuery.replace ( QStringLiteral( "#2#" ), cp_rec->tableInfo ()->table_name ); // Table name
	strQuery.replace ( QStringLiteral( "#4#" ), mSearchTerm ); //The searched string
	strQuery.replace ( QStringLiteral( "#3#" ), strSearchColumns ); //Field (column) in which to search for the search term
	cp_rec->setQuery ( strQuery );
}

void companyPurchasesUI::btnCPFirst_clicked ()
{
	bool ok ( false );
	if ( mbSearchIsOn )
		ok = cp_rec->readFirstRecord ( -1, emptyString );
	else
		ok = cp_rec->readFirstRecord ();

	bEnableStatus[BTN_FIRST] = !ok;
	bEnableStatus[BTN_PREV] = !ok;
	bEnableStatus[BTN_NEXT] = ok;
	bEnableStatus[BTN_LAST] = ok;
	fillForms ();
}

void companyPurchasesUI::btnCPLast_clicked ()
{
	bool ok ( false );
	if ( mbSearchIsOn )
		ok = cp_rec->readLastRecord ( -1, mSearchTerm );
	else
		ok = cp_rec->readLastRecord ();

	bEnableStatus[BTN_FIRST] = ok;
	bEnableStatus[BTN_PREV] = ok;
	bEnableStatus[BTN_NEXT] = !ok;
	bEnableStatus[BTN_LAST] = !ok;
	fillForms ();
}

void companyPurchasesUI::btnCPPrev_clicked ()
{
	bool ok ( false );
	if ( mbSearchIsOn )
		ok = cp_rec->readPrevRecord ( true );
	else
		ok = cp_rec->readPrevRecord ();

	const bool b_isfirst ( static_cast<uint>( recIntValue ( cp_rec, FLD_CP_ID ) ) == VDB ()->getLowestID ( TABLE_CP_ORDER, VDB () ) );
	bEnableStatus[BTN_FIRST] = ok && !b_isfirst;
	bEnableStatus[BTN_PREV] = ok && !b_isfirst;
	bEnableStatus[BTN_NEXT] |= ok;
	bEnableStatus[BTN_LAST] |= ok;
	fillForms ();
}

void companyPurchasesUI::btnCPNext_clicked ()
{
	bool ok ( false );
	if ( mbSearchIsOn )
		ok = cp_rec->readNextRecord ( true );
	else
		ok = cp_rec->readNextRecord ();

	const bool b_islast ( static_cast<uint>( recIntValue ( cp_rec, FLD_CP_ID ) ) == VDB ()->getHighestID ( TABLE_CP_ORDER, VDB () ) );
	bEnableStatus[BTN_FIRST] |= ok;
	bEnableStatus[BTN_PREV] |= ok;
	bEnableStatus[BTN_NEXT] = ok && !b_islast;
	bEnableStatus[BTN_LAST] = ok && !b_islast;
	fillForms ();
}

void companyPurchasesUI::btnCPSearch_clicked ( const bool checked )
{
	if ( checked )
	{
		if ( txtCPSearch->text ().count () >= 2 && txtCPSearch->text () != mSearchTerm )
		{
			searchCancel ();
			prepareSearch ( txtCPSearch->text () );
			if ( cp_rec->readFirstRecord ( -1 ) )
			{
				mFoundField = searchCPRecord ( cp_rec, mSearchTerm );
				mbSearchIsOn = true;
				btnCPSearch->setText ( tr ( "Cancel search" ) );
				bEnableStatus[BTN_FIRST] = false;
				bEnableStatus[BTN_PREV] = false;
				btnCPSearch->setChecked ( true );
				fillForms ();
			}
		}
	}
	else
	{
		searchCancel ();
		txtCPSearch->clear ();
		btnCPSearch->setChecked ( false );
	}
}

void companyPurchasesUI::btnCPAdd_clicked ( const bool checked )
{
	if ( checked )
	{
		cp_rec->clearAll ();
		cp_rec->setAction ( ACTION_ADD );
		fillForms ();
		controlForms ();
		btnCPAdd->setText ( tr ( "Save" ) );
		btnCPAdd->setIcon ( ICON ( "document-save" ) );
		txtCPSupplier->setFocus ();
	}
	else
		saveInfo ();
}

void companyPurchasesUI::btnCPEdit_clicked ( const bool checked )
{
	if ( checked )
	{
		cp_rec->setAction ( ACTION_EDIT );
		btnCPEdit->setText ( tr ( "Save" ) );
		btnCPEdit->setIcon ( ICON ( "document-save" ) );
		txtCPSupplier->setFocus ();
	}
	else
		saveInfo ();
	controlForms ();
}

void companyPurchasesUI::btnCPRemove_clicked ()
{
	if ( btnCPAdd->isChecked () || btnCPEdit->isChecked () ) // cancel
	{
		btnCPAdd->setChecked ( false );
		btnCPEdit->setChecked ( false );
		cp_rec->setAction( ACTION_REVERT );
		controlForms ();
		fillForms ();
	}
	else
	{
		if ( NOTIFY ()->questionBox ( tr ( "Question" ), tr ( "Remove current displayed record ?" ) ) )
		{
			if ( cp_rec->deleteRecord () )
			{
				if ( !cp_rec->readNextRecord () )
				{
					if ( !cp_rec->readPrevRecord () )
						cp_rec->readRecord ( 1 );
				}
				fillForms ();
			}
		}
	}
}

void companyPurchasesUI::relevantKeyPressed ( const QKeyEvent* ke )
{
	switch ( ke->key () )
	{
		case Qt::Key_Enter:
		case Qt::Key_Return:
			if ( btnCPAdd->isChecked () )
			{
				btnCPAdd->setChecked ( false );
				btnCPAdd_clicked ( false );
			}
			else if ( btnCPEdit->isChecked () )
			{
				btnCPEdit->setChecked ( false );
				btnCPEdit_clicked ( false );
			}
		break;
		case Qt::Key_Escape:
			if ( btnCPAdd->isChecked () || btnCPEdit->isChecked () ) // cancel
				btnCPRemove_clicked ();
		break;
		case Qt::Key_F2:
			if ( !btnCPAdd->isChecked () && !btnCPEdit->isChecked () )
				btnCPPrev_clicked ();
		break;
		case Qt::Key_F3:
			if ( !btnCPAdd->isChecked () && !btnCPEdit->isChecked () )
				btnCPNext_clicked ();
		break;
		default:
		break;
	}
}

void companyPurchasesUI::txtCP_textAltered ( const vmWidget* const sender )
{
	setRecValue ( cp_rec, static_cast<uint>( sender->id () ), sender->text () );
}

void companyPurchasesUI::dteCP_dateAltered ( const vmWidget* const sender )
{
    setRecValue ( cp_rec, static_cast<uint>( sender->id () ), static_cast<const vmDateEdit*>( sender )->date ().toString ( DATE_FORMAT_DB ) );
}

void companyPurchasesUI::btnCPShowSupplier_clicked ( const bool checked )
{
	if ( checked )
		SUPPLIERS ()->displaySupplier ( recStrValue ( cp_rec, FLD_CP_SUPPLIER ), true );
	else
		SUPPLIERS ()->hideDialog ();
}

void companyPurchasesUI::txtCPSearch_textAltered ( const QString &text )
{
	if ( text.length () == 0 )
	{
		txtCPSearch->setText ( mSearchTerm );
		btnCPSearch->setEnabled ( !mSearchTerm.isEmpty () );
	}
	btnCPSearch->setEnabled ( text.length () >= 3 );
}
