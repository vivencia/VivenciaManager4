#ifndef COMPANYPURCHASESUI_H
#define COMPANYPURCHASESUI_H

#include "vmlist.h"
#include "system_init.h"
#include <vmStringRecord/stringrecord.h>

#include <QtWidgets/QDialog>

class companyPurchases;
class vmTableWidget;
class vmTableItem;
class vmWidget;
class vmLineEdit;
class vmDateEdit;
class vmListItem;

namespace Ui
{
class companyPurchasesUI;
}

class companyPurchasesUI : public QDialog
{

public:
	explicit companyPurchasesUI ( QWidget* parent = nullptr );
	virtual ~companyPurchasesUI ();
	void showSearchResult ( vmListItem* item, const bool bshow );
	void showSearchResult_internal ( const bool bshow );

private:
	void saveWidget ( vmWidget* widget, const int id );
	void setupUI ();
	void fillForms ();
	void controlForms ();
	void saveInfo ();

	void searchCancel ();
	bool searchFirst ();
	bool searchPrev ();
	bool searchNext ();
	bool searchLast ();

	void btnCPFirst_clicked ();
	void btnCPLast_clicked ();
	void btnCPPrev_clicked ();
	void btnCPNext_clicked ();
	void btnCPSearch_clicked ( const bool checked );
	void btnCPAdd_clicked ( const bool checked );
	void btnCPEdit_clicked ( const bool checked );
	void btnCPRemove_clicked ();

	void setTotalPriceAsItChanges ( const vmTableItem* const item );
	void setPayValueAsItChanges ( const vmTableItem* const item );
	void tableItemsCellChanged ( const vmTableItem* const item );
	bool tableRowRemoved ( const uint row );
	void tablePaymentsCellChanged ( const vmTableItem* const item );

	void relevantKeyPressed ( const QKeyEvent* ke );
	void txtCP_textAltered ( const vmWidget* const sender );
	void dteCP_dateAltered ( const vmWidget* const sender );
	void btnCPShowSupplier_clicked ( const bool checked );
	void txtCPSearch_textAltered ( const QString& text );

	static companyPurchasesUI* s_instance;
	friend companyPurchasesUI* COMPANY_PURCHASES ();
	friend void deleteCompanyPurchasesUiInstance ();

	Ui::companyPurchasesUI* ui;
	QString mSearchTerm;
	bool mbSearchIsOn;
	companyPurchases* cp_rec;
	pointersList<vmWidget*> widgetList;
	podList<uint> mFoundFields;
	stringTable mTableContents;
};

inline companyPurchasesUI* COMPANY_PURCHASES ()
{
	if ( !companyPurchasesUI::s_instance )
		companyPurchasesUI::s_instance = new companyPurchasesUI;
	return companyPurchasesUI::s_instance;
}

#endif // COMPANYPURCHASESUI_H
