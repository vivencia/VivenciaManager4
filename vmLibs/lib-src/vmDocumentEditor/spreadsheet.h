#ifndef QUICKPROJECTUI_H
#define QUICKPROJECTUI_H

#include "vmWidgets/vmtablewidget.h"

#include <dbRecords/quickproject.h>

#include <QtCore/QString>
#include <QtWidgets/QDialog>

class documentEditor;
class Job;
class vmCompleters;

class QPushButton;
class QVBoxLayout;

class spreadSheetEditor : public QDialog
{

public:

	enum SPREADSHEET_FIELDS {
		SPREADSHEET_ITEM = 0,
		SPREADSHEET_SELL_QUANTITY = 1,
		SPREADSHEET_SELL_UNIT_PRICE = 2,
		SPREADSHEET_SELL_TOTAL_PRICE = 3,
		SPREADSHEET_PURCHASE_QUANTITY = 4,
		SPREADSHEET_PURCHASE_UNIT_PRICE = 5,
		SPREADSHEET_PURCHASE_TOTAL_PRICE = 6,
		SPREADSHEET_RESULT = 7
	};

	explicit spreadSheetEditor ( documentEditor* mdiParent = nullptr );
	virtual ~spreadSheetEditor () override;

	void setupUI ();
	void prepareToShow ( const Job* const job = nullptr );
	bool loadData ( const QString& jobid, const bool force = false );
	QString getJobIDFromQPString ( const QString& qpIDstr ) const;

	inline void setCallbackForDialogClosed ( const std::function<void ()>& func ) { funcClosed = func; }
	inline vmTableWidget* table () const { return m_table; }
	inline QString totalPrice () const {
			return m_table->sheetItem ( static_cast<uint>(m_table->totalsRow ()), FLD_QP_SELL_TOTALPRICE - 2 )->text (); }
	inline QString qpString () const { return m_qpString; }

	void getHeadersText ( spreadRow* row ) const;

	inline bool qpChanged () const { return mbQPChanged; }
	//void temporarilyHide ();
	//void showAgain ();

	inline documentEditor* parentEditor () const {
		return m_parentEditor;
	}

	void setCompleterManager ( vmCompleters* const completer );
	inline vmCompleters* completerManager () { return mCompleterManager; }

private:
	void cellModified ( const vmTableItem* const item );
	void completeItem ( const QModelIndex& );
	void editTable ( const bool checked );
	void cancelEdit ();
	void closeClicked ();
	bool rowRemoved ( const uint row );

	void enableControls ( const bool enable );

	documentEditor* m_parentEditor;
	QVBoxLayout* mLayoutMain;
	vmTableWidget* m_table;
	vmCompleters* mCompleterManager;
	QString m_qpString;
	bool mbQPChanged;

	quickProject* qp_rec;
	Job* mJob;

	QPushButton* btnEditTable;
	QPushButton* btnCancel;
	QPushButton* btnClose;

	std::function<void ()> funcClosed;
};
#endif // QUICKPROJECTUI_H
