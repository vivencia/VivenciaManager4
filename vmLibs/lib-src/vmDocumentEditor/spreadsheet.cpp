#include "spreadsheet.h"
#include "vmlibs.h"
#include "heapmanager.h"

#include "documenteditor.h"

#include <vmCalculator/simplecalculator.h>
#include <vmStringRecord/stringrecord.h>
#include <vmNotify/vmnotify.h>

#include <dbRecords/vivenciadb.h>
#include <dbRecords/job.h>
#include <dbRecords/completers.h>

#include <QtWidgets/QDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>

const uint N_SPREADSHEET_FIELDS ( 8 );

constexpr inline uint guiColumnToDBColumn ( const uint gui_col )
{
	return gui_col + 2;
}

constexpr inline uint dbColumnToGuiColumn ( const uint db_col )
{
	return db_col - 2;
}

static const QString MODIFIED ( APP_TR_FUNC ( "Modified - " ) );

spreadSheetEditor::spreadSheetEditor ( documentEditor* mdiParent )
	: QDialog (), m_parentEditor ( mdiParent ), m_table ( nullptr ), mCompleterManager ( nullptr ), mbQPChanged ( false ),
	  qp_rec ( new quickProject ), mJob ( nullptr ), funcClosed ( nullptr )
{
	setWindowFlags ( Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint ); // remove close button
	setCompleterManager ( parentEditor ()->completerManager () );
	setupUI ();
}

spreadSheetEditor::~spreadSheetEditor ()
{
	heap_del ( m_table );
	heap_del ( qp_rec );
}

void spreadSheetEditor::setupUI ()
{
	//--------------------------------CONTROL-BUTTONS--------------------------------------
	btnEditTable = new QPushButton ( TR_FUNC ( "&Edit" ) );
	btnEditTable->setCheckable ( true );
	btnEditTable->setFixedSize ( 150, 30 );
	static_cast<void>(connect ( btnEditTable, static_cast<void (QPushButton::*)( const bool )> ( &QPushButton::clicked ), this, [&] ( const bool checked ) {
				return editTable ( checked ); }) );

	btnCancel = new QPushButton ( TR_FUNC ( "&Cancel" ) );
	btnCancel->setFixedSize ( 150, 30 );
	static_cast<void>(connect ( btnCancel, &QPushButton::clicked, this, [&] () { return cancelEdit (); } ));
	//--------------------------------CONTROL-BUTTONS--------------------------------------

	btnClose = new QPushButton ( TR_FUNC ( "&Close" ) );
	btnClose->setFixedSize ( 100, 30 );
	static_cast<void>(connect ( btnClose, &QPushButton::clicked, this, [&] () { return closeClicked (); } ));

	auto hLayout1 ( new QHBoxLayout );
	hLayout1->setMargin ( 2 );
	hLayout1->setSpacing ( 2 );
	hLayout1->addWidget ( btnEditTable, 1 );
	hLayout1->addWidget ( btnCancel, 1 );
	hLayout1->addWidget ( btnClose, 1 );

	m_table = new vmTableWidget;
	vmTableColumn* fields ( m_table->createColumns( N_SPREADSHEET_FIELDS ) );

	for ( uint i ( 0 ); i < N_SPREADSHEET_FIELDS; ++i )
	{
		fields[i].label = VivenciaDB::getTableColumnLabel ( VivenciaDB::tableInfo ( TABLE_QP_ORDER ), guiColumnToDBColumn ( i ) );
		switch ( i )
		{
			case SPREADSHEET_ITEM: //FLD_QP_ITEM = 3 - Column A
				fields[SPREADSHEET_ITEM].completer_type = PRODUCT_OR_SERVICE;
				fields[SPREADSHEET_ITEM].width = 250;
			break;
			case SPREADSHEET_SELL_QUANTITY: //FLD_QP_SELL_QUANTITY = 4 - Column B
				fields[SPREADSHEET_SELL_QUANTITY].formula = QStringLiteral ( "= E%1" );
				fields[SPREADSHEET_SELL_QUANTITY].text_type = vmLineEdit::TT_DOUBLE;
			break;
			case SPREADSHEET_PURCHASE_QUANTITY: //FLD_QP_PURCHASE_QUANTITY = 7 - Column E
				fields[SPREADSHEET_PURCHASE_QUANTITY].text_type = vmLineEdit::TT_DOUBLE;
			break;
			case SPREADSHEET_SELL_TOTAL_PRICE: //FLD_QP_SELL_TOTALPRICE = 6 - Column D
				fields[SPREADSHEET_SELL_TOTAL_PRICE].formula = QStringLiteral ( "* B%1 C%1" );
				fields[SPREADSHEET_SELL_TOTAL_PRICE].text_type = vmLineEdit::TT_PRICE;
				fields[SPREADSHEET_SELL_TOTAL_PRICE].editable = false;
			break;
			case SPREADSHEET_PURCHASE_TOTAL_PRICE: //FLD_QP_PURCHASE_TOTALPRICE = 9 - Column G
				fields[SPREADSHEET_PURCHASE_TOTAL_PRICE].formula = QStringLiteral ( "* E%1 F%1" );
				fields[SPREADSHEET_PURCHASE_TOTAL_PRICE].text_type = vmLineEdit::TT_PRICE;
				fields[SPREADSHEET_PURCHASE_TOTAL_PRICE].editable = false;
			break;
			case SPREADSHEET_RESULT: //FLD_QP_PROFIT = 10 - Column H
				fields[SPREADSHEET_RESULT].formula = QStringLiteral ( "- D%1 G%1" );
				fields[SPREADSHEET_RESULT].text_type = vmLineEdit::TT_PRICE;
				fields[SPREADSHEET_RESULT].editable = false;
			break;
			case SPREADSHEET_SELL_UNIT_PRICE: //FLD_QP_SELL_UNIT_PRICE = 5 - Column C
				fields[SPREADSHEET_SELL_UNIT_PRICE].formula = QStringLiteral ( "= F%1" );
				fields[SPREADSHEET_SELL_UNIT_PRICE].text_type = vmLineEdit::TT_PRICE;
				fields[SPREADSHEET_SELL_UNIT_PRICE].button_type = vmLineEditWithButton::LEBT_CALC_BUTTON;
			break;
			case SPREADSHEET_PURCHASE_UNIT_PRICE: //FLD_QP_PURCHASE_UNIT_PRICE = 8 - Column F
				fields[SPREADSHEET_PURCHASE_UNIT_PRICE].text_type = vmLineEdit::TT_PRICE;
				fields[SPREADSHEET_PURCHASE_UNIT_PRICE].button_type = vmLineEditWithButton::LEBT_CALC_BUTTON;
			break;
		}
	}

	m_table->setKeepModificationRecords ( false );
	m_table->initTable ( 20 );

	m_table->setCallbackForCellChanged ( [&] ( const vmTableItem* const item ) {
				return cellModified ( item ); } );
	m_table->setCallbackForRowRemoved ( [&] ( const uint row ) { return rowRemoved ( row ); } );

	mLayoutMain = new QVBoxLayout;
	mLayoutMain->setMargin ( 2 );
	mLayoutMain->setSpacing ( 2 );
	m_table->addToLayout ( mLayoutMain );
	mLayoutMain->addLayout ( hLayout1, 0 );
	setLayout ( mLayoutMain );
	setMinimumSize ( 870, 450 );
}

void spreadSheetEditor::prepareToShow ( const Job* const job )
{
	mJob = const_cast<Job*>( job );
	loadData ( recStrValue ( job, FLD_JOB_ID ) );
	m_qpString = QLatin1String ( "QP-" ) + job->projectIDTemplate ();
	m_table->addToLayout ( mLayoutMain );
	m_table->scrollToTop ();
}

bool spreadSheetEditor::loadData ( const QString& jobid , const bool force )
{
	if ( force || jobid != recStrValue ( qp_rec, FLD_QP_JOB_ID ) )
	{
		qp_rec->setAction ( ACTION_READ );
		if ( qp_rec->readRecord ( FLD_QP_JOB_ID, jobid ) )
		{
			mbQPChanged = false;
			m_table->clear ();
			stringRecord fieldRecs;
			uint i_row ( 0 );

			for ( uint i_col ( FLD_QP_ITEM ); i_col < FLD_QP_RESULT; ++i_col )
			{
				if ( i_col != FLD_QP_SELL_TOTALPRICE && i_col != FLD_QP_PURCHASE_TOTAL_PRICE )
				{
					fieldRecs.fromString ( recStrValue ( qp_rec, i_col ) );
					if ( fieldRecs.first () )
					{
						i_row = 0;
						do
						{
							m_table->setCellValue ( fieldRecs.curValue (), i_row++, dbColumnToGuiColumn ( i_col ) );
						} while ( fieldRecs.next () );
					}
				}
			}
		}
		else
		{
			qp_rec->clearAll ();
			m_table->clear ();
		}
	}
	m_table->scrollToTop ();
	return !m_table->isEmpty ();
}

QString spreadSheetEditor::getJobIDFromQPString ( const QString& qpIDstr ) const
{
	const int idx ( qpIDstr.indexOf ( CHR_HYPHEN ) );
	if ( idx != -1 )
		return qpIDstr.right ( qpIDstr.count () - idx - 1 );
	return QStringLiteral ( "-1" );
}

void spreadSheetEditor::completeItem ( const QModelIndex& index )
{
	const stringRecord record ( completerManager ()->getCompleter (
									PRODUCT_OR_SERVICE )->completionModel ()->data (
									index.sibling ( index.row (), 1 ) ).toString () );

	if ( record.isOK () )
	{
		if ( m_table->currentRow () == -1 )
			m_table->setCurrentCell ( 0, SPREADSHEET_ITEM, QItemSelectionModel::Select );
		if ( m_table->currentRow () != -1 )
		{
			const auto current_row ( static_cast<uint>(m_table->currentRow ()) );
			m_table->sheetItem ( current_row, SPREADSHEET_ITEM )->setText ( record.fieldValue ( ISR_NAME ), false, true );
			m_table->sheetItem ( current_row, SPREADSHEET_PURCHASE_UNIT_PRICE )->setText ( record.fieldValue ( ISR_UNIT_PRICE ), false, true );
			m_table->sheetItem ( current_row, SPREADSHEET_PURCHASE_QUANTITY )->setText ( CHR_ONE, false, true );
			m_table->setCurrentCell ( static_cast<int>(current_row), SPREADSHEET_SELL_QUANTITY, QItemSelectionModel::ClearAndSelect );
		}
	}
}

void spreadSheetEditor::cellModified ( const vmTableItem* const item )
{
	if ( item != nullptr )
	{
		if ( item->column () == FLD_QP_SELL_TOTALPRICE || item->column () == FLD_QP_PURCHASE_TOTAL_PRICE  ||
			 item->column () == FLD_QP_RESULT )
		{
			qDebug () << "auto column modified";
			return;
		}
		if ( item->row () < m_table->totalsRow () )
		{
			stringRecord rowRec ( recStrValue ( qp_rec, guiColumnToDBColumn ( static_cast<uint>(item->column ()) ) ) );
			rowRec.changeValue ( static_cast<uint>(item->row ()), item->text () );
			setRecValue ( qp_rec, guiColumnToDBColumn ( static_cast<uint>(item->column ()) ), rowRec.toString () );
			setWindowTitle ( MODIFIED + windowTitle () );
		}
	}
}

bool spreadSheetEditor::rowRemoved ( const uint row )
{
	stringRecord rowRec;
	for ( uint i ( 0 ); i < m_table->colCount (); ++i )
	{
		rowRec.fromString ( recStrValue ( qp_rec, i ) );
		rowRec.removeField ( row );
		setRecValue ( qp_rec, i, rowRec.toString () );
	}
	mbQPChanged = true;
	return true;
}

void spreadSheetEditor::enableControls ( const bool enable )
{
	m_table->setEditable ( enable );
	btnCancel->setEnabled ( enable );
	btnClose->setEnabled ( !enable );
	btnEditTable->setText ( enable ? TR_FUNC ( "&Save" ) : TR_FUNC ( "&Edit" ) );
}

void spreadSheetEditor::editTable ( const bool checked )
{
	enableControls ( checked );
	m_table->setFocus ();
	if ( checked )
	{
		qp_rec->setAction ( recIntValue ( qp_rec, FLD_QP_ID ) > 0 ? ACTION_EDIT : ACTION_ADD );
		setRecValue ( qp_rec, FLD_QP_JOB_ID, recStrValue ( mJob, FLD_JOB_ID ) );
		static_cast<void>(connect ( completerManager ()->getCompleter ( PRODUCT_OR_SERVICE ),
				  static_cast<void (QCompleter::*)( const QModelIndex& )>(&QCompleter::activated), this,
				  [&] ( const QModelIndex& idx ) { return completeItem ( idx ); } ) );
	}
	else
	{
		mbQPChanged |= qp_rec->saveRecord ();
		disconnect ( completerManager ()->getCompleter ( PRODUCT_OR_SERVICE ), nullptr, this, nullptr );
	}
}

void spreadSheetEditor::cancelEdit ()
{
	static_cast<void>(disconnect ( completerManager ()->getCompleter ( PRODUCT_OR_SERVICE ), nullptr, this, nullptr ));
	btnEditTable->setChecked ( false );
	enableControls ( false );
	qp_rec->setAction ( ACTION_REVERT );
	const QString jobid ( recStrValue ( qp_rec, FLD_QP_JOB_ID ) );
	loadData ( jobid, true );
}

void spreadSheetEditor::getHeadersText ( spreadRow* row ) const
{
	for ( uint i_col ( 0 ); i_col < unsigned ( m_table->columnCount () ); ++i_col )
		row->field_value.append ( m_table->sheetItem ( 0, i_col )->text () );
}

void spreadSheetEditor::setCompleterManager ( vmCompleters* const completer )
{
	heap_del ( mCompleterManager );
	mCompleterManager = completer;
}

void spreadSheetEditor::closeClicked ()
{
	if ( mbQPChanged && funcClosed != nullptr )
		funcClosed ();
	enableControls ( false );
	hide ();
}
