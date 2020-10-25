#include "FLACdropQt.h"

// load app settings from ...
void FLACdropQt::readSettings()
{
	// load default values
	FLACdropQtSettings.OUT_Threads = OUT_THREADS;
	FLACdropQtSettings.OUT_Type = OUT_TYPE;
	FLACdropQtSettings.FLAC_EncodingQuality = FLAC_ENCODINGQUALITY;
	FLACdropQtSettings.FLAC_MD5check = FLAC_MD5CHECK;
	FLACdropQtSettings.FLAC_Verify = FLAC_VERIFY;
	FLACdropQtSettings.LAME_InternalQuality = LAME_INTERNALQUALITY;
	FLACdropQtSettings.LAME_VBRQuality = LAME_VBRQUALITY;
	FLACdropQtSettings.LAME_EncodingMode = LAME_ENCODINGMODE;
	FLACdropQtSettings.LAME_CBRBitrate = LAME_CBRBITRATE;
	FLACdropQtSettings.LAME_Flush = LAME_FLUSH;
	FLACdropQtSettings.LAME_NoGap = LAME_NOGAP;

	// setup the widget status'
	// main window
	layout_radiobutton_outputtypes[FLACdropQtSettings.OUT_Type]->setChecked(true);

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

// write app settings to ...
void FLACdropQt::writeSettings()
{
	// update the global settings with the actual widget values
	// main window
	for (int i = 0; i < FILE_TYPE_QUANTITY; i++)
		if (layout_radiobutton_outputtypes[i]->isChecked() == true) FLACdropQtSettings.OUT_Type = i;

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