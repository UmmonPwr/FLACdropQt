#include "FLACdropQt.h"

// load app settings using QSettings
void FLACdropQt::readSettings()
{
	// load values
	FLACdropQtSettings.OUT_Type = settings->value("OUT_TYPE", OUT_TYPE).toInt();
	FLACdropQtSettings.OUT_Threads = settings->value("OUT_THREADS", OUT_THREADS).toInt();
	FLACdropQtSettings.FLAC_EncodingQuality = settings->value("FLAC_ENCODINGQUALITY", FLAC_ENCODINGQUALITY).toInt();
	FLACdropQtSettings.FLAC_MD5check = settings->value("FLAC_MD5CHECK", FLAC_MD5CHECK).toBool();
	FLACdropQtSettings.FLAC_Verify = settings->value("FLAC_VERIFY", FLAC_VERIFY).toBool();
	FLACdropQtSettings.LAME_InternalQuality = settings->value("LAME_INTERNALQUALITY", LAME_INTERNALQUALITY).toInt();
	FLACdropQtSettings.LAME_VBRQuality = settings->value("LAME_VBRQUALITY", LAME_VBRQUALITY).toInt();
	FLACdropQtSettings.LAME_EncodingMode = settings->value("LAME_ENCODINGMODE", LAME_ENCODINGMODE).toInt();
	FLACdropQtSettings.LAME_CBRBitrate = settings->value("LAME_CBRBITRATE", LAME_CBRBITRATE).toInt();
	FLACdropQtSettings.LAME_Flush = settings->value("LAME_FLUSH", LAME_FLUSH).toBool();
	FLACdropQtSettings.LAME_NoGap = settings->value("LAME_NOGAP", LAME_NOGAP).toBool();

	// setup the widget status's
	// main window
	main_radiobutton_outputtypes[FLACdropQtSettings.OUT_Type]->setChecked(true);

	// options - output
	options_output_slider_threads->setValue(FLACdropQtSettings.OUT_Threads);
	options_output_spinbox_threads->setValue(FLACdropQtSettings.OUT_Threads);
	
	// options - libflac
	options_libflac_slider_quality->setValue(FLACdropQtSettings.FLAC_EncodingQuality);
	options_libflac_spinbox_quality->setValue(FLACdropQtSettings.FLAC_EncodingQuality);

	options_libflac_checkbox_md5->setChecked(FLACdropQtSettings.FLAC_MD5check);
	options_libflac_checkbox_verify->setChecked(FLACdropQtSettings.FLAC_Verify);

	// options - libmp3lame
	options_libmp3lame_slider_internalquality->setValue(FLACdropQtSettings.LAME_InternalQuality);
	options_libmp3lame_spinbox_internalquality->setValue(FLACdropQtSettings.LAME_InternalQuality);

	options_libmp3lame_slider_vbrquality->setValue(FLACdropQtSettings.LAME_VBRQuality);
	options_libmp3lame_spinbox_vbrquality->setValue(FLACdropQtSettings.LAME_VBRQuality);

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
}

// store app settings using QSettings
void FLACdropQt::writeSettings()
{
	// update the global settings with the actual widget values
	// main window
	for (int i = 0; i < FILE_TYPE_QUANTITY; i++)
		if (main_radiobutton_outputtypes[i]->isChecked() == true) FLACdropQtSettings.OUT_Type = i;

	// options - output
	FLACdropQtSettings.OUT_Threads = options_output_slider_threads->value();

	// options- libflac
	FLACdropQtSettings.FLAC_EncodingQuality = options_libflac_slider_quality->value();
	FLACdropQtSettings.FLAC_MD5check = options_libflac_checkbox_md5->isChecked();
	FLACdropQtSettings.FLAC_Verify = options_libflac_checkbox_verify->isChecked();

	// options - libmp3lame
	FLACdropQtSettings.LAME_InternalQuality = options_libmp3lame_slider_internalquality->value();
	FLACdropQtSettings.LAME_VBRQuality = options_libmp3lame_slider_internalquality->value();
	FLACdropQtSettings.LAME_CBRBitrate = options_libmp3lame_combobox_cbrbitrate->currentIndex();
	if (options_libmp3lame_radiobutton_cbr->isChecked() == true) FLACdropQtSettings.LAME_EncodingMode = LAME_ENCODINGMODE_CBR;
	if (options_libmp3lame_radiobutton_vbr->isChecked() == true) FLACdropQtSettings.LAME_EncodingMode = LAME_ENCODINGMODE_VBR;

	// store settings
	settings->setValue("OUT_Type", FLACdropQtSettings.OUT_Type);
	settings->setValue("OUT_Threads", FLACdropQtSettings.OUT_Threads);
	settings->setValue("FLAC_EncodingQuality", FLACdropQtSettings.FLAC_EncodingQuality);
	settings->setValue("FLAC_MD5check", FLACdropQtSettings.FLAC_MD5check);
	settings->setValue("FLAC_Verify", FLACdropQtSettings.FLAC_Verify);
	settings->setValue("LAME_InternalQuality", FLACdropQtSettings.LAME_InternalQuality);
	settings->setValue("LAME_VBRQuality", FLACdropQtSettings.LAME_VBRQuality);
	settings->setValue("LAME_CBRBitrate", FLACdropQtSettings.LAME_CBRBitrate);
	settings->setValue("LAME_EncodingMode", FLACdropQtSettings.LAME_EncodingMode);
}

// load an embedded resource
Resource::Resource(int resource_id, const LPCWSTR resource_class)
{
	hResource = FindResource(nullptr, MAKEINTRESOURCE(resource_id), resource_class);
	hMemory = LoadResource(nullptr, hResource);

	p.size_bytes = SizeofResource(nullptr, hResource);
	p.ptr = LockResource(hMemory);
}

LPVOID Resource::GetResourcePointer()
{
	return p.ptr;
}

DWORD Resource::GetResourceSize()
{
	return p.size_bytes;
}