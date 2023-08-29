#include "completers.h"
#include "vmlibs.h"
#include "heapmanager.h"

#include "dbrecord.h"
#include "completerrecord.h"

#include <vmUtils/fast_library_functions.h>
#include <vmWidgets/vmwidgets.h>
#include <vmStringRecord/stringrecord.h>

#include <QtGui/QStandardItemModel>

vmCompleters::vmCompleters ( const bool b_full_init )
	: mbFullInit ( b_full_init ), completersList ( COMPLETERS_COUNT + 1 )
{
	QCompleter* completer ( nullptr );
	if ( b_full_init )
	{
		QStandardItemModel* item_model ( nullptr );
		for ( uint i ( 0 ); i < COMPLETERS_COUNT; ++i )
		{
			completer = new QCompleter;
			completer->setCompletionMode ( QCompleter::PopupCompletion );
			completer->setCaseSensitivity ( Qt::CaseInsensitive );
			completer->setProperty ( "type", QVariant ( i ) );
			// Initialize SERVICES with two columns. The first holds a stringRecord with the pertiment
			// information; the second, an index for faster searches
			item_model = new QStandardItemModel ( 0, i != PRODUCT_OR_SERVICE ? 1 : 2, completer );
			completer->setModel ( item_model );
			completersList.insert ( i, completer );
		}
		loadCompleters ();
	}
}

vmCompleters::~vmCompleters ()
{
	completersList.clear ( true );
}

void vmCompleters::loadCompleters ()
{
	QStringList completer_strings;
	QStandardItemModel* item_model ( nullptr );
	auto model_all ( dynamic_cast<QStandardItemModel*> ( completersList.at ( ALL_CATEGORIES )->model () ) );

	for ( int i ( 2 ), x ( 0 ), str_count ( 0 ); i < static_cast<int>( COMPLETERS_COUNT ); ++i )
	{
		completerRecord::loadCompleterStrings ( completer_strings, static_cast<COMPLETER_CATEGORIES>( i ) );
		str_count = completer_strings.count ();
		if ( str_count > 0 )
		{
			item_model = dynamic_cast<QStandardItemModel*>( completersList.at ( i )->model () );
			for ( x = 0; x < str_count; ++x )
			{
				item_model->appendRow ( new QStandardItem ( completer_strings.at ( x ) ) );
				model_all->appendRow ( new QStandardItem ( completer_strings.at ( x ) ) );
			}
			completer_strings.clear ();
		}
	}

	QStringList completer_strings_2;
	completerRecord::loadCompleterStringsForProductsAndServices ( completer_strings, completer_strings_2 );
	const int str_count ( static_cast<int> ( qMin ( completer_strings.count (), completer_strings_2.count () ) ) );
	if ( str_count > 0 )
	{
		item_model = dynamic_cast<QStandardItemModel*>( completersList.at ( PRODUCT_OR_SERVICE )->model () );
		for ( int x ( 0 ); x < str_count; ++x )
		{
			item_model->insertRow ( x, QList<QStandardItem *> () <<
									new QStandardItem ( completer_strings.at ( x ) ) <<
									new QStandardItem ( completer_strings_2.at ( x ) ) );
		}
	}
}

void vmCompleters::setCompleterForWidget ( vmWidget* widget, const int completer_type )
{
	if ( completer_type >= 0 )
	{
		QCompleter* const completer ( getCompleter ( static_cast<COMPLETER_CATEGORIES> ( completer_type ) ) );

		switch ( widget->type () )
		{
			case WT_LINEEDIT:
				dynamic_cast<vmLineEdit*>( widget )->QLineEdit::setCompleter ( completer );
			break;
			case WT_LINEEDIT_WITH_BUTTON:
				dynamic_cast<vmLineEditWithButton*>( widget )->lineControl ()->QLineEdit::setCompleter ( completer );
			break;
			case WT_COMBO:
			{
				//static_cast<vmComboBox*>( widget )->vmComboBox::setCompleter ( completer );
				QStringList items;
				fillList ( static_cast<COMPLETER_CATEGORIES> ( completer_type ), items );
				dynamic_cast<vmComboBox*>( widget )->addItems ( items );
			}
			break;
			default:
			break;
		}
	}
}

void vmCompleters::updateCompleter ( const DBRecord* const db_rec, const uint field, const COMPLETER_CATEGORIES type )
{
	if ( type >= COMPLETER_CATEGORIES::SUPPLIER )
	{
		updateCompleter ( recStrValue ( db_rec, field ), type );
		if ( (type != SUPPLIER) && (type != BRAND) && (type != ITEM_NAMES) )
			return;
	}
	encodeCompleterISRForSpreadSheet ( db_rec );
}

void vmCompleters::updateCompleter ( const QString& str, const COMPLETER_CATEGORIES type )
{
	if ( str.isEmpty () || (str == QStringLiteral ( "N/A" )) )
        return;
    
	if ( type >= COMPLETER_CATEGORIES::SUPPLIER )
	{
		if ( inList ( str, type ) == -1 )
		{
			const QCompleter* completer ( completersList.at ( static_cast<int>( type ) ) );
			auto item_model ( dynamic_cast<QStandardItemModel*>(completer->model ()) );
		
			item_model->appendRow ( new QStandardItem ( str ) );
			auto model_all ( dynamic_cast<QStandardItemModel*>( completersList.at ( ALL_CATEGORIES )->model () ) );
			model_all->appendRow ( new QStandardItem ( str ) );
			completerRecord cr_rec;
			cr_rec.updateTable ( type, str );
		}
	}
}

void vmCompleters::fillList ( const COMPLETER_CATEGORIES type, QStringList& list ) const
{
	const auto* __restrict model ( dynamic_cast<QStandardItemModel*>( completersList.at ( static_cast<int> ( type ) )->model () ) );
	if ( model )
	{
		const auto n_items ( static_cast<uint> ( model->rowCount () ) );
		if ( n_items > 0 )
		{
			const QModelIndex index ( model->index ( 0, 0 ) );
			list.clear ();
			for ( uint i_row ( 0 ); i_row < n_items; ++i_row )
				insertStringIntoContainer ( list, model->data ( index.sibling ( static_cast<int>(i_row), 0 ) ).toString (),
											[&] ( const int i ) ->QString { return list.at ( i ); },
											[&] (const int i, const QString& text ) { list.insert ( i, text ); } );
				//static_cast<void>( VM_LIBRARY_FUNCS::insertStringListItem ( list, model->data ( index.sibling ( static_cast<int>(i_row), 0 ) ).toString () ) );
		}
	}
}

int vmCompleters::inList ( const QString& str, const COMPLETER_CATEGORIES type ) const
{
	if ( mbFullInit )
	{
		const auto* __restrict model ( dynamic_cast<QStandardItemModel*>( completersList.at ( static_cast<int> ( type ) )->model () ) );
		const QModelIndex index ( model->index ( 0, 0 ) );
		const int n_items ( model->rowCount () );
		for ( int i_row ( 0 ); i_row < n_items; ++i_row )
		{
			if ( str.compare ( model->data ( index.sibling ( i_row, 0 ) ).toString (), Qt::CaseInsensitive ) == 0 )
				return i_row;
		}
	}
	return -1;
}

COMPLETER_CATEGORIES vmCompleters::completerType ( QCompleter* completer, const QString& completion ) const
{
	if ( mbFullInit )
	{
		COMPLETER_CATEGORIES ret ( static_cast<COMPLETER_CATEGORIES>( completer->property ( "type" ).toInt () ) );
		if ( ret == ALL_CATEGORIES )
		{
			if ( !completion.isEmpty () )
			{
				for ( uint i = 0; i <= JOB_TYPE; ++i )
				{
					if ( i != static_cast<int>( ALL_CATEGORIES ) )
					{
						if ( inList ( completion, static_cast<COMPLETER_CATEGORIES>( i ) ) != -1 )
						{
							ret = static_cast<COMPLETER_CATEGORIES>( i );
							break;
						}
					}
				}
			}
		}
		return ret;
	}
	return NONE;
}

void vmCompleters::encodeCompleterISRForSpreadSheet ( const DBRecord* dbrec )
{
	if ( !mbFullInit )
		return;

	if ( !dbrec->completerUpdated () )
	{
		stringRecord info;
		auto model ( static_cast<QStandardItemModel*>( completersList.at ( PRODUCT_OR_SERVICE )->model () ) );

		const QString compositItemName (	dbrec->isrValue ( ISR_NAME ) + QLatin1String ( " (" ) +
											dbrec->isrValue ( ISR_UNIT ) + QLatin1String ( ") " ) +
											dbrec->isrValue ( ISR_BRAND )
									   );

		for ( uint i ( ISR_NAME ); i <= ISR_DATE; ++i )
			info.fastAppendValue ( dbrec->isrValue ( static_cast<ITEMS_AND_SERVICE_RECORD> ( i ) ) );

		const int row ( inList ( compositItemName, PRODUCT_OR_SERVICE ) );
		completerRecord cr_rec;
		bool ok ( false );

		if ( row == -1 )
		{
			model->appendRow ( new QStandardItem ( compositItemName ) );
			model->setItem ( model->rowCount () - 1, 1, new QStandardItem ( info.toString () ) );
			cr_rec.setAction ( ACTION_ADD );
			setRecValue ( &cr_rec, FLD_CR_PRODUCT_OR_SERVICE_1, compositItemName );
			ok = true;
		}
		else
		{
			const QModelIndex index ( model->index ( row, 1 ) );
			model->setData ( index, info.toString () );
			if ( cr_rec.readRecord  ( FLD_CR_PRODUCT_OR_SERVICE_1, compositItemName, false ) )
			{
				cr_rec.setAction ( ACTION_EDIT );
				ok = true;
			}
		}
		if ( ok )
		{
			setRecValue ( &cr_rec, FLD_CR_PRODUCT_OR_SERVICE_2, info.toString () );
			cr_rec.saveRecord ();
			const_cast<DBRecord*>(dbrec)->setCompleterUpdated ( true );
		}
	}
}
