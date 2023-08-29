#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include "texteditor.h"

#include "vmlist.h"
#include <vmNumbers/vmnumberformats.h>
#include <dbRecords/dbrecord.h>

#include <QtWidgets/QDockWidget>

#include <functional>

class dockQP;
class dockBJ;

class Client;
class Job;
class Payment;

class vmComboBox;
class vmLineEdit;
class textEditWithCompleter;
class configOps;
class spreadSheetEditor;

class QFrame;
class QVBoxLayout;

static const uint REPORT_GENERATOR_SUB_WINDOW ( 8 );

class reportGenerator : public textEditor
{

public:
	explicit reportGenerator ( documentEditor* mdiParent );
	virtual ~reportGenerator ();

	void show ( const uint jobid = 0, const uint payid = 0 );
	
	static inline QString filter () {
		return tr ( "VivenciaManager Report (*.vmr)" );
	}

	bool loadFile ( const QString& filename ) override;
	bool saveAs ( const QString& filename ) override;

	void createJobReport ( const uint c_id, const bool include_all_paid = false,
						   const QString& filename = QString (), const bool show = true );
	void createPaymentStub ( const uint payID = 0 );
	const QString initialDir () const override;

	void makeCurrentWindow ();

	inline spreadSheetEditor* spreadSheet () const { return m_spreadSheet; }

	void insertReadCarefullyBlock ();
	void insertPaymentStubText ();
	void getUnpaidPayments ( Payment * const pay );
	void getPaidPayments ( const uint n_months_past = 12, const uint match_payid = 0, vmNumber* const total_paid_value = nullptr );
	bool insertJobKeySentences ();
	void insertReportsText ( const bool include_all_pays );
	void printPayInfo ( const Payment* pay, const uint pay_number, vmNumber& paid_total );
	void btnInsertHeader_clicked ( const uint c_id = 0 );
	void btnInsertFooter_clicked ( const uint c_id = 0 );
	void btnInsertLogo_clicked ( const uint c_id = 0 );
	void btnInsertFootNotes_clicked ( const uint c_id = 0 );
	void btnInsertProjectNumber_clicked ();

	void updateJobIDsAndQPs ( const QString& text );
	void showProjectID ( const int = 0 );
	void changeJobBriefInfo ( const int );
	void btnViewJob_clicked ( const bool );

	void btnCopyTableRow_clicked ();
	void btnCopyEntireTable_clicked ();
	void btnInsertClientAddress_clicked ( const uint c_id = 0 );
	void btnInsertJobReport_clicked ();
	void btnGenerateReport_clicked ( const uint c_id = 0, const bool include_all_paid = false );

private:
	QString mPDFName;

	QPushButton* btnInsertHeader, *btnInsertFooter;
	QPushButton* btnInsertLogo;
	QPushButton* btnInsertProjectNumber, *btnInsertFootNotes;
	QToolButton* btnGenerateReport;
	QToolButton* btnViewJob;

	vmComboBox* cboQPIDs, *cboJobIDs;
	vmComboBox* cboHeaderType, *cboHeaderJobType;

	vmLineEdit* txtClientsIDs;

	QPushButton* btnCopyTableRow, *btnCopyEntireTable;
	QPushButton* btnInsertClientAddress, *btnInsertJobReport;

	bool mb_IgnoreChange;

	Client* rgClient;
	Job* rgJob;
	Payment* rgPay;

	dockQP* m_dockSpreadSheet;
	dockBJ* m_dockBriefJob;

	spreadSheetEditor* m_spreadSheet;
	configOps* m_config;
};

class dockQP : public QDockWidget
{

public:
	explicit dockQP ();
	virtual ~dockQP ();

	void attach ( reportGenerator* instance );
	void detach ( reportGenerator* instance );
	void setCurrentWindow ( reportGenerator* report );

private:
	pointersList<reportGenerator*> instances;
	reportGenerator* cur_report;
	QVBoxLayout* mainLayout;
};

class dockBJ final : public QDockWidget
{

public:
	explicit dockBJ ();
	virtual ~dockBJ () final;

	void attach ( reportGenerator* instance );
	void detach ( reportGenerator* instance );
	void setCurrentWindow ( reportGenerator* report );
	void fillControls ( const Job *job );
	void btnCopyJobType_clicked ();
	void btnCopyJobDate_clicked ();
	void btnCopyJobPrice_clicked ();
	void btnCopyJobReport_clicked ();

private:
	pointersList<reportGenerator*> instances;

	QFrame* frmBriefJob;
	vmLineEdit* txtBriefJobType;
	vmLineEdit* txtBriefJobDate;
	vmLineEdit* txtBriefJobPrice;
	textEditWithCompleter* txtBriefJobReport;

	reportGenerator* cur_report;
};
#endif // REPORTGENERATOR_H
