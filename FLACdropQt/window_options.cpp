#include "FLACdropQt.h"

void FLACdropQt::createOptionsWindow()
{
	// create main layout for options window
	optionsWindow = new QWidget;
	options_layout = new QGridLayout;

	options_group_output = new QGroupBox(tr("output"));
	options_group_libflac = new QGroupBox(tr("libflac"));
	options_group_libmp3lame = new QGroupBox(tr("libmp3lame"));
	options_output_layout = new QVBoxLayout;
	options_libflac_layout = new QVBoxLayout;
	options_libmp3lame_layout = new QVBoxLayout;

	options_layout->addWidget(options_group_output, 0, 0);
	options_layout->addWidget(options_group_libflac, 0, 1);
	options_layout->addWidget(options_group_libmp3lame, 0, 2);

	// add output group layout
	options_output_label_threads = new QLabel;
	options_output_label_threads->setText("Batch processing threads:\n(1-8)");
	options_output_label_threads->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

	options_output_slider_threads = new QSlider;
	options_output_slider_threads->setMinimum(OUT_MIN_THREADS);
	options_output_slider_threads->setMaximum(OUT_MAX_THREADS);
	options_output_slider_threads->setTickInterval(1);
	options_output_slider_threads->setTickPosition(QSlider::TicksAbove);
	options_output_slider_threads->setOrientation(Qt::Horizontal);

	options_output_spinbox_threads = new QSpinBox;
	options_output_spinbox_threads->setMinimum(OUT_MIN_THREADS);
	options_output_spinbox_threads->setMaximum(OUT_MAX_THREADS);
	
	options_output_layout->addWidget(options_output_label_threads);
	options_output_layout->addWidget(options_output_spinbox_threads);
	options_output_layout->addWidget(options_output_slider_threads);
	options_group_output->setLayout(options_output_layout);

	// add libflac group layout
	options_libflac_label_quality = new QLabel;
	options_libflac_label_quality->setText("Quality:\n(higher is better)\n(1-8)");
	options_libflac_label_quality->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

	options_libflac_slider_quality = new QSlider;
	options_libflac_slider_quality->setMinimum(FLAC_MINENCODINGQUALITY);
	options_libflac_slider_quality->setMaximum(FLAC_MAXENCODINGQUALITY);
	options_libflac_slider_quality->setTickInterval(1);
	options_libflac_slider_quality->setTickPosition(QSlider::TicksAbove);
	options_libflac_slider_quality->setOrientation(Qt::Horizontal);

	options_libflac_spinbox_quality = new QSpinBox;
	options_libflac_spinbox_quality->setMinimum(FLAC_MINENCODINGQUALITY);
	options_libflac_spinbox_quality->setMaximum(FLAC_MAXENCODINGQUALITY);

	options_libflac_checkbox_verify = new QCheckBox("Verify");
	options_libflac_checkbox_md5 = new QCheckBox("MD5 checking");
	
	options_libflac_layout->addWidget(options_libflac_label_quality);
	options_libflac_layout->addWidget(options_libflac_spinbox_quality);
	options_libflac_layout->addWidget(options_libflac_slider_quality);
	options_libflac_layout->addWidget(options_libflac_checkbox_verify);
	options_libflac_layout->addWidget(options_libflac_checkbox_md5);
	options_group_libflac->setLayout(options_libflac_layout);

	// add libmp3lame group layout
	options_libmp3lame_label_internalquality = new QLabel;
	options_libmp3lame_label_internalquality->setText("Internal encoding quality:\n(lower is better)\n(0-9)");
	options_libmp3lame_label_internalquality->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

	options_libmp3lame_slider_internalquality = new QSlider;
	options_libmp3lame_slider_internalquality->setMinimum(LAME_MININTERNALQUALITY);
	options_libmp3lame_slider_internalquality->setMaximum(LAME_MAXINTERNALQUALITY);
	options_libmp3lame_slider_internalquality->setTickInterval(1);
	options_libmp3lame_slider_internalquality->setTickPosition(QSlider::TicksAbove);
	options_libmp3lame_slider_internalquality->setOrientation(Qt::Horizontal);

	options_libmp3lame_spinbox_internalquality = new QSpinBox;
	options_libmp3lame_spinbox_internalquality->setMinimum(LAME_MININTERNALQUALITY);
	options_libmp3lame_spinbox_internalquality->setMaximum(LAME_MAXINTERNALQUALITY);

	options_libmp3lame_label_cbrbitrate = new QLabel;
	options_libmp3lame_label_cbrbitrate->setText("Constant Bit Rate (CBR):\n(higher is better)");
	options_libmp3lame_label_cbrbitrate->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

	options_libmp3lame_combobox_cbrbitrate = new QComboBox;
	for (int i=0; i<LAME_CBRBITRATES_QUANTITY; i++) options_libmp3lame_combobox_cbrbitrate->insertItem(i, LAME_CBRBITRATES_TEXT[i], LAME_CBRBITRATES[i]);

	options_libmp3lame_label_vbrquality = new QLabel;
	options_libmp3lame_label_vbrquality->setText("Variable Bit Rate quality (VBR):\n(lower is better)\n(0-9)");
	options_libmp3lame_label_vbrquality->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

	options_libmp3lame_slider_vbrquality = new QSlider;
	options_libmp3lame_slider_vbrquality->setMinimum(LAME_MINVBRQUALITY);
	options_libmp3lame_slider_vbrquality->setMaximum(LAME_MAXVBRQUALITY);
	options_libmp3lame_slider_vbrquality->setTickInterval(1);
	options_libmp3lame_slider_vbrquality->setTickPosition(QSlider::TicksAbove);
	options_libmp3lame_slider_vbrquality->setOrientation(Qt::Horizontal);

	options_libmp3lame_spinbox_vbrquality = new QSpinBox;
	options_libmp3lame_spinbox_vbrquality->setMinimum(LAME_MINVBRQUALITY);
	options_libmp3lame_spinbox_vbrquality->setMaximum(LAME_MAXVBRQUALITY);

	options_libmp3lame_label_mp3type = new QLabel;
	options_libmp3lame_label_mp3type->setText("Encoding type:");
	options_libmp3lame_label_mp3type->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

	options_libmp3lame_radiobutton_cbr = new QRadioButton("CBR");
	options_libmp3lame_radiobutton_vbr = new QRadioButton("VBR");

	options_libmp3lame_layout->addWidget(options_libmp3lame_label_internalquality);
	options_libmp3lame_layout->addWidget(options_libmp3lame_spinbox_internalquality);
	options_libmp3lame_layout->addWidget(options_libmp3lame_slider_internalquality);
	options_libmp3lame_layout->addWidget(options_libmp3lame_label_cbrbitrate);
	options_libmp3lame_layout->addWidget(options_libmp3lame_combobox_cbrbitrate);
	options_libmp3lame_layout->addWidget(options_libmp3lame_label_vbrquality);
	options_libmp3lame_layout->addWidget(options_libmp3lame_spinbox_vbrquality);
	options_libmp3lame_layout->addWidget(options_libmp3lame_slider_vbrquality);
	options_libmp3lame_layout->addWidget(options_libmp3lame_label_mp3type);
	options_libmp3lame_layout->addWidget(options_libmp3lame_radiobutton_cbr);
	options_libmp3lame_layout->addWidget(options_libmp3lame_radiobutton_vbr);
	options_group_libmp3lame->setLayout(options_libmp3lame_layout);

	// add buttons
	options_button_save = new QPushButton(tr("Save"));
	options_layout->addWidget(options_button_save, 1, 0);
	options_button_close = new QPushButton(tr("Close"));
	options_layout->addWidget(options_button_close, 1, 2);

	// connect signals - output
	connect(options_output_slider_threads, SIGNAL(valueChanged(int)), options_output_spinbox_threads, SLOT(setValue(int)));
	connect(options_output_spinbox_threads, SIGNAL(valueChanged(int)), options_output_slider_threads, SLOT(setValue(int)));

	// connect signals - libflac
	connect(options_libflac_slider_quality, SIGNAL(valueChanged(int)), options_libflac_spinbox_quality, SLOT(setValue(int)));
	connect(options_libflac_spinbox_quality, SIGNAL(valueChanged(int)), options_libflac_slider_quality, SLOT(setValue(int)));

	// connect signals - libmp3lame
	connect(options_libmp3lame_slider_internalquality, SIGNAL(valueChanged(int)), options_libmp3lame_spinbox_internalquality, SLOT(setValue(int)));
	connect(options_libmp3lame_spinbox_internalquality, SIGNAL(valueChanged(int)), options_libmp3lame_slider_internalquality, SLOT(setValue(int)));

	connect(options_libmp3lame_slider_vbrquality, SIGNAL(valueChanged(int)), options_libmp3lame_spinbox_vbrquality, SLOT(setValue(int)));
	connect(options_libmp3lame_spinbox_vbrquality, SIGNAL(valueChanged(int)), options_libmp3lame_slider_vbrquality, SLOT(setValue(int)));

	// connect signals - buttons
	connect(FLACdropQt::options_button_save, SIGNAL(released()), this, SLOT(writeSettings()));
	connect(FLACdropQt::options_button_close, SIGNAL(released()), optionsWindow, SLOT(hide()));
	optionsWindow->setLayout(options_layout);
	optionsWindow->setWindowIcon(*icon_FLACdrop);
	optionsWindow->hide();
}

void FLACdropQt::dialog_options()
{
	layout_label_info->setText(tr("Invoked <b>File|Options Dialog</b>"));

	// setup the widget values according to the actual settings before showing the window
	options_output_slider_threads->setValue(FLACdropQtSettings.OUT_Threads);

	options_libflac_slider_quality->setValue(FLACdropQtSettings.FLAC_EncodingQuality);

	options_libflac_checkbox_md5->setChecked(FLACdropQtSettings.FLAC_MD5check);
	options_libflac_checkbox_verify->setChecked(FLACdropQtSettings.FLAC_Verify);

	options_libmp3lame_slider_internalquality->setValue(FLACdropQtSettings.LAME_InternalQuality);

	options_libmp3lame_slider_vbrquality->setValue(FLACdropQtSettings.LAME_VBRQuality);

	options_libmp3lame_combobox_cbrbitrate->setCurrentIndex(FLACdropQtSettings.LAME_CBRBitrate);

	switch (FLACdropQtSettings.LAME_EncodingMode)
	{
	case LAME_ENCODINGMODE_CBR:
		options_libmp3lame_radiobutton_cbr->setChecked(true);
		break;
	case LAME_ENCODINGMODE_VBR:
		options_libmp3lame_radiobutton_vbr->setChecked(true);
		break;
	}

	optionsWindow->show();
}