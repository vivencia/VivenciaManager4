#ifndef CONFIGOPS_H
#define CONFIGOPS_H

#include "vmlibs.h"
#include "vmtextfile.h"

#include <QtCore/QString>

#include <functional>

class configFile;
class configDialog;

enum CFG_FIELDS
{
	HOME_DIR = 0, LAST_LOGGED_USER, FILEMANAGER, PICTURE_VIEWER, PICTURE_EDITOR, DOC_VIEWER,
	DOC_EDITOR, XLS_EDITOR, BASE_PROJECT_DIR, ESTIMATE_DIR, REPORT_DIR,
	BACKUP_DIR, DROPBOX_DIR, EMAIL_ADDRESS
};

static const QString STR_VIVENCIA_LOGO ( QStringLiteral ( "vivencia.jpg" ) );
static const QString STR_VIVENCIA_REPORT_LOGO ( QStringLiteral ( "vivencia_report_logo_2.jpg" ) );
static const QString STR_PROJECT_DOCUMENT_FILE ( QStringLiteral ( "project.docx" ) );
static const QString STR_PROJECT_SPREAD_FILE ( QStringLiteral ( "spreadsheet.xlsx" ) );
static const QString STR_PROJECT_PDF_FILE ( QStringLiteral ( "projeto.pdf" ) );
static const QString XDG_OPEN ( QStringLiteral ( "xdg-open" ) );

constexpr const int CFG_CATEGORIES ( 14 );

class configOps
{

public:
	explicit configOps ( const QString& filename = QString (), const QString& object_name = QString () );
	~configOps ();

	/* IMPORTANT: setAppName () must be called early on any program startup routine. appName () must not
	 * be used in any static variable because it will be empty
	 */
	inline static void setAppName ( const QString& app_name ) { configOps::_appName = app_name; }
	inline static const QString& appName () { return configOps::_appName; }

	inline void addManagedSectionName ( const QString& section_name )
	{
		if ( m_cfgFile != nullptr )
			m_cfgFile->addManagedSection ( section_name );
	}

	const QString& getValue ( const QString& section_name, const QString& category_name, const bool b_save_if_not_in_file = true );
	void setValue ( const QString& section_name, const QString& category_name, const QString& value );

	static const QString kdesu ( const QString& message );
	static const QString gksu ( const QString& message, const QString& appname );
	static const QString pkexec ();

	static bool isSystem ( const QString& os_name );
	static bool initSystem ( const QString& initName );

	inline static const QString mainSectionName () { return QStringLiteral ( "MAIN_SECTION" ); }
	inline static const QString appDefaultsSectionName () { return QStringLiteral ( "APP_DEFAULTS" ); }

	static const QString homeDir ();
	static const QString defaultConfigDir ();
	static const QString appConfigFile ();
	static const QString appDataDir ();

	inline const QString& loggedUser () const
	{
		return const_cast<configOps*>(this)->getValue ( mainSectionName (), configDefaultFieldsNames[LAST_LOGGED_USER] );
	}

	inline const QString& backupDir () const
	{
		return const_cast<configOps*>(this)->getValue ( mainSectionName (), configDefaultFieldsNames[BACKUP_DIR] );
	}

	inline const QString& setBackupDir ( const QString& str )
	{
		return setDir ( BACKUP_DIR, str );
	}

	inline const QString& dropboxDir () const
	{
		return const_cast<configOps*>(this)->getValue ( mainSectionName (), configDefaultFieldsNames[DROPBOX_DIR] );
	}

	inline const QString& setDropboxDir ( const QString& str )
	{
		return setDir ( DROPBOX_DIR, str );
	}

	inline const QString& fileManager () const
	{
		return const_cast<configOps*>(this)->getValue ( appDefaultsSectionName (), configDefaultFieldsNames[FILEMANAGER] );
	}

	inline const QString& setFileManager ( const QString& str )
	{
		return setApp ( FILEMANAGER, str );
	}

	inline const QString& pictureViewer () const
	{
		return const_cast<configOps*>(this)->getValue ( appDefaultsSectionName (), configDefaultFieldsNames[PICTURE_VIEWER] );
	}

	inline const QString& setPictureViewer ( const QString& str )
	{
		return setApp ( PICTURE_VIEWER, str );
	}

	inline const QString& pictureEditor () const
	{
		return const_cast<configOps*>(this)->getValue ( appDefaultsSectionName (), configDefaultFieldsNames[PICTURE_EDITOR] );
	}

	inline const QString& setPictureEditor ( const QString& str )
	{
		return setApp ( PICTURE_EDITOR, str );
	}

	inline const QString& universalViewer () const
	{
		return const_cast<configOps*>(this)->getValue ( appDefaultsSectionName (), configDefaultFieldsNames[DOC_VIEWER] );
	}

	inline const QString& setUniversalViewer ( const QString& str )
	{
		return setApp ( DOC_VIEWER, str );
	}

	inline const QString& docEditor () const
	{
		return const_cast<configOps*>(this)->getValue ( appDefaultsSectionName (), configDefaultFieldsNames[DOC_EDITOR] );
	}

	inline const QString& setDocEditor ( const QString& str )
	{
		return setApp ( DOC_EDITOR, str );
	}

	inline const QString& xlsEditor () const
	{
		return const_cast<configOps*>(this)->getValue ( appDefaultsSectionName (), configDefaultFieldsNames[XLS_EDITOR] );
	}

	inline const QString& setXlsEditor ( const QString& str )
	{
		return setApp ( XLS_EDITOR, str );
	}

	std::function<const QString& ()> projectsBaseDir;

	inline const QString& setProjectsBaseDir ( const QString& str )
	{
		return setDir ( BASE_PROJECT_DIR, str );
	}

	const QString& getProjectBasePath ( const QString& client_name );

	inline static const QString estimatesDirSuffix () { return QStringLiteral ( u"Orçamentos" ); }
	const QString& estimatesDir ( const QString& client_name );
	const QString& setEstimatesDir ( const QString&, const bool full_path = false );

	inline static const QString reportsDirSuffix () { return QStringLiteral ( u"Relatórios" ); }
	const QString& reportsDir ( const QString& client_name );
	const QString& setReportsDir ( const QString& str, const bool full_path = false );

	static const QString& defaultEmailAddress ();

	inline const QString vivienciaLogo () const {
		return appDataDir () + STR_VIVENCIA_LOGO;
	}
	inline const QString vivienciaReportLogo () const {
		return appDataDir () + STR_VIVENCIA_REPORT_LOGO;
	}
	inline const QString projectDocumentFile () const {
		return appDataDir () + STR_PROJECT_DOCUMENT_FILE;
	}
	inline const QString projectSpreadSheetFile () const {
		return appDataDir () + STR_PROJECT_SPREAD_FILE;
	}
	inline const QString projectPDFFile () const {
		return appDataDir () + STR_PROJECT_PDF_FILE;
	}

	static inline const QString projectDocumentExtension () {
		return QStringLiteral ( ".docx" );
	}
	static inline const QString projectDocumentFormerExtension () {
		return QStringLiteral ( ".doc" );
	}
	static inline const QString projectReportExtension () {
		return QStringLiteral ( ".vmr" );
	}
	static inline const QString projectSpreadSheetExtension () {
		return QStringLiteral ( ".xlsx" );
	}
	static inline const QString projectSpreadSheetFormerExtension () {
		return QStringLiteral ( ".xls" );
	}
	static inline const QString projectPDFExtension () {
		return QStringLiteral ( ".pdf" );
	}
	static inline const QString projectExtraFilesExtension () {
		return QStringLiteral ( ".txt" );
	}

	void getWindowGeometry ( QWidget* window, const QString& section_name, const QString& category_name );
	void saveWindowGeometry ( QWidget* window, const QString& section_name, const QString& category_name );

private:
	friend const QString& getDefaultFieldValuesByCategoryName ( const QString& category_name );

	const QString& projectsBaseDir_init ();
	const QString& projectsBaseDir_fast () { return mStrBaseDir; }
	const QString& setApp ( const CFG_FIELDS field, const QString& app );
	const QString& setDir ( const CFG_FIELDS field, const QString& dir );

	configFile* m_cfgFile;

	QString m_filename, mRetString, mStrBaseDir;
	static QString _appName;
	static const QString configDefaultFieldsNames[CFG_CATEGORIES];
};

#endif // CONFIGOPS_H
