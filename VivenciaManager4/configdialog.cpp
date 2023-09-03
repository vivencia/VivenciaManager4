#include "configdialog.h"
#include "ui_configdialog.h"
#include "system_init.h"
#include "mainwindow.h"

#include <vmUtils/configops.h>
#include <vmUtils/fileops.h>
#include <vmWidgets/vmwidgets.h>
#include <vmTaskPanel/actionpanelscheme.h>

#include <QtWidgets/QMenu>

configDialog::configDialog ( QWidget* parent )
	: QDialog ( parent ), ui ( new Ui::configDialog )
{
	ui->setupUi ( this );
	setWindowTitle ( PROGRAM_NAME + windowTitle () );
	fillForms ();
	connectUIForms ();
}

configDialog::~configDialog ()
{
	delete ui;
}

void configDialog::connectUIForms ()
{
	getAppSchemes ();

	static_cast<void>( connect ( ui->btnCfgChooseFileManager, &QToolButton::clicked, this, [&] () { return on_btnCfgChooseFileManager_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultFileManager, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultFileManager_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChoosePictureViewer, &QToolButton::clicked, this, [&] () { return on_btnCfgChoosePictureViewer_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultPictureViewer, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultPictureViewer_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChoosePictureEditor, &QToolButton::clicked, this, [&] () { return on_btnCfgChoosePictureEditor_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultPictureEditor, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultPictureEditor_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChooseDocViewer, &QToolButton::clicked, this, [&] () { return on_btnCfgChooseDocViewer_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultDocumentViewer, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultDocumentViewer_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChooseDocEditor, &QToolButton::clicked, this, [&] () { return on_btnCfgChooseDocEditor_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultDocEditor, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultDocEditor_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChooseXlsEditor, &QToolButton::clicked, this, [&] () { return on_btnCfgChooseXlsEditor_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultXlsEditor, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultXlsEditor_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChooseBaseDir, &QToolButton::clicked, this, [&] () { return on_btnCfgChooseBaseDir_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultBaseDir, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultBaseDir_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChooseEstimatesDir, &QToolButton::clicked, this, [&] () { return on_btnCfgChooseESTIMATEDir_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultEstimatesDir, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultESTIMATEDir_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChooseReportsDir, &QToolButton::clicked, this, [&] () { return on_btnCfgChooseReportsDir_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultReportsDir, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultReportsDir_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgChooseDropboxDir, &QToolButton::clicked, this, [&] () { return on_btnCfgChooseDropBoxDir_clicked (); } ) );
	static_cast<void>( connect ( ui->btnCfgUseDefaultDropboxDir, &QToolButton::clicked, this, [&] () { return on_btnCfgUseDefaultDropBoxDir_clicked (); } ) );

	ui->txtCfgFileManager->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setFileManager ( txtWidget->text () ); } );
	ui->txtCfgPictureViewer->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setPictureViewer ( txtWidget->text () ); } );
	ui->txtCfgPictureEditor->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setPictureEditor ( txtWidget->text () ); } );
	ui->txtCfgDocumentViewer->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setUniversalViewer ( txtWidget->text () ); } );
	ui->txtCfgDocEditor->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setDocEditor ( txtWidget->text () ); } );
	ui->txtCfgXlsEditor->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setXlsEditor ( txtWidget->text () ); } );
	ui->txtCfgJobsPrefix->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setProjectsBaseDir ( txtWidget->text () ); } );
	ui->txtCfgEstimate->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setEstimatesDir ( txtWidget->text () ); } );
	ui->txtCfgReports->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setReportsDir ( txtWidget->text () ); } );
	ui->txtCfgDropbox->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			CONFIG ()->setDropboxDir ( txtWidget->text () ); } );
	ui->txtCfgScheme->setCallbackForContentsAltered ( [&] (const vmWidget* txtWidget ) {
			MAINWINDOW ()->changeSchemeStyle ( txtWidget->text (), true ); } );

	ui->txtCfgFileManager->setEditable ( true );
	ui->txtCfgPictureViewer->setEditable ( true );
	ui->txtCfgPictureEditor->setEditable ( true );
	ui->txtCfgDocumentViewer->setEditable ( true );
	ui->txtCfgDocEditor->setEditable ( true );
	ui->txtCfgXlsEditor->setEditable ( true );
	ui->txtCfgJobsPrefix->setEditable ( true );
	ui->txtCfgEstimate->setEditable ( true );
	ui->txtCfgReports->setEditable ( true );
	ui->txtCfgDropbox->setEditable ( true );
	ui->txtCfgScheme->setEditable ( true );
	
	static_cast<void>( ui->btnClose->connect ( ui->btnClose, &QPushButton::clicked, this, [&] ( const bool ) { return close (); } ) );
}

void configDialog::fillForms ()
{
	ui->txtCfgFileManager->setText ( CONFIG ()->fileManager () );
	ui->txtCfgPictureViewer->setText ( CONFIG ()->pictureViewer () );
	ui->txtCfgPictureEditor->setText ( CONFIG ()->pictureEditor () );
	ui->txtCfgDocumentViewer->setText ( CONFIG ()->universalViewer () );
	ui->txtCfgDocEditor->setText ( CONFIG ()->docEditor () );
	ui->txtCfgXlsEditor->setText ( CONFIG ()->xlsEditor () );
	ui->txtCfgJobsPrefix->setText ( CONFIG ()->projectsBaseDir () );
	ui->txtCfgEstimate->setText ( CONFIG ()->estimatesDirSuffix () );
	ui->txtCfgReports->setText ( CONFIG ()->reportsDirSuffix () );
	ui->txtCfgDropbox->setText ( CONFIG ()->dropboxDir () );
	ui->txtCfgScheme->setText ( CONFIG ()->getValue ( Ui::mw_configSectionName, Ui::mw_configCategoryAppScheme ) );
}

void configDialog::getAppSchemes ()
{
	auto schemes_menu ( new QMenu );
	schemes_menu->addAction ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_NONE], this, 
			[&] () { return setAppScheme ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_NONE] ); } );
	schemes_menu->addAction ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_DEFAULT], this, 
			[&] () { return setAppScheme ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_DEFAULT] ); } );
	schemes_menu->addAction ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_DEFAULT_2], this, 
			[&] () { return setAppScheme ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_DEFAULT_2] ); } );
	schemes_menu->addAction ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_VISTA], this, 
			[&] () { return setAppScheme ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_VISTA] ); } );
	schemes_menu->addAction ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_XP_1], this, 
			[&] () { return setAppScheme ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_XP_1] ); } );
	schemes_menu->addAction ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_XP_2], this, 
			[&] () { return setAppScheme ( ActionPanelScheme::PanelStyleStr[ActionPanelScheme::PANEL_XP_2] ); } );

	ui->btnCfgScheme->setMenu ( schemes_menu );
}

void configDialog::setAppScheme ( const QString& style )
{
	ui->txtCfgScheme->setText ( style, true );
	MAINWINDOW ()->changeSchemeStyle ( style );
}

void configDialog::on_btnCfgChooseFileManager_clicked ()
{
	const QString filename ( fileOps::getOpenFileName ( QStringLiteral ( "/usr/bin/" ), QStringLiteral ( "*.*" ) ) );
	if ( !filename.isEmpty () )
	{
		ui->txtCfgFileManager->setText ( filename, true );
		ui->txtCfgFileManager->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultFileManager_clicked ()
{
	ui->txtCfgFileManager->setText ( CONFIG ()->fileManager (), true );
	ui->txtCfgFileManager->setFocus ();
}

void configDialog::on_btnCfgChoosePictureViewer_clicked ()
{
	const QString filename ( fileOps::getOpenFileName ( QStringLiteral ( "/usr/bin/" ), QStringLiteral ( "*.*" ) ) );
	if ( !filename.isEmpty () )
	{
		ui->txtCfgPictureViewer->setText ( filename, true );
		ui->txtCfgPictureViewer->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultPictureViewer_clicked ()
{
	ui->txtCfgPictureViewer->setText ( CONFIG ()->pictureViewer (), true );
	ui->txtCfgPictureViewer->setFocus ();
}

void configDialog::on_btnCfgChoosePictureEditor_clicked ()
{
	const QString filename ( fileOps::getOpenFileName ( QStringLiteral ( "/usr/bin/" ), QStringLiteral ( "*.*" ) ) );
	if ( !filename.isEmpty () )
	{
		ui->txtCfgPictureEditor->setText ( filename, true );
		ui->txtCfgPictureEditor->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultPictureEditor_clicked ()
{
	ui->txtCfgPictureEditor->setText ( CONFIG ()->pictureEditor (), true );
	ui->txtCfgPictureEditor->setFocus ();
}

void configDialog::on_btnCfgChooseDocViewer_clicked ()
{
	const QString filename ( fileOps::getOpenFileName ( QStringLiteral ( "/usr/bin/" ), QStringLiteral ( "*.*" ) ) );
	if ( !filename.isEmpty () )
	{
		ui->txtCfgDocumentViewer->setText ( filename, true );
		ui->txtCfgDocumentViewer->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultDocumentViewer_clicked ()
{
	ui->txtCfgDocumentViewer->setText ( CONFIG ()->universalViewer (), true );
	ui->txtCfgDocumentViewer->setFocus ();
}

void configDialog::on_btnCfgChooseDocEditor_clicked ()
{
	const QString filename ( fileOps::getOpenFileName ( QStringLiteral ( "/usr/bin/" ), QStringLiteral ( "*.*" ) ) );
	if ( !filename.isEmpty () )
	{
		ui->txtCfgDocEditor->setText ( filename, true );
		ui->txtCfgDocEditor->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultDocEditor_clicked ()
{
	ui->txtCfgDocEditor->setText ( CONFIG ()->docEditor (), true );
	ui->txtCfgDocEditor->setFocus ();
}

void configDialog::on_btnCfgChooseXlsEditor_clicked ()
{
	const QString filename ( fileOps::getOpenFileName ( QStringLiteral ( "/usr/bin/" ), QStringLiteral ( "*.*" ) ) );
	if ( !filename.isEmpty () )
	{
		ui->txtCfgXlsEditor->setText ( filename, true );
		ui->txtCfgXlsEditor->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultXlsEditor_clicked ()
{
	ui->txtCfgXlsEditor->setText ( CONFIG ()->docEditor (), true );
	ui->txtCfgXlsEditor->setFocus ();
}

void configDialog::on_btnCfgChooseBaseDir_clicked ()
{
	const QString dir ( fileOps::getExistingDir ( ui->txtCfgJobsPrefix->text () ) );
	if ( !dir.isEmpty () )
	{
		ui->txtCfgJobsPrefix->setText ( dir, true );
		ui->txtCfgJobsPrefix->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultBaseDir_clicked ()
{
	ui->txtCfgJobsPrefix->setText ( CONFIG ()->projectsBaseDir (), true );
	ui->txtCfgJobsPrefix->setFocus ();
}

void configDialog::on_btnCfgChooseESTIMATEDir_clicked ()
{
	const QString dir ( fileOps::getExistingDir ( CONFIG ()->projectsBaseDir () + CONFIG ()->estimatesDirSuffix () ) );
	if ( !dir.isEmpty () )
	{
		ui->txtCfgEstimate->setText ( dir, true );
		ui->txtCfgEstimate->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultESTIMATEDir_clicked ()
{
	ui->txtCfgEstimate->setText ( CONFIG ()->estimatesDirSuffix (), true );
	ui->txtCfgEstimate->setFocus ();
}

void configDialog::on_btnCfgChooseReportsDir_clicked ()
{
	const QString dir ( fileOps::getExistingDir ( CONFIG ()->projectsBaseDir () + CONFIG ()->reportsDirSuffix () ) );
	if ( !dir.isEmpty () )
	{
		ui->txtCfgReports->setText ( dir, true );
		ui->txtCfgReports->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultReportsDir_clicked ()
{
	ui->txtCfgReports->setText ( CONFIG ()->reportsDirSuffix (), true );
	ui->txtCfgReports->setFocus ();
}

void configDialog::on_btnCfgChooseDropBoxDir_clicked ()
{
	const QString dir ( fileOps::getExistingDir ( CONFIG ()->dropboxDir() ) );
	if ( !dir.isEmpty () )
	{
		ui->txtCfgDropbox->setText ( dir, true );
		ui->txtCfgDropbox->setFocus ();
	}
}

void configDialog::on_btnCfgUseDefaultDropBoxDir_clicked ()
{
	ui->txtCfgDropbox->setText ( CONFIG ()->dropboxDir (), true );
	ui->txtCfgDropbox->setFocus ();
}
