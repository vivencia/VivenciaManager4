#ifndef SUPPLIERSDLG_H
#define SUPPLIERSDLG_H

#include "vmlist.h"

#include <QDialog>

class vmWidget;
class vmLineEdit;
class vmComboBox;
class supplierRecord;
class contactsManagerWidget;
class dbListItem;

class QPushButton;

class suppliersDlg : public QDialog
{

friend class VivenciaDB;

public:
	explicit suppliersDlg ();
	virtual ~suppliersDlg () override;

	void displaySupplier ( const QString& supName, const bool b_showdlg );
	static void supplierInfo ( const QString& name, QString& info );
	void hideDialog ();
	void showSearchResult ( dbListItem* item, const bool bshow );

private:
	supplierRecord* supRec;

	void saveWidget ( vmWidget* widget, const int id );
	void setupUI ();
	void retrieveInfo ( const QString& name );
	void controlForms ();
	void keyPressedSelector ( const QKeyEvent* ke );
	void clearForms ();
	void txtSupplier_textAltered ( const vmWidget* const sender );
	void contactsAdd ( const QString& info, const vmWidget* const sender );
	void contactsDel ( const int idx, const vmWidget* const sender );
	void btnInsertClicked ( const bool checked );
	void btnEditClicked ( const bool checked );
	void btnCancelClicked ();
	void btnRemoveClicked ();
	void btnCopyToEditorClicked ();
	void btnCopyAllToEditorClicked ();

	pointersList<vmWidget*> widgetList;
	vmLineEdit* txtSupID;
	vmLineEdit* txtSupName;
	vmLineEdit* txtSupStreet;
	vmLineEdit* txtSupNbr;
	vmLineEdit* txtSupDistrict;
	vmLineEdit* txtSupCity;
	vmComboBox* cboSupPhones;
	vmComboBox* cboSupEmail;
	contactsManagerWidget* contactsPhones;
	contactsManagerWidget* contactsEmails;
	QPushButton* btnEdit, *btnInsert, *btnCancel, *btnRemove;
	QPushButton* btnCopyToEditor, *btnCopyAllToEditor;
};

#endif // SUPPLIERSDLG_H
