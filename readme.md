FLACdropQt can convert between different audio file formats. You can drop the files on it to start the conversion.
Difference between this and the prevoius FLACdrop version is the use of Qt library for GUI handling and some refactoring for code simplifying.

Aim was to speed up the encoding process. The audio libraries are designed as single thread in mind, so to utilize multithread capabilities it is launching independent encoder threads for each file.
The number of allowed parallel threads are hardcoded into the source code by "OUT_MAX_THREADS" constant value. It can be changed freely to an other number, program will compile and run with the new number.

Currently it can convert:
- from WAV to FLAC
- from WAV to MP3
- from FLAC to WAV
- from FLAC to MP3

FLACdrop is using the below libraries. Only the headers and the pre-built lib files are included:
- Qt 6.2.3 ( https://www.qt.io/ )
- libflac 1.3.4 GitHub version ( https://github.com/xiph/flac )
- libogg 1.3.3 GitHub version ( https://github.com/xiph/ogg )
- libmp3lame 3.100.2 ( https://lame.sourceforge.io/index.php )