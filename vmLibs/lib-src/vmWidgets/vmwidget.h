#ifndef VMWIDGET_H
#define VMWIDGET_H

#include "vmlist.h"

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtWidgets/QWidget>

#include <functional>

enum VMColors
{
	vmDefault_Color = -2,
	vmDefault_Color_Alternate = -1,
	vmGray = Qt::gray,
	vmRed = Qt::red,
	vmYellow = Qt::yellow,
	vmGreen = Qt::green,
	vmBlue = Qt::blue,
	vmWhite = Qt::white,
	vmCyan = Qt::cyan
};

constexpr const QLatin1String colorsStr[6] =
{
	QLatin1String ( "220, 220, 220", 13 ),		// gray		- 0
	QLatin1String ( "255, 0, 0", 9 ),			// red		- 1
	QLatin1String ( "255, 255, 0", 11 ),		// yellow	- 2
	QLatin1String ( "0, 255, 0", 9 ),			// green	- 3
	QLatin1String ( "0, 0, 255", 9 ),			// blue		- 4
	QLatin1String ( "255, 255, 255", 13 )		// white	- 5
};

enum PREDEFINED_WIDGET_TYPES
{
	WT_WIDGET_ERROR = -1, WT_WIDGET_UNKNOWN = 0,
	WT_LINEEDIT = 2<<0, WT_TEXTEDIT = 2<<1, WT_LABEL = 2<<2, WT_COMBO = 2<<3, WT_DATEEDIT = 2<<4,
	WT_TIMEEDIT = 2<<5, WT_TABLE = 2<<6, WT_LED = 2<<7, WT_LISTITEM = 2<<8, WT_ACTION = 2<<9,
	WT_LISTWIDGET = 2<<10, WT_BUTTON = 2<<11, WT_CHECKBOX = 2<<12, WT_TABLE_ITEM = 2<<13,
	WT_LINEEDIT_WITH_BUTTON = 2<<14, WT_QWIDGET = 2<<15

};

class vmTableItem;

class QKeyEvent;
class QVBoxLayout;
class QMenu;

class vmWidget
{

friend class vmTableWidget;

public:
	
	friend void vmwidget_swap ( vmWidget& widget1, vmWidget& widget2 );
	
	enum TEXT_TYPE { TT_TEXT = 0x1, TT_UPPERCASE = 0x2, TT_PHONE = 0x4, TT_ZIPCODE = 0x8,
					 TT_PRICE = 0x10, TT_INTEGER = 0x20, TT_DOUBLE = 0x40
				   };

	vmWidget ();
	vmWidget ( const vmWidget& other );
	vmWidget ( const int type, const int subtype = -1, const int id = -1 );
	
	inline vmWidget ( vmWidget&& other ) : vmWidget ()
	{
		vmwidget_swap ( *this, other );
	}
	
	inline const vmWidget& operator= ( vmWidget widget )
	{
		vmwidget_swap ( *this, widget );
		return *this;
	}
	
	virtual ~vmWidget ();

	inline void setVmParent ( vmWidget* const parent ) { mParent = parent; }
	inline vmWidget* vmParent () const { return mParent; }

	inline void setWidgetPtr ( QWidget* ptr ) { mWidgetPtr = ptr; }
	inline QWidget* toQWidget () const { return mWidgetPtr; }
	inline virtual void setText ( const QString& /*text*/, const bool /*b_notify*/ ) {}
	inline virtual void setText ( const QString& /*text*/, const bool /*b_notify*/, const bool /*extra_param_1*/ ) {}
	inline virtual void setText ( const QString& /*text*/, const bool /*b_notify*/, const bool /*extra_param_1*/, const bool /*extra_param_2*/ ) {}
	inline virtual QString text () const { return QString (); }
	inline virtual QLatin1String qtClassName () const { return QLatin1String (); }
	inline virtual const QString defaultStyleSheet () const { return QString (); }
	inline virtual const QString alternateStyleSheet () const { return QString (); }
	virtual QString widgetToString () const;
	virtual void highlight ( bool b_highlight );
	virtual void highlight ( const VMColors vm_color, const QString& = QString () );
	inline virtual QMenu* standardContextMenu () const { return nullptr; }

	// Can be implemented downstream to produce specific widgets with their particularities.
	// On success, a new object is created on the heap and should be managed by the caller.
	// TODO: consider using heapManager to manage the created object.
	virtual vmWidget* stringToWidget ( const QString& str_widget );

	inline virtual void setEditable ( const bool editable )
	{
		setFontAttributes ( mb_editable = editable );
	}

	inline bool isEditable () const { return mb_editable; }

	void setFontAttributes ( const bool italic, const bool bold = false );
	static inline void changeThemeColors ( const QString& color1, const QString& color2 )
	{
		vmWidget::strColor1 = color1; vmWidget::strColor2 = color2;
	}
	inline const QString& themeColor1 () const { return vmWidget::strColor1; }
	inline const QString& themeColor2 () const { return vmWidget::strColor2; }

	void setTextType ( const TEXT_TYPE t_type );
	inline TEXT_TYPE textType () const { return mTextType; }
	inline int type () const { return m_type; }
	inline int subType () const { return m_subtype; }
	inline void setSubType ( const int subtype ) { m_subtype = subtype; }
	inline int id () const { return m_id; }
	inline void setID ( const int id ) { m_id = id; }

	inline void setCallbackForRelevantKeyPressed
			( const std::function<void ( const QKeyEvent* const, const vmWidget* const )>& func ) { keypressed_func = func; }
	inline virtual void setCallbackForContentsAltered ( const std::function<void ( const vmWidget* const )>& func ) { contentsAltered_func = func; }

	inline const vmTableItem* ownerItem () const { return m_sheetItem; }
	void setOwnerItem ( vmTableItem* const item );

	/* When data is a vmNumber, its internal data structure, privateData, must have the number member created and operational.
	 * For that, call that number's copyNumber method ( a member of vmBaseType ) with itself as argument before calling this method
	 * Because syncing is not automatic, any change to the derived class must be followed by a call to copyNumber if the number is to be used again
	 */
	inline void setInternalData ( const QVariant& data ) { m_data = data; }
	inline const QVariant& internalData () const { return m_data; }

	inline void setCompleterType ( const int completer_type ) { m_completerid = completer_type; }
	inline int completerType () const { return m_completerid; }

	inline void setUtilitiesPlaceLayout ( QVBoxLayout* layoutUtilities ) { m_LayoutUtilities = layoutUtilities; }
	inline QVBoxLayout* utilitiesLayout () { return m_LayoutUtilities; }
	inline int addUtilityPanel ( QWidget* panel ) { return m_UtilityWidgetsList.append ( panel ); }
	inline QWidget* utilityPanel ( const int widget_idx ) const { return m_UtilityWidgetsList.at ( widget_idx ); }
	bool toggleUtilityPanel ( const int widget_idx );

	static int vmColorIndex ( const VMColors vmcolor );
	
	static QPoint getGlobalWidgetPosition ( const QWidget* widget, QWidget* appMainWindow = nullptr );
	static void execMenuWithinWidget ( QMenu* menu, const QWidget* widget, const QPoint& mouse_pos, QWidget* appMainWindow = nullptr );

protected:
	std::function<void ( const QKeyEvent* const ke, const vmWidget* const widget )> keypressed_func;
	std::function<void ( const vmWidget* const )> contentsAltered_func;

private:
	int m_type;
	int m_subtype;
	int m_id;
	int m_completerid;
	bool mb_editable;
	QVariant m_data;

	QWidget* mWidgetPtr;
	vmWidget* mParent;
	vmTableItem* m_sheetItem;
	QVBoxLayout* m_LayoutUtilities;
	pointersList<QWidget*> m_UtilityWidgetsList;
	TEXT_TYPE mTextType;
	static QString strColor1, strColor2;
};

#endif // VMWIDGET_H
