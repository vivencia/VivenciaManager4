#include "vmwidget.h"
#include "vmtableitem.h"

#include <vmStringRecord/stringrecord.h>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QDesktopWidget>

#define TITLE_BAR_HEIGHT qApp->style ()->pixelMetric ( QStyle::PM_TitleBarHeight )
QString vmWidget::strColor1;
QString vmWidget::strColor2;

void vmwidget_swap ( vmWidget& widget1, vmWidget& widget2 )
{
	using std::swap;
	
	swap ( widget1.keypressed_func, widget2.keypressed_func );
	swap ( widget1.contentsAltered_func, widget2.contentsAltered_func );
	swap ( widget1.m_type, widget2.m_type );
	swap ( widget1.m_subtype, widget2.m_subtype );
	swap ( widget1.m_id, widget2.m_id );
	swap ( widget1.m_completerid, widget2.m_completerid );
	swap ( widget1.mb_editable, widget2.mb_editable );
	swap ( widget1.m_data, widget2.m_data );
	swap ( widget1.mWidgetPtr, widget2.mWidgetPtr );
	swap ( widget1.mParent, widget2.mParent );
	swap ( widget1.m_sheetItem, widget2.m_sheetItem );
	swap ( widget1.m_LayoutUtilities, widget2.m_LayoutUtilities );
	pointersList<QWidget*>::vmList_swap ( widget1.m_UtilityWidgetsList, widget2.m_UtilityWidgetsList );
	swap ( widget1.mTextType, widget2.mTextType );
}

vmWidget::vmWidget ()
	: keypressed_func ( nullptr ), contentsAltered_func ( nullptr ),
	  m_type ( WT_WIDGET_UNKNOWN ), m_subtype ( WT_WIDGET_UNKNOWN ), m_id ( -1 ), m_completerid ( -1 ),
	  mb_editable ( false ), mWidgetPtr ( nullptr ), mParent ( nullptr ), m_sheetItem ( nullptr ),
	  m_LayoutUtilities ( nullptr ), m_UtilityWidgetsList ( 2 ), mTextType ( TT_TEXT )
{}

vmWidget::vmWidget ( const int type, const int subtype, const int id )
	: vmWidget ()
{
	m_type = type;
	m_subtype = subtype;
	m_id = id;
}

vmWidget::~vmWidget () {}

static const QString strWidgetPrefix ( QStringLiteral ( "##%%" ) );

QString vmWidget::widgetToString () const
{
	stringRecord strWidget;
	strWidget.fastAppendValue ( strWidgetPrefix );
	strWidget.fastAppendValue ( QString::number ( type () ) );
	strWidget.fastAppendValue ( QString::number ( subType () ) );
	strWidget.fastAppendValue ( QString::number ( id () ) );
	return strWidget.toString ();
}

void vmWidget::highlight ( bool b_highlight )
{
	toQWidget ()->setStyleSheet ( b_highlight ? alternateStyleSheet () : defaultStyleSheet () );
}

void vmWidget::highlight ( const VMColors vm_color, const QString& )
{
	toQWidget ()->setStyleSheet ( vm_color == vmDefault_Color ? defaultStyleSheet () :
			qtClassName () + QLatin1String ( " { background-color: rgb(" ) + colorsStr[vmColorIndex (vm_color)] + QLatin1String ( " ) }" ) );
}

vmWidget* vmWidget::stringToWidget ( const QString& /*str_widget*/ )
{
	vmWidget* widget ( nullptr );
	/*vmWidget::vmWidgetStructure widget_st ( stringToWidgetSt ( str_widget ) );
	if ( widget_st.type > WT_WIDGET_UNKNOWN ) {
		widget = new vmWidget ( 0 );
		widget->m_type = widget_st.type;
		widget->m_subtype = widget_st.subtype;
		widget->m_id = widget_st.id;
	}*/
	return widget;
}

void vmWidget::setFontAttributes ( const bool italic, const bool bold )
{
	if ( toQWidget () )
	{
		QFont fnt ( toQWidget ()->font () );
		fnt.setItalic ( italic );
		fnt.setBold ( bold );
		toQWidget ()->setFont ( fnt );
	}
}

void vmWidget::setTextType ( const TEXT_TYPE t_type )
{
	if ( t_type != mTextType )
	{
		mTextType = t_type;
		QWidget* widget ( toQWidget () );
		if ( widget )
		{
			vmLineEdit* line ( nullptr );
			switch ( type () )
			{
				case WT_LINEEDIT:
					line = static_cast<vmLineEdit*> ( widget );
				break;
				case WT_LINEEDIT_WITH_BUTTON:
					line = static_cast<vmLineEditWithButton*> ( widget )->lineControl ();
				break;
				case WT_COMBO:
					line = static_cast<vmComboBox*> ( widget )->editor ();
				break;
				case WT_TEXTEDIT:
				break;
				default:
				return;
			}

			QValidator* qval ( nullptr );
			Qt::InputMethodHints imh ( Qt::ImhNone );
			QString input_mask;
			switch ( t_type )
			{
				case TT_PHONE:
					input_mask = QStringLiteral ( "(DD) dD990-9999" );
					imh = Qt::ImhDigitsOnly;
				break;
				case TT_ZIPCODE:
					input_mask = QStringLiteral ( "D9999-999" );
					imh = Qt::ImhFormattedNumbersOnly;
				break;
				case TT_PRICE:
					qval = new QDoubleValidator ( -999.999, 999.999, 2 );
					imh = Qt::ImhFormattedNumbersOnly;
				break;
				case TT_INTEGER:
					qval = new QIntValidator ( INT_MIN, INT_MAX );
					imh = Qt::ImhDigitsOnly;
				break;
				case TT_DOUBLE:
					qval = new QDoubleValidator ( 0.0, 9.999, 2 );
					imh = Qt::ImhFormattedNumbersOnly;
				break;
				case TT_UPPERCASE:
				break;
				case TT_TEXT:
				break;
			}
			if ( line )
			{
				line->mTextType = this->mTextType;
				line->setValidator ( qval );
				line->setInputMask ( input_mask );
			}
			widget->setInputMethodHints ( imh );
		}
	}
}

/* Compost widgets have a problem when setting/getting properties. Depending from whence they are called sub widgets might be referenced
 * by the caller and those sub widgets might not have gotten all the properties from the parent widget. One TODO feature is to have a
 * list of all sub widgets and recursively call them setting a property when said property is set for the parent widget
 * */
void vmWidget::setOwnerItem ( vmTableItem* const item )
{
	 m_sheetItem = item;
	 if ( m_type == WT_LINEEDIT_WITH_BUTTON )
		 static_cast<vmLineEditWithButton*>( toQWidget () )->lineControl ()->setOwnerItem ( item );
	 else if ( m_type == WT_DATEEDIT )
		static_cast<vmDateEdit*> ( toQWidget() )->setOwnerItemToDateControl ( item );
}

bool vmWidget::toggleUtilityPanel ( const int widget_idx )
{
	if ( utilitiesLayout () != nullptr )
	{
		QWidget* panel ( m_UtilityWidgetsList.at ( widget_idx ) );
		if ( panel != nullptr )
		{
			if ( panel->isVisible () )
			{
				utilitiesLayout ()->removeWidget ( panel );
				panel->setVisible ( false );
			}
			else
			{
				utilitiesLayout ()->insertWidget ( 0, panel, 1 );
				panel->setVisible ( true );
			}
			return true;
		}
	}
	return false;
}

int vmWidget::vmColorIndex ( const VMColors vmcolor )
{
	int idx ( -1 );
	switch ( vmcolor )
	{
		case vmGray: idx = 0; break;
		case vmRed: idx = 1; break;
		case vmYellow: idx = 2; break;
		case vmGreen: idx = 3; break;
		case vmBlue: idx = 4; break;
		case vmWhite: idx = 5; break;
		default: break;
	}
	return idx;
}

QPoint vmWidget::getGlobalWidgetPosition ( const QWidget* widget, QWidget* appMainWindow )
{
	QWidget* refWidget ( nullptr );
	if ( appMainWindow && appMainWindow->isAncestorOf ( widget ) )
		refWidget = appMainWindow;
	else
	{
		refWidget = widget->parentWidget ();
		if ( refWidget == nullptr )
			refWidget = qApp->desktop ();
	}
	QPoint wpos;
	const QPoint posInRefWidget ( widget->mapTo ( refWidget, widget->pos () ) );
	wpos.setX ( refWidget->pos ().x () + posInRefWidget.x () - widget->pos ().x () );
	wpos.setY ( refWidget->pos ().y () + posInRefWidget.y () - widget->pos ().y () + widget->height () + TITLE_BAR_HEIGHT );
	return wpos;
}

void vmWidget::execMenuWithinWidget ( QMenu* menu, const QWidget* widget,
									  const QPoint& mouse_pos, QWidget *appMainWindow )
{
	QPoint menuPos ( getGlobalWidgetPosition ( widget, appMainWindow ) );
	menuPos.setX ( menuPos.x () + mouse_pos.x () );
	menuPos.setY ( menuPos.y () + mouse_pos.y () + TITLE_BAR_HEIGHT );
	menu->exec ( menuPos );
}
