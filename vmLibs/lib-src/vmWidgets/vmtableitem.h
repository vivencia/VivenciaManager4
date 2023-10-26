#ifndef VMTABLEITEM_H
#define VMTABLEITEM_H

#include "vmwidget.h"
#include "vmwidgets.h"

#include <QtCore/QVariant>
#include <QtCore/QString>
#include <QtWidgets/QTableWidgetItem>

class vmTableWidget;

class vmTableItem : public QTableWidgetItem, public vmWidget
{

public:
	friend void table_item_swap ( vmTableItem& t_item1, vmTableItem& t_item2 );

	vmTableItem ();

	explicit vmTableItem ( const PREDEFINED_WIDGET_TYPES wtype,
						   const vmLineEdit::TEXT_TYPE ttype,
						   const QString& text, const vmTableWidget* table );

	vmTableItem ( const QString& text, const vmTableWidget* table ); // Simple item. No widget
	inline vmTableItem ( const vmTableItem& t_item ) : vmTableItem () { copy ( t_item ); }

	inline vmTableItem ( vmTableItem&& other ) : vmTableItem ()
	{
		table_item_swap ( *this, other );
	}

	inline const vmTableItem& operator= ( vmTableItem t_item )
	{
		table_item_swap ( *this, t_item );
		return *this;
	}

	virtual ~vmTableItem () override;

	inline void setTable ( vmTableWidget* table ) { m_table = table; }
	inline vmTableWidget* table () const { return m_table; }

	const QString defaultStyleSheet () const override;
	void setEditable ( const bool editable ) override;

	inline vmWidget* widget () const { return m_widget; }
	inline void setWidget ( vmWidget* widget ) { m_widget = widget; }

	void highlight ( const VMColors color, const QString& str = QString () ) override;

	QVariant data ( const int role ) const override;

	inline QString text () const override { return mCache.toString (); }
	inline QString prevText () const { return mprev_datacache.toString (); }
	//inline void setOriginalText ( const QString& text ) { mBackupData_cache = text; }
	inline QString originalText () const { return mBackupData_cache.toString (); }
	inline void setCellIsAltered ( const bool altered ) { mb_CellAltered = altered; }
	inline bool cellIsAltered () const { return mb_CellAltered; }

	inline void syncOriginalTextWithCurrent () {
		mBackupData_cache = mCache;
		mb_CellAltered = false;
	}

	inline void setMemoryTextOnly ( const QString& text ) { mCache = text; }
	void setText ( const QString& text, const bool b_notify, const bool b_from_cell_itself = false, const bool b_formulaResult = false ) override;
	inline void setTextToDefault ( const bool force_notify = false ) { setText ( mDefaultValue, false, force_notify ); }

	void setDate ( const vmNumber& date );
	vmNumber date ( const bool bCurText = true ) const;
	vmNumber number ( const bool bCurText = true ) const;

	inline bool hasFormula () const { return mb_hasFormula; }
	inline bool formulaOverride () const { return mb_formulaOverride; }
	inline void setFormulaOverride ( const bool b_override ) { mb_formulaOverride = b_override; }
	inline const QString& formula () const { return mStr_Formula; }
	inline const QString& formulaTemplate () const { return mStr_FormulaTemplate; }

	void targetsFromFormula ();
	void setFormula ( const QString& formula_template, const QString& firstValue, const QString& secondValue = QString () );
	bool setCustomFormula ( const QString& strFormula );
	void computeFormula ();

	inline void setDefaultValue ( const QString& text ) { mDefaultValue = text; }
	inline const QString& defaultValue () const { return mDefaultValue; }

	inline void setWidgetType ( const PREDEFINED_WIDGET_TYPES wtype ) { m_wtype = wtype; }
	inline PREDEFINED_WIDGET_TYPES widgetType () const { return m_wtype; }

	inline void setButtonType ( const vmLineEditWithButton::LINE_EDIT_BUTTON_TYPE btype ) { m_btype = btype; }
	inline vmLineEditWithButton::LINE_EDIT_BUTTON_TYPE buttonType () const { return m_btype; }
	
protected:
	void copy ( const vmTableItem& src_item );
	
private:
	PREDEFINED_WIDGET_TYPES m_wtype;
	vmLineEditWithButton::LINE_EDIT_BUTTON_TYPE m_btype;

	bool mb_hasFormula, mb_formulaOverride, mb_customFormula;
	bool mb_CellAltered;

	QString mStr_Formula, mStr_FormulaTemplate, mStrOp, mDefaultValue;
	QVariant mCache, mprev_datacache, mBackupData_cache;

	vmTableWidget* m_table;
	pointersList<vmTableItem*> m_targets;
	vmWidget* m_widget;
};

#endif // VMTABLEITEM_H
