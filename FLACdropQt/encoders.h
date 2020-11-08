#include "FLAC++\encoder.h"
#include "FLAC++\decoder.h"
#include "FLAC++\metadata.h"
#include "lame.h"

#include <QtWidgets>

// file formats handled by the app
#define FILE_TYPE_FLAC		0
#define FILE_TYPE_MP3		1
#define FILE_TYPE_WAV		2
#define FILE_TYPE_QUANTITY	3

// encoder internal buffer sizes
#define READSIZE_FLAC 1048576		// size of a block to read from disk in bytes for the FLAC encoder algorithm
#define READSIZE_MP3 8192			// size of a block to read from disk in bytes for the MP3 encoder algorithm
#define SIZE_RAW_BUFFER 32768		// size of raw audio data buffer for transcoding
#define MAXMETADATA 1024			// maximum character size of metadata string
#define MAXFILENAMELENGTH 1024		// maximum size of a file name with full path
//#define EVENTLOGSIZE 65536			// maximum character size of the event log

#define OUT_MAX_THREADS 8			// maximum number of batch processing threads

// failure codes
#define ALL_OK						0
#define FAIL_FILE_OPEN				1
#define FAIL_WAV_BAD_HEADER			2
#define FAIL_WAV_UNSUPPORTED		3
#define FAIL_LIBFLAC_ONLY_16_24_BIT	4
#define FAIL_LIBFLAC_BAD_HEADER		5
#define FAIL_LIBFLAC_ALLOC			6
#define FAIL_LIBFLAC_ENCODE			7
#define FAIL_LIBFLAC_DECODE			8
#define FAIL_LIBFLAC_METADATA		9
#define FAIL_LIBFLAC_RELEASE		10
#define WARN_LIBFLAC_MD5			11
#define FAIL_REGISTRY_OPEN			12
#define FAIL_REGISTRY_WRITE			13
#define FAIL_REGISTRY_READ			14
#define FAIL_LAME_ONLY_16_BIT		15
#define FAIL_LAME_MAX_2_CHANNEL		16
#define FAIL_LAME_INIT				17
#define FAIL_LAME_ID3TAG			18
#define FAIL_LAME_ENCODE			19
#define FAIL_LAME_CLOSE				20

// WAVE file audio formats
// http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
#define WAV_FORMAT_PCM			0x0001	// PCM
#define WAV_FORMAT_IEEE_FLOAT	0x0003	// IEEE float
#define WAV_FORMAT_ALAW			0x0006	// 8 - bit ITU - T G.711 A - law
#define WAV_FORMAT_MULAW		0x0007	// 8 - bit ITU - T G.711 µ - law
#define WAV_FORMAT_EXTENSIBLE	0xFFFE	// Determined by SubFormat

// positions of the metadata variables in the transfer structure
#define MD_NUMBER		16			// number of metadata fields
#define MD_TITLE		0
#define MD_VERSION		1
#define MD_ALBUM		2
#define MD_TRACKNUMBER	3
#define MD_DISCNUMBER	4
#define MD_ARTIST		5
#define MD_PERFORMER	6
#define MD_COPYRIGHT	7
#define MD_LICENSE		8
#define MD_ORGANIZATION	9
#define MD_DESCRIPTION	10
#define MD_GENRE		11
#define MD_DATE			12
#define MD_LOCATION		13
#define MD_CONTACT		14
#define MD_ISRC			15

// for libflac decoder
#define RENDERTARGET_MEM 0
#define RENDERTARGET_FILE 1

// libmp3lame CBR encoding bitrates
const QString LAME_CBRBITRATES_TEXT[] = {
	"48", "64", "80", "96", "112", "128", "160", "192", "224", "256", "320" };
const int LAME_CBRBITRATES[] = {
	48, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
#define LAME_CBRBITRATES_QUANTITY 11	// quantity of items in "LAME_CBRBITRATES" array

struct sFLACdropQtSettings
{
	bool FLAC_Verify;
	bool FLAC_MD5check;
	int FLAC_EncodingQuality;			// 1..8
	int LAME_CBRBitrate;				// 0..LAME_CBRBITRATES_QUANTITY
	int LAME_VBRQuality;				// 0..9
	int LAME_InternalQuality;			// 0..9
	int LAME_EncodingMode;				// 0: CBR; 1: VBR
	bool LAME_Flush;
	bool LAME_NoGap;
	int OUT_Type;						// 0: FLAC; 1: MP3; 2: WAV
	int OUT_Threads;					// 1..MAX_THREADS
};

//---------------------------------------------------------------------------------
// Wrapper for encoder algorithms
//---------------------------------------------------------------------------------
class encoders : public QThread
{
	Q_OBJECT

public:
	encoders(){}
	void addFile(QString encfile);
	void addEncoderSettings(sFLACdropQtSettings encparams);
	void setInputFileType(int in);
	void setID(int thread_id);
	void run();

signals:
	void setProgressbarLimits(int ID, int min, int max);
	void setProgressbarValue(int ID, int value);
	void ThreadFinished(int ID);

private:
	void wav2flac();
	void flac2wav();
	void flac2mp3();
	void wav2mp3();

	// libflac callbacks for metadata handling
	static size_t libflac_metadata_read_callback(void* ptr, size_t size, size_t nmemb, ::FLAC__IOHandle handle);
	static size_t libflac_metadata_write_callback(const void* ptr, size_t size, size_t nmemb, ::FLAC__IOHandle handle);
	static FLAC__int64 libflac_metadata_tell_callback(FLAC__IOHandle handle);
	static int libflac_metadata_eof_callback(::FLAC__IOHandle handle);
	static int libflac_metadata_seek_callback(::FLAC__IOHandle handle, FLAC__int64 offset, int whence);
	static int libflac_metadata_close_callback(::FLAC__IOHandle handle);

	// wave file header
	struct sWAVEheader
	{
		char ChunkID[4];		// "RIFF"
		int ChunkSize;			// size of the complete WAVE file
		char Format[4];			// "WAVE"
	};

	// wave file format chunk header
	struct sFMTheader
	{
		char ChunkID[4];				// "fmt "
		int ChunkSize;					// 16, 18 or 40 bytes
		unsigned short AudioFormat;		// data format code
		short NumChannels;
		int SampleRate;
		int ByteRate;					// SampleRate * NumChannels * BitsPerSample/8
		short BlockAlign;				// NumChannels * BitsPerSample/8
		short BitsPerSample;
		short ExtensionSize;			// Size of the extension (0 or 22 bytes)
		short ValidBitsPerSample;
		int ChannelMask;				// Speaker position mask
		short SubFormat_AudioFormat;	// data format code
		char SubFormat_GUID[14];
	};

	// wave file data chunk header
	struct sDATAheader
	{
		char ChunkID[4];		// "data"
		int ChunkSize;			// NumSamples * NumChannels * BitsPerSample/8
	};

	// structure for metadata transfer
	struct sMetaData
	{
		char* text;
		bool present;
	};

	sFLACdropQtSettings settings;
	QString infile;
	int inputFileType;
	int ID;
};

//---------------------------------------------------------------------------------
// Encoder thread scheduler
//---------------------------------------------------------------------------------
class cScheduler : public QObject
{
	Q_OBJECT

public:
	cScheduler(QApplication* gui_thread);
	void addPathList(const QStringList& droppedFiles);
	void addEncoderSettings(sFLACdropQtSettings encSettings);

signals:
	void setProgressbarValue(int ID, int value);
	void setProgressbarTotalLimits(int min, int max);
	void setProgressbarTotalValue(int value);
	void setDrop(bool state);

private slots:
	void ThreadFinished(int ID);
	void startNewThread();
	void startEncoding();

private:
	bool SelectEncoder(const QString& droppedfile);

	QApplication* gui_window;
	QStringList pathList;
	sFLACdropQtSettings ActualSettings;
	encoders* encoder_list[OUT_MAX_THREADS];
	bool thread_status[OUT_MAX_THREADS];
	int pathlistPosition;
	int countfinishedthreads;
};

//---------------------------------------------------------------------------------
// libFLAC encoder / decoder classes
//---------------------------------------------------------------------------------
class libflac_StreamEncoder : public FLAC::Encoder::Stream
{
public:
	FILE* file_;

	libflac_StreamEncoder() : FLAC::Encoder::Stream(), file_(0) { }
	~libflac_StreamEncoder() { }

	// callback functions for libFLAC encoder stream handling
	::FLAC__StreamEncoderReadStatus read_callback(FLAC__byte buffer[], size_t* bytes);
	::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], size_t bytes, uint32_t samples, uint32_t current_frame);
	::FLAC__StreamEncoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
	::FLAC__StreamEncoderTellStatus tell_callback(FLAC__uint64* absolute_byte_offset);
	void metadata_callback(const ::FLAC__StreamMetadata* metadata);

private:
	libflac_StreamEncoder(const libflac_StreamEncoder&);
	//StreamEncoder& operator=(const StreamEncoder&);
};

class libflac_StreamDecoder : public FLAC::Decoder::Stream
{
public:
	FILE* file_;

	libflac_StreamDecoder() : FLAC::Decoder::Stream(), file_(0) { }
	~libflac_StreamDecoder() { }

	unsigned int get_bits_per_sample_clientdata();
	unsigned int get_blocksize_clientdata();
	unsigned int get_channels_clientdata();
	unsigned int get_sample_rate_clientdata();
	void addOutputFile(FILE* f);
	void addOutputMem(BYTE* buffer);
	
	// callback functions for libFLAC encoder stream handling
	::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t* bytes);
	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame* frame, const FLAC__int32* const buffer[]);
	::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
	::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64* absolute_byte_offset);
	::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64* stream_length);
	bool eof_callback();
	void metadata_callback(const ::FLAC__StreamMetadata* metadata);
	void error_callback(::FLAC__StreamDecoderErrorStatus status);

private:
	libflac_StreamDecoder(const libflac_StreamDecoder&);
	//libflac_StreamDecoder& operator=(const libflac_StreamDecoder&);

	// wave file header
	struct sWAVEheader
	{
		char ChunkID[4];		// "RIFF"
		int ChunkSize;			// size of the complete WAVE file
		char Format[4];			// "WAVE"
	};
	
	// wave file format chunk header
	struct sFMTheader
	{
		char ChunkID[4];				// "fmt "
		int ChunkSize;					// 16, 18 or 40 bytes
		unsigned short AudioFormat;		// data format code
		short NumChannels;
		int SampleRate;
		int ByteRate;					// SampleRate * NumChannels * BitsPerSample/8
		short BlockAlign;				// NumChannels * BitsPerSample/8
		short BitsPerSample;
		short ExtensionSize;			// Size of the extension (0 or 22 bytes)
		short ValidBitsPerSample;
		int ChannelMask;				// Speaker position mask
		short SubFormat_AudioFormat;	// data format code
		char SubFormat_GUID[14];
	};

	// wave file data chunk header
	struct sDATAheader
	{
		char ChunkID[4];		// "data"
		int ChunkSize;			// NumSamples * NumChannels * BitsPerSample/8
	};

	struct sClientData
	{
		FILE* fout;
		BYTE* buffer_out;
		BYTE rendertarget;
		FLAC__uint64 total_samples;
		unsigned int sample_rate;
		unsigned int channels;
		unsigned int bps;
		unsigned int blocksize;
	};

	sClientData client_data;
};