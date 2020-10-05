#pragma once
#include <QtWidgets>
#include <QMainWindow>
#include <QProgressBar>
#include <QRadioButton>
#include <QSlider>
#include <QComboBox>
#include <QSpinbox>
#include "ui_FLACdropQt.h"
#include "resource.h"

#include "encoders.h"

// default values for system variables of libmp3lame
#define LAME_FLUSH true
#define LAME_NOGAP false
#define LAME_CBRBITRATE 10			// index of the "LAME_CBRBITRATES" array, starts with 0
#define LAME_INTERNALQUALITY 2		// 0..9, recommended: 2
#define LAME_MININTERNALQUALITY 0
#define LAME_MAXINTERNALQUALITY 9
#define LAME_VBRQUALITY 1			// 0..9, recommended: 1
#define LAME_MINVBRQUALITY 0
#define LAME_MAXVBRQUALITY 9
#define LAME_ENCODINGMODE 0
#define LAME_ENCODINGMODE_CBR 0
#define LAME_ENCODINGMODE_VBR 1
#define OUT_TYPE 0
#define OUT_THREADS 4

const QString LAME_CBRBITRATES_TEXT[]= {
	"48", "64", "80", "96", "112", "128", "160", "192", "224", "256", "320"};
const int LAME_CBRBITRATES[] = {
	48, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
#define LAME_CBRBITRATES_QUANTITY 11	// quantity of items in "LAME_CBRBITRATES" array

// file formats handled by the app
const QString OUT_TYPE_NAMES[] = {
	"FLAC", "MP3", "WAVE" };

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QLabel;
class QMenu;
QT_END_NAMESPACE

class FLACdropQt : public QMainWindow
{
	Q_OBJECT

public:
	FLACdropQt(QWidget *parent = Q_NULLPTR);
	~FLACdropQt();

signals:
	void startEncoding();

private slots:
	void dialog_options();
	void dialog_log();
	void dialog_about();
	void dialog_aboutQt();

	void set_OUT_type();
	void writeSettings();

	void setProgressbarLimits(int slot, int min, int max);
	void setProgressbarValue(int slot, int value);
	void setDrop(bool state);

protected:
	void dragEnterEvent(QDragEnterEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dragLeaveEvent(QDragLeaveEvent* event);
	void dropEvent(QDropEvent* event);

private:
	void createActions();
	void createMenus();
	void createOptionsWindow();
	void readSettings();

	QThread controller;

	sFLACdropQtSettings FLACdropQtSettings;	// global settings
	cScheduler* encoderScheduler;

	QMenu* menu_file;
	QMenu* menu_log;
	QMenu* menu_help;
	
	QAction* action_menu_options;
	QAction* action_menu_exit;
	QAction* action_menu_log;
	QAction* action_menu_about;
	QAction* action_menu_aboutQt;

	// widgets on main window
	QWidget* mainWindow;
	QVBoxLayout* main_layout;
	
	QGridLayout* layout_progressbars;
	QHBoxLayout* layout_radiobuttons;

	QWidget* layout_filler_top;
	QWidget* layout_filler_bottom;
	QWidget* layout_filler_radiobutton;

	QLabel* layout_label_info;
	QLabel* layout_label_status;

	QImage* layout_image_FLACdropLogo;
	QLabel* layout_label_FLACdropLogo;
	QIcon* icon_FLACdrop;
	QPixmap* pixmap_FLACdropQt;

	QProgressBar* layout_progressbar_threads[OUT_MAX_THREADS];
	QRadioButton* layout_radiobutton_outputtypes[FILE_TYPE_QUANTITY];

	// widgets on options window
	QWidget* optionsWindow;
	QGridLayout* options_layout;

	QGroupBox* options_group_output;
	QGroupBox* options_group_libflac;
	QGroupBox* options_group_libmp3lame;
	QVBoxLayout* options_output_layout;
	QVBoxLayout* options_libflac_layout;
	QVBoxLayout* options_libmp3lame_layout;
	QPushButton* options_button_save;
	QPushButton* options_button_close;
	
	// widgets on options window - output
	QLabel* options_output_label_threads;
	QSlider* options_output_slider_threads;
	QSpinBox* options_output_spinbox_threads;

	// widgets on options window - libflac
	QLabel* options_libflac_label_quality;
	QSlider* options_libflac_slider_quality;
	QSpinBox* options_libflac_spinbox_quality;
	QCheckBox* options_libflac_checkbox_verify;
	QCheckBox* options_libflac_checkbox_md5;

	// widgets on options window - libmp3lame
	QLabel* options_libmp3lame_label_internalquality;
	QSlider* options_libmp3lame_slider_internalquality;
	QSpinBox* options_libmp3lame_spinbox_internalquality;
	QLabel* options_libmp3lame_label_cbrbitrate;
	QComboBox* options_libmp3lame_combobox_cbrbitrate;
	QLabel* options_libmp3lame_label_vbrquality;
	QSlider* options_libmp3lame_slider_vbrquality;
	QSpinBox* options_libmp3lame_spinbox_vbrquality;
	QLabel* options_libmp3lame_label_mp3type;
	QRadioButton* options_libmp3lame_radiobutton_cbr;
	QRadioButton* options_libmp3lame_radiobutton_vbr;

	Ui::FLACdropQtClass ui;
};

class Resource
{
public:
	struct Parameters
	{
		DWORD size_bytes;
		LPVOID ptr;
	};

	Resource(int resource_id, const LPCWSTR resource_class);
	LPVOID GetResourcePointer();
	DWORD GetResourceSize();

private:
	HRSRC hResource;
	HGLOBAL hMemory;
	Parameters p;
};