#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QMainWindow>
#include <QFile>
#include <vector>

#include "ui_MainWindow.h"
#include "ui_info_dialog.h"
#include "InfoDialog.h"
#include "VMTabSettings.h"

class Ui_MainWindow;
class Ui_Info_dialog;
class QFile;

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
	public:
		explicit MainWindow(const QString &fileToOpen = QString(), QWidget *parent = 0);
		~MainWindow();
		
	protected:
		void closeEvent(QCloseEvent *event);
		void slotOpen();
		bool slotSave();
		bool slotSaveAs();
		
	private slots:
	// 	void slotAddMachine();
		void slotInfo();

	private:
		bool queryClose();
		bool loadFile(const QString &path);
		
		Ui_MainWindow *ui;
		std::vector<VMTabSettings*> VMTabSettings_vec;
		InfoDialog infoDialog;
};

#endif //MAINWINDOW_H
