#ifndef MACHINESDLG_H
#define MACHINESDLG_H

#include <QtWidgets/QDialog>

class vmWidget;
class machinesRecord;
class vmActionGroup;
class stringRecord;
class vmTableWidget;
class vmComboBox;
class vmLineEdit;
class vmDateEdit;
class vmTimeEdit;

class QToolButton;
class QPushButton;

class machinesDlg : public QDialog
{

public:
	explicit machinesDlg ( QWidget* parent = nullptr );
	virtual ~machinesDlg () override;
	void showJobMachineUse ( const QString& jobid );

private:
	machinesRecord* mMacRec;
	vmComboBox* cboMachines, *cboBrand, *cboType, *cboEvents;
	vmLineEdit* txtJob; //, *txtMachineTotalTime;
	vmDateEdit* dteEventDate;
	vmTimeEdit* timeEventTime;
	QToolButton* btnAddEvent, *btnDelEvent;
	QToolButton* btnSelectJob;
	QPushButton* btnEdit, *btnCancel, *btnSave, *btnClose;
	vmTableWidget* tableMachineHistory, *tableJobEvents;
	vmActionGroup* grpHistory, *grpJobMachineEvents;

	void setupUI ();
	void setupConnections ();
	void initTableMachineEvents ();
	void inittableMachineHistory ();
	void canClose ();
	void controlForms ();
	void clearForms ();
	void fillComboBoxes ();
	int loadJobMachineEventsTable ( const QString& jobid );
	QString decodeStringRecord ( const uint field );
	bool loadData ( const int id );
	void getSelectedJobID ( const uint jobid );

	void cboMachines_IndexChanged ( const int idx );
	void dataAltered ( const vmWidget* const sender );
	void btnAddEvent_clicked ();
	void btnDelEvent_clicked ();
	void btnSelectJob_clicked ();
	void btnEdit_clicked ();
	void btnCancel_clicked ();
	void btnSave_clicked ();
};
#endif // MACHINESDLG_H
