#ifndef BUGETSDIALOG_H
#define BUGETSDIALOG_H

#include "global.h"
#include "mainwindow.h"

#include <vmUtils/fileops.h>

#include <QtWidgets/QDialog>

class newProjectDialog;

class QTreeWidgetItem;
class QAction;

class estimateDlg : public QDialog
{

public:
	explicit estimateDlg ( QWidget* parent = nullptr );
	virtual ~estimateDlg () override;
	void showWindow ( const QString& client_name );

private:
	void cancelItemSelection ();
	void selectEstimateFromFile ( QTreeWidgetItem* targetClient );
	void convertToProject ( QTreeWidgetItem* item );
	void projectActions ( QAction* action );
	jobListItem* findJobByPath ( QTreeWidgetItem* const item );
	void changeJobData ( jobListItem* const jobItem, const QString& strProjectPath, const QString& strProjectID);
	void estimateActions ( QAction* action );
	void reportActions ( QAction* action );
	void createReport ( QAction* action );
	uint actionIndex ( const QString& filename ) const;
	void updateButtons ();
	void openWithDoubleClick ( QTreeWidgetItem* item );
	void scanDir ();
	void setupActions ();
	void addToTree ( pointersList<fileOps::st_fileInfo *> &files, const QString& c_name, const int newItemType = -1 );
	void removeFiles ( QTreeWidgetItem* item, const bool bSingleFile = false, const bool bAsk = false );
	bool removeDir ( QTreeWidgetItem* item, const bool bAsk = false );
	bool renameDir ( QTreeWidgetItem* item, QString& strNewName );
	void addFilesToDir ( const bool bAddDoc, const bool bAddXls, const QString& projectpath, QString& projectid, pointersList<fileOps::st_fileInfo*>& files );
	void execAction ( const QTreeWidgetItem* item, const int action_id );

	newProjectDialog* m_npdlg;
};

#endif // BUGETSDIALOG_H
