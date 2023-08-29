#include "companypurchasesui.h"
#include "ui_companypurchasesui.h"
#include "global.h"
#include "heapmanager.h"

#include "searchui.h"
#include "suppliersdlg.h"
#include "system_init.h"

#include <dbRecords/vivenciadb.h>
#include <dbRecords/inventory.h>
#include <dbRecords/companypurchases.h>
#include <dbRecords/completers.h>

#include <vmUtils/fast_library_functions.h>

#include <vmWidgets/vmwidgets.h>
#include <vmWidgets/vmlistitem.h>

#include <vmNumbers/vmnumberformats.h>

#include <vmNotify/vmnotify.h>

#include <QtGui/QKeyEvent>

companyPurchasesUI* companyPurchasesUI::s_instance ( nullptr );

void deleteCompanyPurchasesUiInstance ()
{
	heap_del ( companyPurchasesUI::s_instance );
}

enum btnNames { BTN_FIRST = 0, BTN_PREV = 1, BTN_NEXT = 2, BTN_LAST = 3, BTN_ADD = 4, BTN_EDIT = 5, BTN_DEL = 6, BTN_SEARCH = 7 };
static bool bEnableStatus[4] = { false };

companyPurchasesUI::companyPurchasesUI ( QWidget* parent )
	: QDialog ( parent ), ui ( new Ui::companyPurchasesUI () ),
	  mbSearchIsOn ( false ), cp_rec ( new companyPurchases ( true ) ), widgetList ( INVENTORY_FIELD_COUNT + 1 ),
	  mFoundFields ( 0, 5 )
{
	ui->setupUi ( this );
	setupUI ();
	const bool have_items ( VDB ()->getHighestID ( TABLE_CP_ORDER ) > 0 );
	ui->btnCPEdit->setEnabled ( true );
	ui->btnCPRemove->setEnabled ( have_items );
	ui->btnCPNext->setEnabled ( have_items );
	ui->btnCPFirst->setEnabled ( false );
	ui->btnCPPrev->setEnabled ( have_items );
	ui->btnCPLast->setEnabled ( have_items );
	ui->txtCPSearch->setEditable ( have_items );
	ui->btnCPSearch->setEnabled ( have_items );

	cp_rec->readLastRecord ();
	fillForms ();

	Sys_Init::addPostRoutine ( deleteCompanyPurchasesUiInstance );
}

companyPurchasesUI::~companyPurchasesUI ()
{
	heap_del ( cp_rec );
	heap_del ( ui );
}

void companyPurchasesUI::showSearchResult ( vmListItem* item, const bool bshow )
{
	if ( bshow )
	{
		if ( cp_rec->readRecord ( item->dbRecID () ) )
		{
			if ( !isVisible () )
				showNormal ();
			fillForms ();
		}
	}
	for ( uint i ( 0 ); i < COMPANY_PURCHASES_FIELD_COUNT; ++i )
	{
		if ( item->searchFieldStatus ( i ) == SS_SEARCH_FOUND )
			widgetList.at ( i )->highlight ( bshow ? vmBlue : vmDefault_Color, SEARCH_UI ()->searchTerm () );
	}
}

void companyPurchasesUI::showSearchResult_internal ( const bool bshow )
{
	for ( uint i ( 0 ); i < mFoundFields.count (); ++i )
		widgetList.at ( i )->highlight ( bshow ? vmBlue : vmDefault_Color, SEARCH_UI ()->searchTerm () );
	if ( bshow )
		fillForms ();
}

void companyPurchasesUI::setTotalPriceAsItChanges ( const vmTableItem* const item )
{
	setRecValue ( cp_rec, FLD_CP_TOTAL_PRICE, item->text () );
}

void companyPurchasesUI::setPayValueAsItChanges ( const vmTableItem* const item )
{
	ui->txtCPPayValue->setText ( item->text () );
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
	qDebug () << items.toString();
	items.removeRecord ( row );
	qDebug () << items.toString();
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
	static_cast<void>( ui->btnCPAdd->connect ( ui->btnCPAdd, static_cast<void (QPushButton::*)( const bool )>( &QPushButton::clicked ),
			this, [&] ( const bool checked ) { return btnCPAdd_clicked ( checked ); } ) );
	static_cast<void>( ui->btnCPEdit->connect ( ui->btnCPEdit, static_cast<void (QPushButton::*)( const bool )>( &QPushButton::clicked ),
			this, [&] ( const bool checked ) { return btnCPEdit_clicked ( checked ); } ) );
	static_cast<void>( ui->btnCPSearch->connect ( ui->btnCPSearch, static_cast<void (QPushButton::*)( const bool )>( &QPushButton::clicked ),
			this, [&] ( const bool checked ) { return btnCPSearch_clicked ( checked ); } ) );
	static_cast<void>( ui->btnCPShowSupplier->connect ( ui->btnCPShowSupplier, static_cast<void (QToolButton::*)( const bool )>( &QToolButton::clicked ),
			this, [&] ( const bool checked ) { return btnCPShowSupplier_clicked ( checked ); } ) );
	static_cast<void>( ui->btnCPFirst->connect ( ui->btnCPFirst, &QPushButton::clicked, this, [&] () { return btnCPFirst_clicked (); } ) );
	static_cast<void>( ui->btnCPPrev->connect ( ui->btnCPPrev, &QPushButton::clicked, this, [&] () { return btnCPPrev_clicked (); } ) );
	static_cast<void>( ui->btnCPNext->connect ( ui->btnCPNext, &QPushButton::clicked, this, [&] () { return btnCPNext_clicked (); } ) );
	static_cast<void>( ui->btnCPLast->connect ( ui->btnCPLast, &QPushButton::clicked, this, [&] () { return btnCPLast_clicked (); } ) );
	static_cast<void>( ui->btnCPRemove->connect ( ui->btnCPRemove, &QPushButton::clicked, this, [&] () { return btnCPRemove_clicked (); } ) );
	static_cast<void>( ui->btnClose->connect ( ui->btnClose, &QPushButton::clicked, this, [&] () { return hide (); } ) );

	static_cast<void>( vmTableWidget::createPurchasesTable ( ui->tableItems ) );
	saveWidget ( ui->tableItems, FLD_CP_ITEMS_REPORT );
	ui->tableItems->setKeepModificationRecords ( false );
	ui->tableItems->setParentLayout ( ui->layoutCPTable );
	ui->tableItems->setCallbackForMonitoredCellChanged ( [&] ( const vmTableItem* const item ) {
			return setTotalPriceAsItChanges ( item ); } );
	ui->tableItems->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );
	ui->tableItems->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) {
			return tableItemsCellChanged ( item ); } );
	ui->tableItems->setCallbackForRowRemoved ( [&] ( const uint row ) {
			return tableRowRemoved ( row ); } );

	static_cast<void>( vmTableWidget::createPayHistoryTable ( ui->tablePayments, this, PHR_METHOD ) );
	saveWidget ( ui->tablePayments, FLD_CP_PAY_REPORT );
	ui->tablePayments->setParentLayout ( ui->layoutCPTable );
	ui->tablePayments->setCallbackForMonitoredCellChanged ( [&] ( const vmTableItem* const item ) {
			return setPayValueAsItChanges ( item ); } );
	ui->tablePayments->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );
	ui->tablePayments->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) {
			return tablePaymentsCellChanged ( item ); } );

	APP_COMPLETERS ()->setCompleter ( ui->txtCPSupplier, vmCompleters::SUPPLIER );
	saveWidget ( ui->txtCPSupplier, FLD_CP_SUPPLIER );
	ui->txtCPSupplier->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCP_textAltered ( sender ); } );
	ui->txtCPSupplier->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	APP_COMPLETERS ()->setCompleter ( ui->txtCPDeliveryMethod, vmCompleters::DELIVERY_METHOD );
	saveWidget ( ui->txtCPDeliveryMethod, FLD_CP_DELIVERY_METHOD );
	ui->txtCPDeliveryMethod->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCP_textAltered ( sender ); } );
	ui->txtCPDeliveryMethod->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	saveWidget ( ui->txtCPPayValue, FLD_CP_PAY_VALUE );
	ui->txtCPPayValue->lineControl ()->setTextType ( vmLineEdit::TT_PRICE );
	ui->txtCPPayValue->setButtonType ( vmLineEditWithButton::LEBT_CALC_BUTTON );
	ui->txtCPPayValue->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCP_textAltered ( sender ); } );
	ui->txtCPPayValue->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );
	
	saveWidget ( ui->txtCPNotes, FLD_CP_NOTES );
	ui->txtCPNotes->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCP_textAltered ( sender ); } );
	ui->txtCPNotes->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	saveWidget ( ui->dteCPDate, FLD_CP_DATE );
	ui->dteCPDate->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return dteCP_dateAltered ( sender ); } );
	ui->dteCPDate->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	saveWidget ( ui->dteCPDeliveryDate, FLD_CP_DELIVERY_DATE );
	ui->dteCPDeliveryDate->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return dteCP_dateAltered ( sender ); } );
	ui->dteCPDeliveryDate->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const ke, const vmWidget* const ) {
			return relevantKeyPressed ( ke ); } );

	ui->txtCPSearch->setCallbackForContentsAltered ( [&] ( const vmWidget* const sender ) {
			return txtCPSearch_textAltered ( sender->text () ); } );
	ui->txtCPSearch->setCallbackForRelevantKeyPressed ( [&] ( const QKeyEvent* const, const vmWidget* const ) {
			return btnCPSearch_clicked ( true ); } );
}

void companyPurchasesUI::fillForms ()
{
	ui->txtCPID->setText ( recStrValue ( cp_rec, FLD_CP_ID ) );
	ui->dteCPDate->setDate ( cp_rec->date ( FLD_CP_DATE ) );
	ui->dteCPDeliveryDate->setDate ( cp_rec->date ( FLD_CP_DELIVERY_DATE ) );
	ui->txtCPPayValue->setText ( recStrValue ( cp_rec, FLD_CP_PAY_VALUE ) );
	ui->txtCPDeliveryMethod->setText ( recStrValue ( cp_rec, FLD_CP_DELIVERY_METHOD ) );
	ui->txtCPSupplier->setText ( recStrValue ( cp_rec, FLD_CP_SUPPLIER ) );
	ui->txtCPNotes->setText ( recStrValue ( cp_rec, FLD_CP_NOTES ) );

	ui->tableItems->loadFromStringTable ( recStrValue ( cp_rec, FLD_CP_ITEMS_REPORT ) );
	ui->tableItems->scrollToTop ();
	ui->tablePayments->loadFromStringTable ( recStrValue ( cp_rec, FLD_CP_PAY_REPORT ) );
	ui->tablePayments->scrollToTop ();
}

void companyPurchasesUI::controlForms ()
{
	const bool editable ( cp_rec->action () >= ACTION_ADD );
	ui->dteCPDate->setEditable ( editable );
	ui->dteCPDeliveryDate->setEditable ( editable );
	ui->txtCPPayValue->setEditable ( editable );
	ui->txtCPDeliveryMethod->setEditable ( editable );
	ui->txtCPSupplier->setEditable ( editable );
	ui->txtCPNotes->setEditable ( editable );
	ui->tableItems->setEditable ( editable );
	ui->tablePayments->setEditable ( editable );
	ui->txtCPSearch->setEditable ( !editable );

	if ( editable )
	{
		bEnableStatus[BTN_FIRST] = ui->btnCPFirst->isEnabled ();
		bEnableStatus[BTN_PREV] = ui->btnCPPrev->isEnabled ();
		bEnableStatus[BTN_NEXT] = ui->btnCPNext->isEnabled ();
		bEnableStatus[BTN_LAST] = ui->btnCPLast->isEnabled ();
		ui->btnCPFirst->setEnabled ( false );
		ui->btnCPPrev->setEnabled ( false );
		ui->btnCPNext->setEnabled ( false );
		ui->btnCPLast->setEnabled ( false );
		ui->btnCPAdd->setEnabled ( cp_rec->action () == ACTION_ADD );
		ui->btnCPEdit->setEnabled ( cp_rec->action () == ACTION_EDIT );
		ui->btnCPSearch->setEnabled ( false );
	}
	else
	{
		ui->btnCPFirst->setEnabled ( bEnableStatus[BTN_FIRST] );
		ui->btnCPPrev->setEnabled ( bEnableStatus[BTN_PREV] );
		ui->btnCPNext->setEnabled ( bEnableStatus[BTN_NEXT] );
		ui->btnCPLast->setEnabled ( bEnableStatus[BTN_LAST] );
		ui->btnCPAdd->setEnabled ( true );
		ui->btnCPEdit->setEnabled ( true );
		ui->btnCPSearch->setEnabled ( true );
		
		ui->btnCPEdit->setText ( emptyString );
		ui->btnCPEdit->setIcon ( ICON ( "browse-controls/edit" ) );
		ui->btnCPAdd->setText ( emptyString );
		ui->btnCPAdd->setIcon ( ICON ( "browse-controls/add" ) );
		ui->btnCPRemove->setText ( emptyString );
		ui->btnCPRemove->setIcon ( ICON ( "browse-controls/remove" ) );
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
	for ( uint i ( 0 ); i < mFoundFields.count (); ++i )
		widgetList.at ( mFoundFields.at ( i ) )->highlight ( vmDefault_Color );
	mFoundFields.clearButKeepMemory ();
	mSearchTerm.clear ();
	mbSearchIsOn = false;
}

bool companyPurchasesUI::searchFirst ()
{
	if ( cp_rec->readFirstRecord ( -1, mSearchTerm ) )
	{
		showSearchResult_internal ( false ); // unhighlight current widgets
		cp_rec->contains ( mSearchTerm, mFoundFields );
		return true;
	}
	return false;
}

bool companyPurchasesUI::searchPrev ()
{
	if ( cp_rec->readPrevRecord ( true ) )
	{
		showSearchResult_internal ( false ); // unhighlight current widgets
		cp_rec->contains ( mSearchTerm, mFoundFields );
		return true;
	}
	return false;
}

bool companyPurchasesUI::searchNext ()
{
	if ( cp_rec->readNextRecord ( true ) )
	{
		showSearchResult_internal ( false ); // unhighlight current widgets
		cp_rec->contains ( mSearchTerm, mFoundFields );
		return true;
	}
	return false;
}

bool companyPurchasesUI::searchLast ()
{
	if ( cp_rec->readLastRecord ( -1, mSearchTerm ) )
	{
		showSearchResult_internal ( false ); // unhighlight current widgets
		cp_rec->contains ( mSearchTerm, mFoundFields );
		return true;
	}
	return false;
}

void companyPurchasesUI::btnCPFirst_clicked ()
{
	bool ok ( false );
	if ( mbSearchIsOn )
	{
		if ( ( ok = searchFirst () ) )
			showSearchResult_internal ( true );
	}
	else
	{
		if ( ( ok = cp_rec->readFirstRecord () ) )
			fillForms ();
	}

	ui->btnCPFirst->setEnabled ( !ok );
	ui->btnCPPrev->setEnabled ( !ok );
	ui->btnCPNext->setEnabled ( ui->btnCPNext->isEnabled () || ok );
	ui->btnCPLast->setEnabled ( ui->btnCPLast->isEnabled () || ok );
}

void companyPurchasesUI::btnCPLast_clicked ()
{
	bool ok ( false );
	if ( mbSearchIsOn )
	{
		if ( ( ok = searchLast () ) )
			showSearchResult_internal ( true );
	}
	else
	{
		ok = cp_rec->readLastRecord ();
		fillForms ();
	}

	ui->btnCPFirst->setEnabled ( ui->btnCPFirst->isEnabled () || ok );
	ui->btnCPPrev->setEnabled ( ui->btnCPPrev->isEnabled () || ok );
	ui->btnCPNext->setEnabled ( !ok );
	ui->btnCPLast->setEnabled ( !ok );
}

void companyPurchasesUI::btnCPPrev_clicked ()
{
	bool b_isfirst ( false );
	bool ok ( false );

	if ( mbSearchIsOn )
	{
		if ( ( ok = searchPrev () ) )
		{
			b_isfirst = ( static_cast<uint>( recIntValue ( cp_rec, FLD_CP_ID ) ) == VDB ()->getLowestID ( cp_rec->t_info.table_order ) );
			showSearchResult_internal ( true );
		}
	}
	else
	{
		ok = cp_rec->readPrevRecord ();
		b_isfirst = ( static_cast<uint>( recIntValue ( cp_rec, FLD_CP_ID ) ) == VDB ()->getLowestID ( TABLE_CP_ORDER ) );
		fillForms ();
	}

	ui->btnCPFirst->setEnabled ( !b_isfirst );
	ui->btnCPPrev->setEnabled ( !b_isfirst );
	ui->btnCPNext->setEnabled ( ui->btnCPNext->isEnabled () || ok );
	ui->btnCPLast->setEnabled ( ui->btnCPLast->isEnabled () || ok );
}

void companyPurchasesUI::btnCPNext_clicked ()
{
	bool b_islast ( false );
	bool ok ( false );
	if ( mbSearchIsOn )
	{
		if ( ( ok = searchPrev () ) )
		{
			b_islast = ( static_cast<uint>( recIntValue ( cp_rec, FLD_CP_ID ) ) == VDB ()->getHighestID ( cp_rec->t_info.table_order ) );
			showSearchResult_internal ( true );
		}
	}
	else
	{
		ok = cp_rec->readNextRecord ();
		b_islast = ( static_cast<uint>( recIntValue ( cp_rec, FLD_CP_ID ) ) == VDB ()->getHighestID ( TABLE_CP_ORDER ) );
		fillForms ();
	}

	ui->btnCPFirst->setEnabled ( ui->btnCPFirst->isEnabled () || ok );
	ui->btnCPPrev->setEnabled ( ui->btnCPPrev->isEnabled () || ok );
	ui->btnCPNext->setEnabled ( !b_islast );
	ui->btnCPLast->setEnabled ( !b_islast );
}

void companyPurchasesUI::btnCPSearch_clicked ( const bool checked )
{
	if ( checked )
	{
		if ( ui->txtCPSearch->text ().count () >= 2 && ui->txtCPSearch->text () != mSearchTerm )
		{
			searchCancel ();
			const QString searchTerm ( ui->txtCPSearch->text () );
			if ( cp_rec->readFirstRecord ( -1, searchTerm ) )
			{
				cp_rec->contains ( searchTerm, mFoundFields );
				mbSearchIsOn = !mFoundFields.isEmpty ();
			}
			ui->btnCPNext->setEnabled ( mbSearchIsOn );
			if ( mbSearchIsOn ) {
				mSearchTerm = searchTerm;
				ui->btnCPSearch->setText ( tr ( "Cancel search" ) );
				showSearchResult_internal ( true );
			}
		}
	}
	else
		searchCancel ();
	ui->btnCPSearch->setChecked ( checked );
}

void companyPurchasesUI::btnCPAdd_clicked ( const bool checked )
{
	if ( checked )
	{
		cp_rec->clearAll ();
		cp_rec->setAction ( ACTION_ADD );
		fillForms ();
		controlForms ();
		ui->btnCPAdd->setText ( tr ( "Save" ) );
		ui->btnCPAdd->setIcon ( ICON ( "document-save" ) );
		ui->btnCPRemove->setText ( tr ( "Cancel" ) );
		ui->btnCPRemove->setIcon ( ICON ( "cancel" ) );
		ui->txtCPSupplier->setFocus ();
	}
	else
		saveInfo ();
}

void companyPurchasesUI::btnCPEdit_clicked ( const bool checked )
{
	if ( checked )
	{
		cp_rec->setAction ( ACTION_EDIT );
		controlForms ();
		ui->btnCPEdit->setText ( tr ( "Save" ) );
		ui->btnCPEdit->setIcon ( ICON ( "document-save" ) );
		ui->btnCPRemove->setText ( tr ( "Cancel" ) );
		ui->btnCPRemove->setIcon ( ICON ( "cancel" ) );
		ui->txtCPSupplier->setFocus ();
	}
	else
		saveInfo ();
}

void companyPurchasesUI::btnCPRemove_clicked ()
{
	if ( ui->btnCPAdd->isChecked () || ui->btnCPEdit->isChecked () ) // cancel
	{
		ui->btnCPAdd->setChecked ( false );
		ui->btnCPEdit->setChecked ( false );
		cp_rec->setAction( ACTION_REVERT );
		controlForms ();
		fillForms ();
	}
	else
	{
		if ( VM_NOTIFY ()->questionBox ( tr ( "Question" ), tr ( "Remove current displayed record ?" ) ) )
		{
			if ( cp_rec->deleteRecord () )
			{
				if ( !cp_rec->readNextRecord () )
				{
					if ( !cp_rec->readPrevRecord () )
						cp_rec->readRecord ( 1 );
				}
				fillForms ();
				ui->btnCPFirst->setEnabled ( cp_rec->actualRecordInt ( FLD_CP_ID ) != static_cast<int>( VDB ()->getLowestID ( TABLE_CP_ORDER ) ) );
				ui->btnCPPrev->setEnabled ( cp_rec->actualRecordInt ( FLD_CP_ID ) != static_cast<int>( VDB ()->getLowestID ( TABLE_CP_ORDER ) ) );
				ui->btnCPNext->setEnabled ( cp_rec->actualRecordInt ( FLD_CP_ID ) != static_cast<int>( VDB ()->getHighestID ( TABLE_CP_ORDER ) ) );
				ui->btnCPLast->setEnabled ( cp_rec->actualRecordInt ( FLD_CP_ID ) != static_cast<int>( VDB ()->getHighestID ( TABLE_CP_ORDER ) ) );
				ui->btnCPSearch->setEnabled ( VDB ()->getHighestID ( TABLE_CP_ORDER  ) > 0 );
				ui->txtCPSearch->setEditable ( VDB ()->getHighestID ( TABLE_CP_ORDER ) > 0 );
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
			if ( ui->btnCPAdd->isChecked () )
			{
				ui->btnCPAdd->setChecked ( false );
				btnCPAdd_clicked ( false );
			}
			else if ( ui->btnCPEdit->isChecked () )
			{
				ui->btnCPEdit->setChecked ( false );
				btnCPEdit_clicked ( false );
			}
		break;
		case Qt::Key_Escape:
			if ( ui->btnCPAdd->isChecked () || ui->btnCPEdit->isChecked () ) // cancel
				btnCPRemove_clicked ();
		break;
		case Qt::Key_F2:
			if ( !ui->btnCPAdd->isChecked () && !ui->btnCPEdit->isChecked () )
				btnCPPrev_clicked ();
		break;
		case Qt::Key_F3:
			if ( !ui->btnCPAdd->isChecked () && !ui->btnCPEdit->isChecked () )
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
	setRecValue ( cp_rec, static_cast<uint>( sender->id () ), static_cast<const vmDateEdit* const>( sender )->date ().toString ( DATE_FORMAT_DB ) );
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
		ui->txtCPSearch->setText ( mSearchTerm );
		ui->btnCPSearch->setEnabled ( !mSearchTerm.isEmpty () );
	}
	ui->btnCPSearch->setEnabled ( text.length () >= 3 );
}
