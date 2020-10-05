#include "encoders.h"

//---------------------------------------------------------------------------------
// Encoder thread scheduler definitions
//---------------------------------------------------------------------------------
cScheduler::cScheduler(QApplication* gui_thread)
{
	gui_window = gui_thread;

	connect(this, SIGNAL(setDrop(bool)), gui_window, SLOT(setDrop(bool)));

	// allocate each encoder object and connect signalst to slots
	for (int i = 0; i < OUT_MAX_THREADS; i++)
	{
		encoder_list[i] = new encoders();
		encoder_list[i]->setID(i);

		connect(encoder_list[i], SIGNAL(setProgressbarLimits(int, int, int)), gui_window, SLOT(setProgressbarLimits(int, int, int)));
		connect(encoder_list[i], SIGNAL(setProgressbarValue(int, int)), gui_window, SLOT(setProgressbarValue(int, int)));
		connect(encoder_list[i], SIGNAL(registerThreadFinished(int)), this, SLOT(registerThreadFinished(int)));
		connect(encoder_list[i], SIGNAL(finished()), this, SLOT(startNewThread()));
	}
}

void cScheduler::addPathList(const QStringList& droppedFiles)
{
	pathList = droppedFiles;
}

void cScheduler::addEncoderSettings(sFLACdropQtSettings encSettings)
{
	ActualSettings = encSettings;
}

void cScheduler::registerThreadFinished(int ID)
{
	thread_status[ID] = false;
}

void cScheduler::startEncoding()
{
	int firstbatch;

	// setup each encoder and reset the progressbars
	for (int i = 0; i < OUT_MAX_THREADS; i++)
	{
		encoder_list[i]->addEncoderSettings(ActualSettings);
		emit setProgressbarValue(i, 0);
		thread_status[i] = false;
	}

	// we first start the max allowed number of threads and after that wait for the thread finished signals to start a new thread
	// if the number of the dropped files are less than the number of available threads then we start threads only for the dropped files
	if (ActualSettings.OUT_Threads > pathList.size()) firstbatch = pathList.size();
	else firstbatch = ActualSettings.OUT_Threads;

	for (int i = 0; i < firstbatch; i++) startSingleThread(pathList.at(i));
	pathlistPosition = firstbatch;
}

// TODO: check only the actual extension of the file, do not search the entire path string
void cScheduler::startSingleThread(const QString& droppedfile)
{

	//encoder_list[thread_slot]->setID(thread_slot);

	// encode WAV to FLAC or MP3
	if (droppedfile.contains(".wav", Qt::CaseInsensitive))
	{
		int thread_slot = 0;
		while (thread_status[thread_slot] == true) thread_slot++;		// search for an available encoder
		
		encoder_list[thread_slot]->addFile(droppedfile);
		encoder_list[thread_slot]->setInputFileType(FILE_TYPE_WAV);
		thread_status[thread_slot] = true;
		encoder_list[thread_slot]->start();
	}

	// encode FLAC to MP3 or WAV
	if (droppedfile.contains(".flac", Qt::CaseInsensitive))
	{
		int thread_slot = 0;
		while (thread_status[thread_slot] == true) thread_slot++;		// search for an available encoder
		
		encoder_list[thread_slot]->addFile(droppedfile);
		encoder_list[thread_slot]->setInputFileType(FILE_TYPE_FLAC);
		thread_status[thread_slot] = true;
		encoder_list[thread_slot]->start();
	}
}

void cScheduler::startNewThread()
{
	if (pathlistPosition < pathList.size())
	{
		startSingleThread(pathList.at(pathlistPosition));
		pathlistPosition++;
	}
	else
	{
		// all files have been encoded so now we can accept new drops
		emit setDrop(true);
	}
}

//---------------------------------------------------------------------------------
// Encoder class definitions
//---------------------------------------------------------------------------------
encoders::encoders()
{
	// only to prevent compiler warning
	inputFileType = 0;
	ID = 0;
}

void encoders::addEncoderSettings(sFLACdropQtSettings encparams)
{
	settings = encparams;
}

void encoders::setID(int thread_ID)
{
	ID = thread_ID;
}

void encoders::addFile(QString encfile)
{
	infile = encfile;
}

void encoders::setInputFileType(int in)
{
	inputFileType = in;
}

void encoders::run()
{
	// choose which encoder should be used
	switch (settings.OUT_Type)
	{
		case FILE_TYPE_FLAC:
			switch (inputFileType)
			{
				case FILE_TYPE_WAV:
					wav2flac();
					break;
				case FILE_TYPE_MP3:
					break;
			}
			break;
		
		case FILE_TYPE_MP3:
			break;
		
		case FILE_TYPE_WAV:
			break;
	}
}

//---------------------------------------------------------------------------------
// Encoder algorithms
//---------------------------------------------------------------------------------
void encoders::wav2flac()
{
	FLAC::Encoder::Stream* encoder = NULL;
	FILE* fin, *fout;
	int err = ALL_OK;

	// WAVE file data structures
	sWAVEheader WAVEheader;
	sFMTheader FMTheader;
	sDATAheader DATAheader;
	unsigned int total_samples = 0;		// can use a 32-bit number due to WAV file size limitation in the specification

	// WAV: open the wave file
	{
		WCHAR* wchar_infile;

		wchar_infile = new WCHAR[MAXFILENAMELENGTH];
		infile.toWCharArray(wchar_infile);
		wchar_infile[infile.length()] = 0;	// must add the closing 0 to have correctly prepared wchar array
		if ((_wfopen_s(&fin, wchar_infile, L"rb")) != NULL)
		{
			err = FAIL_FILE_OPEN;
		}
		delete[]wchar_infile;
	}
	
	// WAV: read header and check if it is valid
	if (err == ALL_OK)
	{
		if (fread(&WAVEheader, 1, 12, fin) != 12)
		{
			fclose(fin);
			err = FAIL_FILE_OPEN;
		}
	}
	if (err == ALL_OK)
	{
		if (memcmp(WAVEheader.ChunkID, "RIFF", 4) || memcmp(WAVEheader.Format, "WAVE", 4))
		{
			fclose(fin);
			err = FAIL_WAV_BAD_HEADER;
		}
	}

	// WAV: read the format chunk's header only to get its chunk size
	if (err == ALL_OK)
	{
		if (fread(&DATAheader, 1, 8, fin) != 8)
		{
			fclose(fin);
			err = FAIL_WAV_BAD_HEADER;
		}
	}

	// WAV: read the complete wave file header according to its actual chunk size (16, 18 or 40 byte), ChunkSize does not include the size of the header
	if (err == ALL_OK)
	{
		fseek(fin, -8, SEEK_CUR);
		if ((fread(&FMTheader, 1, (size_t)DATAheader.ChunkSize + 8, fin) != (size_t)DATAheader.ChunkSize + 8))
		{
			fclose(fin);
			err = FAIL_WAV_BAD_HEADER;
		}
	}

	// WAV: check if the wav file has PCM uncompressed data
	if (err == ALL_OK)
	{
		switch (FMTheader.AudioFormat)
		{
		case WAV_FORMAT_PCM:
			break;
		case WAV_FORMAT_EXTENSIBLE:
			// in this case the first two byte of the SubFormat is defining the audio format
			if (FMTheader.SubFormat_AudioFormat == WAV_FORMAT_PCM)
			{
				break;
			}
			else
			{
				fclose(fin);
				err = FAIL_WAV_UNSUPPORTED;
			}
			break;
		default:
			fclose(fin);
			err = FAIL_WAV_UNSUPPORTED;
		}
	}

	// WAV: check if the file has 16 or 24 bit resolution
	if (err == ALL_OK)
	{
		switch (FMTheader.BitsPerSample)
		{
		case 16:
		case 24:
			break;
		default:
			fclose(fin);
			err = FAIL_LIBFLAC_ONLY_16_24_BIT;
		}
	}
	
	// WAV: search for the data chunk
	if (err == ALL_OK)
	{
		do
		{
			fread(&DATAheader, 1, 8, fin);
			fseek(fin, DATAheader.ChunkSize, SEEK_CUR);
		} while (memcmp(DATAheader.ChunkID, "data", 4));
		fseek(fin, -DATAheader.ChunkSize, SEEK_CUR);													// go back to the beginning of the data chunk
		total_samples = DATAheader.ChunkSize / FMTheader.NumChannels / (FMTheader.BitsPerSample / 8);	// sound data's size divided by one sample's size
	}

	// libFLAC: allocate the libFLAC encoder and data buffers
	if (err == ALL_OK)
	{
		bool ok = TRUE;

		encoder = new libflac_StreamEncoder();
		ok = encoder->is_valid();
		if (ok == false)
		{
			fclose(fin);
			delete encoder;
			err = FAIL_LIBFLAC_ALLOC;
		}
	}

	// libFLAC: set the encoder parameters
	if (err == ALL_OK)
	{
		bool ok = TRUE;

		ok &= encoder->set_verify(settings.FLAC_Verify);
		ok &= encoder->set_compression_level(settings.FLAC_EncodingQuality);
		ok &= encoder->set_channels(FMTheader.NumChannels);
		ok &= encoder->set_bits_per_sample(FMTheader.BitsPerSample);
		ok &= encoder->set_sample_rate(FMTheader.SampleRate);
		ok &= encoder->set_total_samples_estimate(total_samples);
		if (ok == false)
		{
			fclose(fin);
			delete encoder;
			err = FAIL_LIBFLAC_ALLOC;
		}
	}

	// now add some metadata; we'll add some tags and a padding block
//	if(err == ALL_OK)
//	{
//		FLAC__StreamMetadata *metadata[2];
//		FLAC__StreamMetadata_VorbisComment_Entry entry;

//		if((metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
//			(metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
			// there are many tag (vorbiscomment) functions but these are convenient for this particular use:
//			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Some Artist") ||
//			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false) || // copy=false: let metadata object take control of entry's allocated string
//			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "1984") ||
//			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false))
//		{
			// out of memory or tag error
//			ok = false;
//			delete encoder;
//			err = FAIL_LIBFLAC_METADATA;
//		}
//		metadata[1]->length = 1234;	// set the padding length
//		ok = FLAC__stream_encoder_set_metadata(encoder, metadata, 2);
//	}

	// libFLAC: initialize the encoder
	if (err == ALL_OK)
	{
		// generate the output file name from the input file name		
		WCHAR *wchar_infile, *wchar_outfile;
		
		wchar_infile = new WCHAR[MAXFILENAMELENGTH];
		infile.toWCharArray(wchar_infile);
		wchar_infile[infile.length()] = 0;	// must add the closing 0 to have correctly prepared wchar array

		wchar_outfile = new WCHAR[MAXFILENAMELENGTH];
		wcsncpy_s(wchar_outfile, MAXFILENAMELENGTH, wchar_infile, wcsnlen(wchar_infile, MAXFILENAMELENGTH) - 3);	// leave out the "wav" from the end
		wcscat_s(wchar_outfile, MAXFILENAMELENGTH, L"flac");

		if ((_wfopen_s(&fout, wchar_outfile, L"w+b")) != NULL)
		{
			fclose(fin);
			delete encoder;
			err = FAIL_FILE_OPEN;
		}
		else
		{
			FLAC__StreamEncoderInitStatus init_status;

			dynamic_cast<libflac_StreamEncoder*>(encoder)->file_ = fout;
			init_status = encoder->init();

			if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
			{
				fclose(fin);
				fclose(fout);
				delete encoder;
				err = FAIL_LIBFLAC_ALLOC;
			}
		}

		delete[]wchar_outfile;
		delete[]wchar_infile;
	}

	// read blocks of samples from WAVE file and feed to the encoder
	if (err == ALL_OK)
	{
		size_t left, need, i;
		FLAC__byte* buffer_wav;				// READSIZE * bytes per sample * channels, we read the WAVE data here
		FLAC__int32* buffer_flac;			// READSIZE * channels
		int ProgressBarPos = 0;
		bool ok = true;

		buffer_wav = new FLAC__byte[READSIZE_FLAC * FMTheader.NumChannels * (FMTheader.BitsPerSample / 8)];
		buffer_flac = new FLAC__int32[READSIZE_FLAC * FMTheader.NumChannels];

		// set up the progress bar boundaries and reset it
		emit setProgressbarLimits(ID, 0, total_samples / READSIZE_FLAC);
		emit setProgressbarValue(ID, 0);

		left = (size_t)total_samples;
		while (ok && left)
		{
			need = (left > READSIZE_FLAC ? (size_t)READSIZE_FLAC : (size_t)left);	// calculate the number of samples to read
			if (fread(buffer_wav, (size_t)FMTheader.NumChannels * (FMTheader.BitsPerSample / 8), need, fin) != need)
			{
				// error during reading from WAVE file
				ok = false;
			}
			else
			{
				// convert the packed little-endian PCM samples from WAVE file into an interleaved FLAC__int32 buffer for libFLAC
				switch (FMTheader.BitsPerSample)
				{
				case 16:
					for (i = 0; i < need * FMTheader.NumChannels; i++)
					{
						// convert the 16 bit values stored in byte array into 32 bit signed integer values
						buffer_flac[i] = buffer_wav[2 * i + 1] << 8;
						buffer_flac[i] |= buffer_wav[2 * i];
						if (buffer_flac[i] & 0x8000) buffer_flac[i] |= 0xffff0000; // adjust the top 16 bit to have correct 2nd complement code
					}
					break;
				case 24:
					for (i = 0; i < need * FMTheader.NumChannels; i++)
					{
						// convert the 24 bit values stored in byte array into 32 bit signed integer values
						buffer_flac[i] = buffer_wav[3 * i + 2] << 16;
						buffer_flac[i] |= buffer_wav[3 * i + 1] << 8;
						buffer_flac[i] |= buffer_wav[3 * i];
						if (buffer_flac[i] & 0x800000) buffer_flac[i] |= 0xff000000; // adjust the top 8 bit to have correct 2nd complement code
					}
					break;
				}

				// feed samples to the encoder
				ok = encoder->process_interleaved(buffer_flac, need);
				if (ok == false) err = FAIL_LIBFLAC_ENCODE;
				// increase the progress bar
				emit setProgressbarValue(ID, ProgressBarPos++);
			}
			left -= need;
		}
		delete[]buffer_wav;
		delete[]buffer_flac;
	}
			
	if (err == ALL_OK)
	{
		bool ok;

		ok = encoder->finish();
		if (ok == false) err = FAIL_LIBFLAC_RELEASE;
		// now that encoding is finished, the metadata can be freed
		//FLAC__metadata_object_delete(metadata[0]);
		//FLAC__metadata_object_delete(metadata[1]);
		delete encoder;
		fclose(fin);
		fclose(fout);
	}

	emit registerThreadFinished(ID);
}

//---------------------------------------------------------------------------------
// libflac callbacks
//---------------------------------------------------------------------------------
::FLAC__StreamEncoderReadStatus libflac_StreamEncoder::read_callback(FLAC__byte buffer[], size_t* bytes)
{
	if (*bytes > 0) {
		*bytes = fread(buffer, sizeof(FLAC__byte), *bytes, file_);
		if (ferror(file_))
			return ::FLAC__STREAM_ENCODER_READ_STATUS_ABORT;
		else if (*bytes == 0)
			return ::FLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM;
		else
			return ::FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE;
	}
	else
		return ::FLAC__STREAM_ENCODER_READ_STATUS_ABORT;
}

::FLAC__StreamEncoderWriteStatus libflac_StreamEncoder::write_callback(const FLAC__byte buffer[], size_t bytes, uint32_t samples, uint32_t current_frame)
{
	(void)samples, (void)current_frame;

	if (fwrite(buffer, 1, bytes, file_) != bytes)
		return ::FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	else
		return ::FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

::FLAC__StreamEncoderSeekStatus libflac_StreamEncoder::seek_callback(FLAC__uint64 absolute_byte_offset)
{
	/*if (layer_ == LAYER_STREAM)
		return ::FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;
	else*/ if (_fseeki64(file_, absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

::FLAC__StreamEncoderTellStatus libflac_StreamEncoder::tell_callback(FLAC__uint64* absolute_byte_offset)
{
	long long pos;
	/*if (layer_ == LAYER_STREAM)
		return ::FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED;
	else*/ if ((pos = _ftelli64(file_)) < 0)
		return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
	}
}

void libflac_StreamEncoder::metadata_callback(const ::FLAC__StreamMetadata* metadata)
{
	(void)metadata;
}