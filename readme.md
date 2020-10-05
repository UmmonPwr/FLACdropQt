FLACdropQt can convert between different audio file formats. You can drop the files on it to start the conversion.
Difference between this and the prevoius FLACdrop version is the use of Qt library for GUI handling and some refactoring for code simplifying.

Aim was to speed up the encoding process. The used audio libraries are designed as single thread in mind, so to utilize multithread capabilities it is launching independent encoder threads for each file.
The number of allowed parallel threads are hardcoded into the source code by "OUT_MAX_THREADS" constant value. It can be changed freely to an other number if desired.

Currently it can convert:
- from WAV to FLAC

FLACdrop is using the below libraries. Only the headers and the pre-built lib files are included:
- Qt 5.15
- libflac 1.3.2 GitHub version ( https://github.com/xiph/flac )
- libogg 1.3.3 GitHub version ( https://github.com/xiph/ogg )