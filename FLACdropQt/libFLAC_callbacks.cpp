#include "encoders.h"

//---------------------------------------------------------------------------------
// libflac encoder callbacks
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
	if (_fseeki64(file_, absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

::FLAC__StreamEncoderTellStatus libflac_StreamEncoder::tell_callback(FLAC__uint64* absolute_byte_offset)
{
	long long pos;

	if ((pos = _ftelli64(file_)) < 0)
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

//---------------------------------------------------------------------------------
// libflac decoder callbacks
//---------------------------------------------------------------------------------
void libflac_StreamDecoder::addOutputFile(FILE* f)
{
	client_data.fout = f;
}

unsigned int libflac_StreamDecoder::get_bits_per_sample_clientdata()
{
	return client_data.bps;
}

unsigned int libflac_StreamDecoder::get_blocksize_clientdata()
{
	return client_data.blocksize;
}

::FLAC__StreamDecoderReadStatus libflac_StreamDecoder::read_callback(FLAC__byte buffer[], size_t* bytes)
{
	if (*bytes > 0)
	{
		*bytes = fread(buffer, sizeof(FLAC__byte), *bytes, file_);
		if (ferror(file_))
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		else if (*bytes == 0)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
	else
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT; // abort to avoid a deadlock
}

::FLAC__StreamDecoderSeekStatus libflac_StreamDecoder::seek_callback(FLAC__uint64 absolute_byte_offset)
{
	if (file_ == stdin)
		return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
	else if (_fseeki64(file_, (off_t)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

::FLAC__StreamDecoderTellStatus libflac_StreamDecoder::tell_callback(FLAC__uint64* absolute_byte_offset)
{
	long long pos;

	if (file_ == stdin)
		return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
	else if ((pos = _ftelli64(file_)) < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}
}

::FLAC__StreamDecoderLengthStatus libflac_StreamDecoder::length_callback(FLAC__uint64* stream_length)
{
	struct __stat64 filestats;

	if (file_ == stdin)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
	else if (_fstat64(_fileno(file_), &filestats) != 0)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	else {
		*stream_length = (FLAC__uint64)filestats.st_size;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

bool libflac_StreamDecoder::eof_callback()
{
	return feof(file_) ? true : false;
}

::FLAC__StreamDecoderWriteStatus libflac_StreamDecoder::write_callback(const ::FLAC__Frame* frame, const FLAC__int32* const buffer[])
{
	UNREFERENCED_PARAMETER(frame);
	FILE* f = client_data.fout;

	const FLAC__uint32 total_size = (FLAC__uint32)(client_data.total_samples * client_data.channels * (client_data.bps / 8));
	size_t i;
	unsigned int chan, pos;
	FLAC__int32 temp;
	FLAC__byte* pcm;

	// write WAVE header before writing the first frame
	if (frame->header.number.sample_number == 0)
	{
		sWAVEheader WAVEheader;
		sFMTheader FMTheader;
		sDATAheader DATAheader;

		// fill up the WAVE file headers
		WAVEheader.ChunkID[0] = 'R';
		WAVEheader.ChunkID[1] = 'I';
		WAVEheader.ChunkID[2] = 'F';
		WAVEheader.ChunkID[3] = 'F';
		WAVEheader.ChunkSize = 36 + total_size;	// This is the size of the entire file in bytes minus 8 bytes for the two fields not included in this count: ChunkID and ChunkSize.
		WAVEheader.Format[0] = 'W';
		WAVEheader.Format[1] = 'A';
		WAVEheader.Format[2] = 'V';
		WAVEheader.Format[3] = 'E';
		FMTheader.ChunkID[0] = 'f';
		FMTheader.ChunkID[1] = 'm';
		FMTheader.ChunkID[2] = 't';
		FMTheader.ChunkID[3] = ' ';
		FMTheader.ChunkSize = 16;	// size of the format subchunk
		FMTheader.AudioFormat = WAV_FORMAT_PCM;
		FMTheader.NumChannels = client_data.channels;
		FMTheader.SampleRate = client_data.sample_rate;
		FMTheader.ByteRate = client_data.sample_rate * client_data.channels * (client_data.bps / 8);	// SampleRate * NumChannels * BitsPerSample/8
		FMTheader.BlockAlign = client_data.channels * (client_data.bps / 8);	// NumChannels * BitsPerSample/8
		FMTheader.BitsPerSample = client_data.bps;
		DATAheader.ChunkID[0] = 'd';
		DATAheader.ChunkID[1] = 'a';
		DATAheader.ChunkID[2] = 't';
		DATAheader.ChunkID[3] = 'a';
		DATAheader.ChunkSize = total_size;

		i = fwrite(&WAVEheader, 1, 12, f);
		if (i != 12) return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;	// write error
		i = fwrite(&FMTheader, 1, 24, f);
		if (i != 24) return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		i = fwrite(&DATAheader, 1, 8, f);
		if (i != 8) return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	// write decoded PCM samples
	switch (client_data.bps)
	{
	case 16:	pcm = new FLAC__byte[frame->header.blocksize * client_data.channels * 2];
		for (chan = 0; chan < client_data.channels; chan++)
		{
			for (pos = 0; pos < frame->header.blocksize; pos++)
			{
				temp = buffer[chan][pos];
				pcm[client_data.channels * pos * 2 + chan * 2] = (FLAC__byte)temp;
				temp = temp >> 8;
				pcm[client_data.channels * pos * 2 + chan * 2 + 1] = (FLAC__byte)temp;
			}
		}
		i = fwrite(pcm, client_data.channels * 2, frame->header.blocksize, f);
		delete[]pcm;
		if (i != frame->header.blocksize) return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		break;

	case 24:	pcm = new FLAC__byte[frame->header.blocksize * client_data.channels * 3];
		for (chan = 0; chan < get_channels(); chan++)
		{
			for (pos = 0; pos < frame->header.blocksize; pos++)
			{
				temp = buffer[chan][pos];
				pcm[client_data.channels * pos * 3 + chan * 3] = (FLAC__byte)temp;
				temp = temp >> 8;
				pcm[client_data.channels * pos * 3 + chan * 3 + 1] = (FLAC__byte)temp;
				temp = temp >> 8;
				pcm[client_data.channels * pos * 3 + chan * 3 + 2] = (FLAC__byte)temp;
			}
		}
		i = fwrite(pcm, client_data.channels * 3, frame->header.blocksize, f);
		delete[]pcm;
		if (i != frame->header.blocksize) return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		break;
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void libflac_StreamDecoder::metadata_callback(const ::FLAC__StreamMetadata* metadata)
{
	// save WAVE metadata for other callback processes
	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	{
		client_data.total_samples = metadata->data.stream_info.total_samples;
		client_data.sample_rate = metadata->data.stream_info.sample_rate;
		client_data.channels = metadata->data.stream_info.channels;
		client_data.bps = metadata->data.stream_info.bits_per_sample;
		client_data.blocksize = metadata->data.stream_info.max_blocksize;
	}
}

void libflac_StreamDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
	// intentionally kept blank
}