#include "encoders.h"

//---------------------------------------------------------------------------------
// Encoder thread scheduler definitions
//---------------------------------------------------------------------------------
cScheduler::cScheduler(QApplication* gui_thread)
{
	gui_window = gui_thread;

	connect(this, SIGNAL(setDrop(bool)), gui_window, SLOT(setDrop(bool)));

	// allocate each encoder object and connect signals to slots
	for (int i = 0; i < OUT_MAX_THREADS; i++)
	{
		encoder_list[i] = new encoders();
		encoder_list[i]->setID(i);

		connect(encoder_list[i], SIGNAL(setProgressbarLimits(int, int, int)), gui_window, SLOT(setProgressbarLimits(int, int, int)));
		connect(encoder_list[i], SIGNAL(setProgressbarValue(int, int)), gui_window, SLOT(setProgressbarValue(int, int)));
		connect(encoder_list[i], SIGNAL(ThreadFinished(int)), this, SLOT(ThreadFinished(int)));
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

void cScheduler::ThreadFinished(int ID)
{
	thread_status[ID] = false;
}

// run when drag&drop is initiated
void cScheduler::startEncoding()
{
	int parallelthreads, allowedthreads;
	bool threadStarted;

	// setup each encoder and reset the progressbars
	for (int i = 0; i < OUT_MAX_THREADS; i++)
	{
		encoder_list[i]->addEncoderSettings(ActualSettings);
		emit setProgressbarValue(i, 0);
		thread_status[i] = false;
	}

	// we first start the max allowed number of threads and after that wait for the thread finished signals to start a new thread
	// if the number of the dropped files are less than the number of available threads then we run the loop only for the number of dropped files
	ActualSettings.OUT_Threads > pathList.size() ? allowedthreads = pathList.size() : allowedthreads = ActualSettings.OUT_Threads;
	pathlistPosition = 0;
	parallelthreads = 0;

	while ((parallelthreads < allowedthreads) && (pathlistPosition < pathList.size()))
	{
		threadStarted = SelectEncoder(pathList.at(pathlistPosition));
		pathlistPosition++;
		if (threadStarted == true) parallelthreads++;
	}

	// check if at least a thread is running, if not then enable drag&drop
	threadStarted = false;
	for (int i = 0; i < OUT_MAX_THREADS; i++)
		if (thread_status[i] == true) threadStarted = true;
	if (threadStarted == false) emit setDrop(true);
}

// run when a finished thread signal was emitted
void cScheduler::startNewThread()
{
	bool threadStarted = false;

	while ((threadStarted == false) && (pathlistPosition < pathList.size()))
	{
		threadStarted = SelectEncoder(pathList.at(pathlistPosition));
		pathlistPosition++;
	}
	
	if (pathlistPosition == pathList.size()) emit setDrop(true);	// we were not able to start a new thread so we can enable drag&drop
}

// TODO: check only the actual extension of the file, do not search the entire path string
// choose which encoder to run based on the path string
bool cScheduler::SelectEncoder(const QString& droppedfile)
{
	bool threadStarted = false;

	// dropped file is WAV
	if (droppedfile.contains(".wav", Qt::CaseInsensitive))
	{
		int thread_slot = 0;
		while (thread_status[thread_slot] == true) thread_slot++;		// search for an available encoder
		
		encoder_list[thread_slot]->addFile(droppedfile);
		encoder_list[thread_slot]->setInputFileType(FILE_TYPE_WAV);
		thread_status[thread_slot] = true;
		encoder_list[thread_slot]->start();

		threadStarted = true;
	}

	// dropped file is FLAC
	if (droppedfile.contains(".flac", Qt::CaseInsensitive))
	{
		int thread_slot = 0;
		while (thread_status[thread_slot] == true) thread_slot++;		// search for an available encoder
		
		encoder_list[thread_slot]->addFile(droppedfile);
		encoder_list[thread_slot]->setInputFileType(FILE_TYPE_FLAC);
		thread_status[thread_slot] = true;
		encoder_list[thread_slot]->start();

		threadStarted = true;
	}

	return threadStarted;
}

//---------------------------------------------------------------------------------
// Encoders class definitions
//---------------------------------------------------------------------------------
void encoders::run()
{
	// choose which encoder should be used
	switch (settings.OUT_Type)
	{
		case FILE_TYPE_FLAC:
			if (inputFileType == FILE_TYPE_WAV) wav2flac();
			break;
		
		case FILE_TYPE_WAV:
			if (inputFileType == FILE_TYPE_FLAC) flac2wav();
			break;
		
		case FILE_TYPE_MP3:
			if (inputFileType == FILE_TYPE_FLAC) flac2mp3();
			if (inputFileType == FILE_TYPE_WAV) wav2mp3();
			break;
	}
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

//---------------------------------------------------------------------------------
// Encoder algorithm: WAV -> FLAC
//---------------------------------------------------------------------------------
void encoders::wav2flac()
{
	libflac_StreamEncoder* encoder = NULL;
	FILE* fin, *fout;
	int err = ALL_OK;

	// WAVE file data structures
	sWAVEheader WAVEheader;
	sFMTheader FMTheader;
	sDATAheader DATAheader;
	unsigned int total_samples = 0;		// can use a 32-bit number due to WAV file size limitation in the specification

	// WAV: open the input WAVE file
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
			if (FMTheader.SubFormat_AudioFormat != WAV_FORMAT_PCM)
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

	// libFLAC: allocate the libFLAC encoder
	if (err == ALL_OK)
	{
		encoder = new libflac_StreamEncoder();
		if (encoder->is_valid() == false)
		{
			fclose(fin);
			delete encoder;
			err = FAIL_LIBFLAC_ALLOC;
		}
	}

	// libFLAC: set the encoder parameters
	if (err == ALL_OK)
	{
		bool ok = true;

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
			encoder->file_ = fout;
			if (encoder->init() != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
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

	// libFLAC: read blocks of samples from WAVE file and feed to the encoder
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
				
				ok = encoder->process_interleaved(buffer_flac, need);	// feed samples to the encoder
				if (ok == false) err = FAIL_LIBFLAC_ENCODE;
				
				emit setProgressbarValue(ID, ProgressBarPos++);	// increase the progress bar
			}
			left -= need;
		}
		delete[]buffer_wav;
		delete[]buffer_flac;
	}

	// libFLAC: close the encoder
	if (err == ALL_OK)
	{
		if (encoder->finish() == false) err = FAIL_LIBFLAC_RELEASE;
		// now that encoding is finished, the metadata can be freed
		//FLAC__metadata_object_delete(metadata[0]);
		//FLAC__metadata_object_delete(metadata[1]);
		fclose(fin);
		fclose(fout);
		delete encoder;
	}

	emit ThreadFinished(ID);
}

//---------------------------------------------------------------------------------
// Encoder algorithm: FLAC -> WAV
//---------------------------------------------------------------------------------
void encoders::flac2wav()
{
	libflac_StreamDecoder* decoder = NULL;
	FILE* fin, * fout;
	int err = ALL_OK;

	// libFLAC: allocate the decoder
	{
		decoder = new libflac_StreamDecoder();
		if (decoder->is_valid() == false)
		{
			delete decoder;
			err = FAIL_LIBFLAC_ALLOC;
		}
	}

	// libFLAC: set decoder parameters
	if (err == ALL_OK)
	{
		decoder->set_md5_checking(settings.FLAC_MD5check);
	}

	// libFLAC: open the input FLAC file
	if (err == ALL_OK)
	{
		WCHAR* wchar_infile;

		wchar_infile = new WCHAR[MAXFILENAMELENGTH];
		infile.toWCharArray(wchar_infile);
		wchar_infile[infile.length()] = 0;	// must add the closing 0 to have correctly prepared wchar array
		if ((_wfopen_s(&fin, wchar_infile, L"rb")) != NULL)
		{
			delete decoder;
			err = FAIL_FILE_OPEN;
		}
		else
		{
			decoder->file_ = fin;
		}

		delete[]wchar_infile;
	}

	// libFLAC: initialize decoder
	if (err == ALL_OK)
	{
		if (decoder->init() != FLAC__STREAM_DECODER_INIT_STATUS_OK)
		{
			fclose(fin);
			delete decoder;
			err = FAIL_LIBFLAC_ALLOC;
		}
	}

	// libFLAC: check the FLAC file parameters
	if (err == ALL_OK)
	{
		// check if the first block really contained the metadata
		if (decoder->process_until_end_of_metadata() == false)
		{
			fclose(fin);
			delete decoder;
			err = FAIL_LIBFLAC_DECODE;
		}
	}
	if (err == ALL_OK)
	{
		// check if FLAC file has 16 or 24 bit resolution
		switch (decoder->get_bits_per_sample_clientdata())
		{
		case 16:
		case 24:
			break;
		default:
			fclose(fin);
			delete decoder;
			err = FAIL_LIBFLAC_ONLY_16_24_BIT;
		}
	}
	if (err == ALL_OK)
	{
		// check if total_samples count is in the STREAMINFO
		if (decoder->get_total_samples() == 0)
		{
			fclose(fin);
			delete decoder;
			err = FAIL_LIBFLAC_BAD_HEADER;
		}
	}

	// libFLAC: open output file
	if (err == ALL_OK)
	{
		WCHAR* wchar_infile, * wchar_outfile;

		// open the output file
		wchar_infile = new WCHAR[MAXFILENAMELENGTH];
		infile.toWCharArray(wchar_infile);
		wchar_infile[infile.length()] = 0;	// must add the closing 0 to have correctly prepared wchar array

		wchar_outfile = new WCHAR[MAXFILENAMELENGTH];
		wcsncpy_s(wchar_outfile, MAXFILENAMELENGTH, wchar_infile, wcsnlen(wchar_infile, MAXFILENAMELENGTH) - 4);	// leave out the "flac" from the end
		wcscat_s(wchar_outfile, MAXFILENAMELENGTH, L"wav");
		if ((_wfopen_s(&fout, wchar_outfile, L"w+b")) != NULL)
		{
			fclose(fin);
			delete decoder;
			err = FAIL_LIBFLAC_DECODE;
		}
		else decoder->addOutputFile(fout);

		delete[]wchar_outfile;
		delete[]wchar_infile;
	}

	// libFLAC: start the decoding
	if (err == ALL_OK)
	{
		bool ok;
		FLAC__StreamDecoderState state;
		int ProgressBarPos = 0;

		// set up the progress bar boundaries, block size is around 4k depending on resolution
		emit setProgressbarLimits(ID, 0, decoder->get_total_samples() / decoder->get_blocksize_clientdata());
		emit setProgressbarValue(ID, 0);

		// loop the decoder until it reaches the end of the input file or returns an error
		do
		{
			ok = decoder->process_single();
			state = decoder->get_state();
			emit setProgressbarValue(ID, ProgressBarPos++);	// increase the progress bar
		} while ((state != FLAC__STREAM_DECODER_END_OF_STREAM && FLAC__STREAM_DECODER_SEEK_ERROR && FLAC__STREAM_DECODER_ABORTED && FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR) && ok == true);
	}

	// libFLAC: close the decoder
	if (err == ALL_OK)
	{
		if (decoder->finish()== false) err = FAIL_LIBFLAC_RELEASE;
		fclose(fin);
		fclose(fout);
		delete decoder;
	}

	emit ThreadFinished(ID);
}

//---------------------------------------------------------------------------------
// Encoder algorithm: WAV -> MP3
//---------------------------------------------------------------------------------
void encoders::wav2mp3()
{
	lame_global_flags* lame_gfp;
	FILE* fin, * fout;
	int err = 0;

	unsigned int total_samples = 0;	// can use a 32-bit number due to WAV file size limitation
	sWAVEheader WAVEheader;
	sFMTheader FMTheader;
	sDATAheader DATAheader;

	// WAV: open the input WAVE file
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
			if (FMTheader.SubFormat_AudioFormat != WAV_FORMAT_PCM)
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

	// WAV: check if the WAVE file has 16 bit resolution, MP3 stream does not support 24 bit
	if (err == ALL_OK)
	{
		switch (FMTheader.BitsPerSample)
		{
		case 16:
			break;
		case 24:
		default:
			fclose(fin);
			err = FAIL_LAME_ONLY_16_BIT;
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

	// libmp3lame: initialize lame encoder
	if (err == ALL_OK)
	{
		lame_gfp = lame_init();
		if (lame_gfp == NULL)
		{
			fclose(fin);
			err = FAIL_LAME_INIT;
		}
	}

	// libmp3lame: set encoder parameters
	if (err == ALL_OK)
	{
		switch (FMTheader.NumChannels)
		{
		case 1:
			lame_set_mode(lame_gfp, MONO);
			break;
		case 2:
			lame_set_mode(lame_gfp, JOINT_STEREO);
			break;
		default:
			// only mono and stereo streams are supported by libmp3lame
			fclose(fin);
			err = FAIL_LAME_MAX_2_CHANNEL;
			break;
		}
	}
	if (err == ALL_OK)
	{
		// Internal algorithm selection. True quality is determined by the bitrate but this variable will effect quality by selecting expensive or cheap algorithms.
		// quality=0..9.  0=best (very slow).  9=worst.
		// recommended:  2     near-best quality, not too slow
		// 5     good quality, fast
		// 7     ok quality, really fast
		lame_set_quality(lame_gfp, settings.LAME_InternalQuality);

		// turn off automatic writing of ID3 tag data into mp3 stream we have to call it before 'lame_init_params', because that function would spit out ID3v2 tag data.
		lame_set_write_id3tag_automatic(lame_gfp, 0);

		// set lame encoder parameters for CBR encoding
		lame_set_num_channels(lame_gfp, FMTheader.NumChannels);
		lame_set_in_samplerate(lame_gfp, FMTheader.SampleRate);
		lame_set_brate(lame_gfp, LAME_CBRBITRATES[settings.LAME_CBRBitrate]);	// load encoding bitrate setting from LUT

		// set lame encoder parameters for VBR encoding
		switch (settings.LAME_EncodingMode)
		{
			case 0:	// CBR
				lame_set_VBR(lame_gfp, vbr_off);
				break;
			case 1:	// VBR
				lame_set_VBR(lame_gfp, vbr_mtrh);
				break;
		}

		lame_set_VBR_q(lame_gfp, settings.LAME_VBRQuality); // VBR quality level.  0=highest  9=lowest

		// now that all the options are set, lame needs to analyze them and set some more internal options and check for problems
		if (lame_init_params(lame_gfp) != 0)
		{
			fclose(fin);
			err = FAIL_LAME_INIT;
		}
	}

	// libmp3lame: open output file
	if (err == ALL_OK)
	{
		WCHAR* wchar_infile, * wchar_outfile;

		wchar_infile = new WCHAR[MAXFILENAMELENGTH];
		infile.toWCharArray(wchar_infile);
		wchar_infile[infile.length()] = 0;	// must add the closing 0 to have correctly prepared wchar array

		wchar_outfile = new WCHAR[MAXFILENAMELENGTH];
		wcsncpy_s(wchar_outfile, MAXFILENAMELENGTH, wchar_infile, wcsnlen(wchar_infile, MAXFILENAMELENGTH) - 3);	// leave out the "wav" from the end
		wcscat_s(wchar_outfile, MAXFILENAMELENGTH, L"mp3");

		if ((_wfopen_s(&fout, wchar_outfile, L"w+b")) != NULL)
		{
			fclose(fin);
			err = FAIL_FILE_OPEN;
		}

		delete[]wchar_outfile;
		delete[]wchar_infile;
	}

	// libmp3lame: start encoding
	if (err == ALL_OK)
	{
		bool ok = true;
		int ProgressBarPos = 0;
		BYTE* buffer_mp3, * buffer_wav;
		int imp3, owrite;
		size_t left, need;

		// set up the progress bar boundaries
		emit setProgressbarLimits(ID, 0, total_samples / READSIZE_MP3);
		emit setProgressbarValue(ID, 0);

		// allocate memory buffers
		buffer_wav = new BYTE[READSIZE_MP3 * FMTheader.NumChannels * (FMTheader.BitsPerSample / 8)];
		buffer_mp3 = new BYTE[LAME_MAXMP3BUFFER];

		/*size_t  id3v2_size;
		unsigned char *id3v2tag;

		id3v2_size = lame_get_id3v2_tag(lame_gfp, 0, 0);
		if (id3v2_size > 0)
		{
			id3v2tag = new unsigned char[id3v2_size];
			if (id3v2tag != 0)
			{
				imp3 = lame_get_id3v2_tag(lame_gfp, id3v2tag, id3v2_size);
				owrite = (int) fwrite(id3v2tag, 1, imp3, fout);
				delete []id3v2tag;
				if (owrite != imp3) return FAIL_LAME_ID3TAG;
			}
		}
		else
		{
			unsigned char* id3v2tag = getOldTag(gf);
			id3v2_size = sizeOfOldTag(gf);
			if ( id3v2_size > 0 )
			{
				size_t owrite = fwrite(id3v2tag, 1, id3v2_size, fout);
				if (owrite != id3v2_size) return FAIL_LAME_ID3TAG;
			}
		}
		if (settings.LAME_Flush == true) fflush(fout);*/

		// read blocks of samples from WAVE file and feed to the encoder
		left = (size_t)total_samples;
		while (ok && left)
		{
			need = (left > READSIZE_MP3 ? (size_t)READSIZE_MP3 : (size_t)left);	// calculate the number of samples to read
			if (fread(buffer_wav, (size_t)FMTheader.NumChannels * (FMTheader.BitsPerSample / 8), need, fin) != need)
			{
				// error during reading from WAVE file
				ok = false;
			}
			else
			{
				// feed samples to the encoder
				switch (FMTheader.NumChannels)
				{
				case 2:
					imp3 = lame_encode_buffer_interleaved(lame_gfp, (short int*)buffer_wav, need, buffer_mp3, LAME_MAXMP3BUFFER);
					break;
				case 1:	// the interleaved version corrupts the mono stream
					imp3 = lame_encode_buffer(lame_gfp, (short int*)buffer_wav, NULL, need, buffer_mp3, LAME_MAXMP3BUFFER);
					break;
				}

				// was our output buffer big enough?
				if (imp3 < 0) ok = false;
				else
				{
					owrite = (int)fwrite(buffer_mp3, 1, imp3, fout);
					if (owrite != imp3) ok = false;
					if (settings.LAME_Flush == true) fflush(fout);
				}

				emit setProgressbarValue(ID, ProgressBarPos++);	// increase the progress bar
			}
			left -= need;
		}

		// may return one more mp3 frame
		if (ok == true)
		{
			if (settings.LAME_NoGap == true) imp3 = lame_encode_flush_nogap(lame_gfp, buffer_mp3, LAME_MAXMP3BUFFER);
			else imp3 = lame_encode_flush(lame_gfp, buffer_mp3, LAME_MAXMP3BUFFER);
			if (imp3 < 0) ok = false;
		}

		if (ok == true)
		{
			owrite = (int)fwrite(buffer_mp3, 1, imp3, fout);
			if (owrite != imp3) ok = false;
		}

		if (settings.LAME_Flush == true && ok == true) fflush(fout);

		delete[]buffer_wav;
		delete[]buffer_mp3;
	
		if (ok == false)
		{
			fclose(fin);
			fclose(fout);
			err = FAIL_LAME_ENCODE;
		}
	}

	// libmp3lame: close encoder
	if (err == ALL_OK)
	{
		if (lame_close(lame_gfp) != 0) err = FAIL_LAME_CLOSE;
		fclose(fin);
		fclose(fout);
	}

	emit ThreadFinished(ID);
}

//---------------------------------------------------------------------------------
// Encoder algorithm: FLAC -> MP3
//---------------------------------------------------------------------------------
void encoders::flac2mp3()
{
	emit ThreadFinished(ID);
}