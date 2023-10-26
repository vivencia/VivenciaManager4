#ifndef VMWIDGETS_H
#define VMWIDGETS_H

#include "vmwidget.h"
#include "vmlibs.h"

#include <vmNumbers/vmnumberformats.h>

#include <QtGui/QAction>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>

class simpleCalculator;
class QHBoxLayout;

//------------------------------------------------VM-ACTION-------------------------------------------------
class vmAction final : public QAction, public vmWidget
{

public:
	inline vmAction ( const int index, QObject* parent = nullptr )
		: QAction ( parent ), vmWidget ( WT_ACTION, -1, index ) {}

	inline vmAction ( const int index, const QString& text, QObject* parent = nullptr )
		: QAction ( text, parent ), vmWidget ( WT_ACTION, -1, index ) {}

	inline vmAction ( const int index, const QIcon &icon, const QString& text, QObject* parent = nullptr )
		: QAction ( icon, text, parent ), vmWidget ( WT_ACTION, -1, index ) {}

	virtual ~vmAction () final = default;
	
	inline void setLabel ( const QString& text ) { QAction::setText ( text ); }
};
//------------------------------------------------VM-ACTION-------------------------------------------------

//------------------------------------------------VM-ACTION-LABEL-------------------------------------------------
class vmActionLabel final : public QToolButton, public vmWidget
{

public:
	explicit vmActionLabel ( QWidget* parent = nullptr );
	explicit vmActionLabel ( const QString& text, QWidget* parent = nullptr );
	explicit vmActionLabel ( vmAction* action, QWidget* parent = nullptr );

	virtual ~vmActionLabel () final = default;

	inline QLatin1String qtClassName () const override { return QLatin1String ( "QToolButton" ); }
	const QString defaultStyleSheet () const override;
	const QString alternateStyleSheet () const override;

	inline void setText ( const QString& text, const bool = false ) override { QToolButton::setText ( text ); }
	inline QString text () const override { return QToolButton::text (); }

	void setCallbackForLabelActivated ( const std::function<void ()>& func );
	// Programmatically call the callback generally called via user input
	inline void callLabelActivatedFunc () { if ( labelActivated_func ) labelActivated_func (); }
	QSize sizeHint () const override;
	QSize minimumSizeHint () const override;

protected:
	void init ( const bool b_action = true );

private:
	std::function<void ()> labelActivated_func;
};
//------------------------------------------------VM-ACTION-LABEL-------------------------------------------------

//------------------------------------------------VM-DATE-EDIT------------------------------------------------
class pvmDateEdit;
class vmDateEdit : public QWidget, public vmWidget
{

friend class pvmDateEdit;

public:
	vmDateEdit ( QWidget* parent = nullptr );
	virtual ~vmDateEdit () override;

	static void appStartingProcedures ();
	static void appExitingProcedures ();

	void setID ( const int id );
	
	inline QLatin1String qtClassName () const override { return QLatin1String ( "QDateEdit" ); }
	const QString defaultStyleSheet () const override;

	void setDate ( const vmNumber& date, const bool b_notify = false );
	const QDate date () const;
	inline void setText ( const QString& text, const bool b_notify = false ) override {
		setDate ( vmNumber ( text, VMNT_DATE ), b_notify );
	}
	inline QString text () const override { return vmNumber ( date () ).toDate ( vmNumber::VDF_HUMAN_DATE ); }

	void setEditable ( const bool editable ) override;
	void datesButtonMenuRequested ();

	static void execDateButtonsMenu ( const vmAction* const action, pvmDateEdit* dte );
	static void updateRecentUsedDates ( const vmNumber& date );
	
	inline QMenu* standardContextMenu () const override;
	void setTabOrder ( QWidget* formOwner, QWidget* prevWidget, QWidget* nextWidget );

	void setOwnerItemToDateControl ( vmTableItem* const item );

private:
	pvmDateEdit* mDateEdit;
	QToolButton* mButton;
	static QMenu* menuDateButtons;
};
//------------------------------------------------VM-DATE-EDIT------------------------------------------------

//------------------------------------------------VM-TIME-EDIT------------------------------------------------
class vmTimeEdit final : public QTimeEdit, public vmWidget
{

public:
	vmTimeEdit ( QWidget* parent = nullptr );
	virtual ~vmTimeEdit () final = default;

	inline QLatin1String qtClassName () const override { return QLatin1String ( "QTimeEdit" ); }
	const QString defaultStyleSheet () const override;

	void setTime ( const vmNumber& time, const bool b_notify = false );
	inline void setText ( const QString& text, const bool b_notify = false ) override { setTime ( vmNumber ( text, VMNT_TIME ), b_notify ); }
	inline QString text () const override { return vmNumber ( time () ).toTime (); }
	
	inline void setEditable ( const bool editable ) override {
		setReadOnly ( !editable );
		vmWidget::setEditable ( editable );
	}

protected:
	void keyPressEvent ( QKeyEvent* ) override;
	void focusInEvent ( QFocusEvent* ) override;
	void focusOutEvent ( QFocusEvent* ) override;

private:
	QTime mTimeBeforeFocus;
};
//------------------------------------------------VM-TIME-EDIT------------------------------------------------

//------------------------------------------------LINE-EDIT-LINK----------------------------------------------
class vmLineEdit : public QLineEdit, public vmWidget
{

friend class vmLineEditWithButton;
friend class vmComboBox;

public:
	explicit vmLineEdit ( QWidget* parent = nullptr, QWidget* ownerWindow = nullptr );
	virtual ~vmLineEdit () override = default;

	inline QLatin1String qtClassName () const override { return QLatin1String ( "QLineEdit" ); }
	const QString defaultStyleSheet () const override;
	void highlight ( const VMColors vm_color, const QString& str = QString () ) override;

	void setText ( const QString& text, const bool b_notify = false ) override;
	inline QString text () const override { return QLineEdit::text (); }
	void setTrackingEnabled ( const bool b_tracking );

	void setEditable ( const bool editable ) override;
	inline void setCallbackForMouseClicked ( const std::function<void ( const vmLineEdit* const )>& func ) { mouseClicked_func = func; }

	QMenu* standardContextMenu () const override;

	void updateText ( const bool b_notify );
	
	inline const QString& textBeforeChange () const { return mCurrentText; }
	inline void setCurrentText ( const QString& text ) { mCurrentText = text; }
	
protected:
	bool passKeyOntoTable ( const int key ) const;
	void keyPressEvent ( QKeyEvent* ) override;
	void keyReleaseEvent ( QKeyEvent* ) override;
	void mouseMoveEvent ( QMouseEvent* ) override;
	void mouseReleaseEvent ( QMouseEvent* ) override;
	void contextMenuEvent ( QContextMenuEvent* ) override;
	void focusInEvent ( QFocusEvent* ) override;
	void focusOutEvent ( QFocusEvent* ) override;

private:
	bool mCtrlKey;
	bool mCursorChanged;
	bool b_widgetCannotGetFocus;
	bool mbTrack;
	bool mbButtonClicked;
	QString mCurrentText; // Can be used as old value by contentsAltered_func because it's only updated after the call to it. Retrieve it via textBeforeChange ()

	std::function<void ( const vmLineEdit* const )> mouseClicked_func;
};
//------------------------------------------------LINE-EDIT-LINK----------------------------------------------

//------------------------------------------------VM-LINE-EDIT-BUTTON-----------------------------------------------
class vmLineEditWithButton : public QWidget, public vmWidget
{

public:

	enum LINE_EDIT_BUTTON_TYPE { LEBT_NO_BUTTON = 0, LEBT_CALC_BUTTON = 0x20, LEBT_DIALOG_BUTTON_DIR = 0x40,
								LEBT_DIALOG_BUTTON_FILE = 0x80, LEBT_DIALOG_BUTTON_SAVE = 0x100, LEBT_CUSTOM_BUTTOM = 0x200 };

	vmLineEditWithButton ( QWidget* parent = nullptr );
	virtual ~vmLineEditWithButton () override;

	inline QLatin1String qtClassName () const override { return QLatin1String ( "QLineEdit" ); }
	inline const QString defaultStyleSheet () const override { return mLineEdit->defaultStyleSheet () ; }
	inline void highlight ( const VMColors wm_color, const QString& str = QString () ) override { mLineEdit->highlight ( wm_color, str ); }
	void setEditable ( const bool editable ) override;
	inline void setText ( const QString& text, const bool b_notify = false ) override { mLineEdit->setText ( text, b_notify ); }
	inline QString text () const override { return lineControl ()->text (); }

	void setButtonType ( const uint btn_idx, const LINE_EDIT_BUTTON_TYPE type );
	void setButtonIcon ( const uint btn_idx, const QIcon& icon );
	void setButtonTooltip ( const uint btn_idx, const QString& str_tip );
	void setCallbackForButtonClicked ( const uint btn_idx, const std::function<void()>& func );

	inline vmLineEdit* lineControl () const { return mLineEdit; }
	inline bool wasButtonClicked () const { return mLineEdit->mbButtonClicked; }
	inline QMenu* standardContextMenu () const override { return mLineEdit->standardContextMenu (); }
	
private:
	vmLineEdit* mLineEdit;
	simpleCalculator* mCalc;

	struct buttons_st
	{
		QToolButton* mButton;
		LINE_EDIT_BUTTON_TYPE mBtnType;
		std::function<void()> buttonClicked_func;
		uint idx;

		buttons_st () : mButton ( nullptr ), mBtnType ( LEBT_NO_BUTTON ), buttonClicked_func ( nullptr ), idx ( 0 ) {}
	};
	pointersList<buttons_st*> buttonsList;
	QHBoxLayout* mainLayout;

	void execButtonAction ( const uint i );
	buttons_st *addButton ( const uint i );
};
//------------------------------------------------VM-LINE-EDIT-BUTTON------------------------------------------------

//------------------------------------------------VM-COMBO-BOX------------------------------------------------
class vmComboBox final : public QComboBox, public vmWidget
{

public:
	vmComboBox ( QWidget* parent = nullptr );
	virtual ~vmComboBox () final = default;

	inline void setID ( const int id )
	{
		mLineEdit->setID ( id );
		vmWidget::setID ( id );
	}
	
	inline QLatin1String qtClassName () const override { return QLatin1String ( "QComboBox" ); }
	const QString defaultStyleSheet () const override;
	void highlight ( const VMColors vm_color, const QString& str = QString () ) override;

	void setText ( const QString& text, const bool b_notify = false ) override;
	inline QString text () const override { return mLineEdit->QLineEdit::text (); }

	void setIgnoreChanges ( const bool b_ignore );
	inline bool isIgnoringChanges () const { return mbIgnoreChanges; }
	void setEditable ( const bool editable ) override;

	/* Will check for existance of text and only append it if there is no match
	 * For additions that we know there is no way of a match, call QComboBox::addItem dicrectly,
	 * for it will be faster
	 */
	void insertItem ( const QString& text, const int idx = -1, const bool b_check = true );
	int insertItemSorted ( const QString& text, const bool b_insert_if_exists = false );

	// The receiver should retrieve the line edit's parent to find the vmComboBox owning the sender of the signal
	inline void setCallbackForContentsAltered ( const std::function<void ( const vmWidget* const )>& func ) override
	{
		vmWidget::setCallbackForContentsAltered ( func );
		mLineEdit->setCallbackForContentsAltered ( func );
	}

	inline void setCallbackForActivated ( const std::function<void ( const int )>& func ) { activated_func = func; }
	inline void setCallbackForIndexChanged ( const std::function<void ( const int )>& func ) { indexChanged_func = func; }
	inline void setCallbackForEnterKeyPressed ( const std::function<void ()>& func ) { keyEnter_func = func; }
	inline void setCallbackForEscKeyPressed ( const std::function<void ()>& func ) { keyEsc_func = func; }

	inline vmLineEdit* editor () const { return mLineEdit; }

protected:
	void focusOutEvent ( QFocusEvent* ) override;
	void keyPressEvent ( QKeyEvent* ) override;
	void focusInEvent ( QFocusEvent* ) override;
	void wheelEvent ( QWheelEvent* ) override;

private:
	bool mbIgnoreChanges;
	bool mbKeyEnterPressedOnce;
	bool mbKeyEscPressedOnce;

	vmLineEdit* mLineEdit;

	std::function<void ( const vmLineEdit* const )> textAltered_func;
	std::function<void ( const int )> indexChanged_func;
	std::function<void ( const int )> activated_func;
	std::function<void ()> keyEnter_func;
	std::function<void ()> keyEsc_func;

	void currentIndexChanged_slot ( const int idx );
};
//------------------------------------------------VM-COMBO-BOX------------------------------------------------

//------------------------------------------------VM-CHECK-BOX------------------------------------------------
class vmCheckBox final : public QCheckBox, public vmWidget
{

public:
	explicit vmCheckBox ( QWidget* parent = nullptr );
	vmCheckBox ( const QString& text, QWidget* parent = nullptr );
	virtual ~vmCheckBox () final = default;

	inline QLatin1String qtClassName () const override { return QLatin1String ( "QCheckBox" ); }
	const QString defaultStyleSheet () const override;

	inline void setChecked ( const bool checked, const bool b_notify = false )
	{
		setText ( checked ? CHR_ONE : CHR_ZERO, b_notify );
	}
	
	void setText ( const QString& text, const bool b_notify = false ) override;

	inline QString text () const override { return isChecked () ? CHR_ONE : CHR_ZERO; }
	void setEditable ( const bool editable ) override;

	// Qt's uic calls setText, which render the controle labelless because we use the same function name, which trumps the base's name.
	// Every controler used in the source must call setLabel if created by Qt's UI tools. Otherwise, set the label in the constructor
	inline void setLabel ( const QString& label ) { QCheckBox::setText ( label ); }

	void focusInEvent ( QFocusEvent* e ) override;
};
//------------------------------------------------VM-CHECK-BOX------------------------------------------------

#endif // VMWIDGETS_H
