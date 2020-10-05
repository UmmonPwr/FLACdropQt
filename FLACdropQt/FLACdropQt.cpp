#include "FLACdropQt.h"

// create the main window
FLACdropQt::FLACdropQt(QWidget *parent)	: QMainWindow(parent)
{
	// load the embedded resorces
	Resource resource_image_banner(IDB_BANNER, L"PNG");
	Resource resource_image_FLACdropQt(IDB_ICON, L"PNG");

	mainWindow = new QWidget;
	setCentralWidget(mainWindow);

	// create main window layout
	main_layout = new QVBoxLayout;
	mainWindow->setLayout(main_layout);

	// start to populate the main window
	// add widgets to the progress bar area
	layout_progressbars = new QGridLayout;
	for (int i = 0; i < OUT_MAX_THREADS; i++)
		layout_progressbar_threads[i] = new QProgressBar;

	main_layout->addLayout(layout_progressbars);	// grid layout have to be added first and then populated
	
	// add the progress bars in two columns
	for (int i = 0; i < OUT_MAX_THREADS; i++) layout_progressbars->addWidget(layout_progressbar_threads[i], i/2, i%2);

	// add widgets to middle info area
	layout_filler_top = new QWidget;
	layout_filler_top->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	layout_label_info = new QLabel(tr("<i>Waiting for dropped audio files</i>"));
	layout_label_info->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	layout_label_info->setAlignment(Qt::AlignCenter);

	layout_filler_bottom = new QWidget;
	layout_filler_bottom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	main_layout->addWidget(layout_filler_top);
	main_layout->addWidget(layout_label_info);
	main_layout->addWidget(layout_filler_bottom);

	// add widgets to output format selection area
	layout_filler_radiobutton = new QWidget;
	layout_filler_radiobutton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	layout_radiobuttons = new QHBoxLayout;
	for (int i = 0; i < FILE_TYPE_QUANTITY; i++)
	{
		layout_radiobutton_outputtypes[i] = new QRadioButton(OUT_TYPE_NAMES[i], this);
		layout_radiobuttons->addWidget(layout_radiobutton_outputtypes[i]);
		(void)connect(layout_radiobutton_outputtypes[i], SIGNAL(toggled(bool)), this, SLOT(set_OUT_type()));
	}
	layout_radiobuttons->addWidget(layout_filler_radiobutton);

	main_layout->addLayout(layout_radiobuttons);

	// add widgets to bottom area
	layout_image_FLACdropLogo = new QImage;
	layout_image_FLACdropLogo->loadFromData((uchar*)resource_image_banner.GetResourcePointer(), resource_image_banner.GetResourceSize(), NULL);
	layout_label_FLACdropLogo = new QLabel;
	layout_label_FLACdropLogo->setPixmap(QPixmap::fromImage(*layout_image_FLACdropLogo));

	layout_label_status = new QLabel(tr("Status: OK"));

	main_layout->addWidget(layout_label_FLACdropLogo);
	main_layout->addWidget(layout_label_status);

	// setup the application appearance
	icon_FLACdrop = new QIcon;
	pixmap_FLACdropQt = new QPixmap;
	pixmap_FLACdropQt->loadFromData((uchar*)resource_image_FLACdropQt.GetResourcePointer(), resource_image_FLACdropQt.GetResourceSize(), NULL);
	icon_FLACdrop->addPixmap(*pixmap_FLACdropQt, QIcon::Normal, QIcon::On);

	setWindowIcon(*icon_FLACdrop);
	setWindowTitle(tr("FLACdropQt"));
	setMinimumSize(460, 360);
	resize(460, 360);
	
	createActions();
	createMenus();
	createOptionsWindow();

	// load settings and allocate encoder scheduler
	readSettings();
	setAcceptDrops(true);
	encoderScheduler = new cScheduler((QApplication*)this);
	encoderScheduler->moveToThread(&controller);
	connect(encoderScheduler, SIGNAL(setProgressbarValue(int, int)), this, SLOT(setProgressbarValue(int, int)));
	connect(this, SIGNAL(startEncoding()), encoderScheduler, SLOT(startEncoding()));
	controller.start();	// start the event loop of the encoder thread scheduler
}

void FLACdropQt::createActions()
{
	action_menu_options = new QAction(tr("&Options"), this);
	//action_menu_options->setStatusTip(tr("Display options dialog"));
	connect(FLACdropQt::action_menu_options, &QAction::triggered, this, &FLACdropQt::dialog_options);

	action_menu_exit = new QAction(tr("E&xit"), this);
	action_menu_exit->setShortcuts(QKeySequence::Quit);
	//action_menu_exit->setStatusTip(tr("Exit the application"));
	connect(FLACdropQt::action_menu_exit, &QAction::triggered, this, &QWidget::close);

	action_menu_log = new QAction(tr("&Log"), this);
	//action_menu_log->setStatusTip(tr("View the events log"));
	connect(FLACdropQt::action_menu_log, &QAction::triggered, this, &FLACdropQt::dialog_log);

	action_menu_about = new QAction(tr("&About"), this);
	//action_menu_about->setStatusTip(tr("Show the application's About box"));
	connect(FLACdropQt::action_menu_about, &QAction::triggered, this, &FLACdropQt::dialog_about);

	action_menu_aboutQt = new QAction(tr("About &Qt"), this);
	//action_menu_aboutQt->setStatusTip(tr("Show the Qt library's About box"));
	connect(FLACdropQt::action_menu_aboutQt, &QAction::triggered, this, &FLACdropQt::dialog_aboutQt);
	connect(FLACdropQt::action_menu_aboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

// create the menu bar of main window
void FLACdropQt::createMenus()
{
	menu_file = menuBar()->addMenu(tr("&File"));
	menu_file->addAction(action_menu_options);
	menu_file->addAction(action_menu_exit);

	menu_log = menuBar()->addMenu(tr("&Log"));
	menu_log->addAction(action_menu_log);

	menu_help = menuBar()->addMenu(tr("&Help"));
	menu_help->addAction(action_menu_about);
	menu_help->addAction(action_menu_aboutQt);
}

//---------------------------------------------------------------------------------
// Slots
//---------------------------------------------------------------------------------

// set the output type variable according which radio button was selected
void FLACdropQt::set_OUT_type()
{
	for (int i = 0; i < FILE_TYPE_QUANTITY; i++)
		if (layout_radiobutton_outputtypes[i]->isChecked() == true) FLACdropQtSettings.OUT_Type = i;
}

void FLACdropQt::setProgressbarLimits(int slot, int min, int max)
{
	layout_progressbar_threads[slot]->setRange(min, max);
}

void FLACdropQt::setProgressbarValue(int slot, int value)
{
	layout_progressbar_threads[slot]->setValue(value);
}

void FLACdropQt::setDrop(bool state)
{
	setAcceptDrops(state);
}

//---------------------------------------------------------------------------------
// Drag and drop implementation
//---------------------------------------------------------------------------------
void FLACdropQt::dragEnterEvent(QDragEnterEvent* event)
{
	// if some actions should not be usable, like move, this code must be adopted
	event->acceptProposedAction();
}

void FLACdropQt::dragMoveEvent(QDragMoveEvent* event)
{
	// if some actions should not be usable, like move, this code must be adopted
	event->acceptProposedAction();
}

void FLACdropQt::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}

// start the encoder thread scheduler and disable drops while it is running
void FLACdropQt::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();

	// get the list of the dropped files
	if (mimeData->hasUrls())
	{
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();

		for (int i = 0; i < urlList.size(); i++)
			pathList.append(urlList.at(i).toLocalFile());

		setAcceptDrops(false);
		encoderScheduler->addPathList(pathList);
		encoderScheduler->addEncoderSettings(FLACdropQtSettings);	// use the settings set at the start of the encoding, disregard changes made during the time of encoding
		emit startEncoding();
	}
}

//---------------------------------------------------------------------------------
// Destructor
//---------------------------------------------------------------------------------
FLACdropQt::~FLACdropQt()
{
	controller.quit();
	controller.wait();
}