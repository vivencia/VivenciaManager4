#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QtWidgets/QDialog>

class vmWidget;
class vmListWidget;
class dbListItem;
class vmCheckBox;
class vmComboBox;
class vmLineEdit;
class jobListItem;
class clientListItem;

class QToolButton;

class newProjectDialog : public QDialog
{

public:
	explicit newProjectDialog ( QWidget* parent = nullptr );
	virtual ~newProjectDialog () = default;
	void showDialog ( const QString& clientname, const bool b_allow_other_client = false , const bool b_allow_name_change = true );

	inline const QString& projectPath () const { return mProjectPath; }
	inline const QString& projectID () const { return mProjectID; }
	inline jobListItem* jobItem () const { return mJobItem; }
	inline bool resultAccepted () const { return bresult; }

private:
	void loadJobsList ( const uint clientid );
	void jobTypeItemSelected ( dbListItem* item );
	void txtProjectNameAltered ( const vmWidget* const );
	void btnChooseExistingDir_clicked ();
	void chkUseDefaultName_checked ();
	void btnOK_clicked ();
	void btnCancel_clicked ();

	vmListWidget* lstJobTypes;
	vmLineEdit* txtProjectName;
	vmComboBox* cboClients;
	vmCheckBox* chkUseDefaultName;
	QPushButton* btnOK, *btnCancel;
	QToolButton* btnChooseExistingDir;
	jobListItem* mJobItem;
	QString mProjectID, mProjectPath;
	clientListItem* mClientItem;
	bool bresult;
};

#endif // NEWPROJECTDIALOG_H
