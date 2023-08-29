#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QtWidgets/QDialog>

class emailConfigDialog;

namespace Ui
{
class configDialog;
}

class configDialog : public QDialog
{

public:
	explicit configDialog ( QWidget* parent = nullptr );
	virtual ~configDialog ();

	void connectUIForms ();
	void fillForms ();

private:
	void getAppSchemes ();
	void setAppScheme ( const QString& style );

	void on_btnCfgChooseFileManager_clicked ();
	void on_btnCfgUseDefaultFileManager_clicked ();
	void on_btnCfgChoosePictureViewer_clicked ();
	void on_btnCfgUseDefaultPictureViewer_clicked ();
	void on_btnCfgChoosePictureEditor_clicked ();
	void on_btnCfgUseDefaultPictureEditor_clicked ();
	void on_btnCfgChooseDocViewer_clicked ();
	void on_btnCfgUseDefaultDocumentViewer_clicked ();
	void on_btnCfgChooseDocEditor_clicked ();
	void on_btnCfgUseDefaultDocEditor_clicked ();
	void on_btnCfgChooseXlsEditor_clicked ();
	void on_btnCfgUseDefaultXlsEditor_clicked ();
	void on_btnCfgChooseBaseDir_clicked ();
	void on_btnCfgUseDefaultBaseDir_clicked ();
	void on_btnCfgChooseESTIMATEDir_clicked ();
	void on_btnCfgUseDefaultESTIMATEDir_clicked ();
	void on_btnCfgChooseReportsDir_clicked ();
	void on_btnCfgUseDefaultReportsDir_clicked ();
	void on_btnCfgChooseDropBoxDir_clicked ();
	void on_btnCfgUseDefaultDropBoxDir_clicked ();

private:
	Ui::configDialog* ui;
};

#endif // CONFIGDIALOG_H
