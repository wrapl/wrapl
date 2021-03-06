#include <Std.h>
#include <Agg/Table.h>
#include <Sys/Module.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <Util/TypedFunction.h>
#include <Num/Array.h>
#include <sndfile.h>

typedef struct sndfile_t {
	const Std$Type$t *Type;
	SNDFILE *Handle;
	IO$Stream$t *Stream;
	Std$Object$t *SampleRate, *Channels, *Format;
	int (*read)(IO$Stream$t *, char *, int, int);
	int (*write)(IO$Stream$t *, char *, int, int);
	int (*tell)(IO$Stream$t *);
	int (*seek)(IO$Stream$t *, int, int);
} sndfile_t;

TYPE(T, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$T);

CONSTANT(Mode, Sys$Module$T) {
	Sys$Module$t *Module = Sys$Module$new("Mode");
	Sys$Module$export(Module, "Read", 0, Std$Integer$new_small(SFM_READ));
	Sys$Module$export(Module, "Write", 0, Std$Integer$new_small(SFM_WRITE));
	Sys$Module$export(Module, "ReadWrite", 0, Std$Integer$new_small(SFM_RDWR));
	return (Std$Object$t *)Module;
};

CONSTANT(Format, Sys$Module$T) {
	Sys$Module$t *Module = Sys$Module$new("Format");
	Sys$Module$export(Module, "WAV", 0, Std$Integer$new_small(SF_FORMAT_WAV));
	Sys$Module$export(Module, "AIFF", 0, Std$Integer$new_small(SF_FORMAT_AIFF));
	Sys$Module$export(Module, "AU", 0, Std$Integer$new_small(SF_FORMAT_AU));
	Sys$Module$export(Module, "RAW", 0, Std$Integer$new_small(SF_FORMAT_RAW));
	Sys$Module$export(Module, "PAF", 0, Std$Integer$new_small(SF_FORMAT_PAF));
	Sys$Module$export(Module, "SVX", 0, Std$Integer$new_small(SF_FORMAT_SVX));
	Sys$Module$export(Module, "NIST", 0, Std$Integer$new_small(SF_FORMAT_NIST));
	Sys$Module$export(Module, "VOC", 0, Std$Integer$new_small(SF_FORMAT_VOC));
	Sys$Module$export(Module, "IRCAM", 0, Std$Integer$new_small(SF_FORMAT_IRCAM));
	Sys$Module$export(Module, "W64", 0, Std$Integer$new_small(SF_FORMAT_W64));
	Sys$Module$export(Module, "MAT4", 0, Std$Integer$new_small(SF_FORMAT_MAT4));
	Sys$Module$export(Module, "MAT5", 0, Std$Integer$new_small(SF_FORMAT_MAT5));
	Sys$Module$export(Module, "PVF", 0, Std$Integer$new_small(SF_FORMAT_PVF));
	Sys$Module$export(Module, "XI", 0, Std$Integer$new_small(SF_FORMAT_XI));
	Sys$Module$export(Module, "HTK", 0, Std$Integer$new_small(SF_FORMAT_HTK));
	Sys$Module$export(Module, "SDS", 0, Std$Integer$new_small(SF_FORMAT_SDS));
	Sys$Module$export(Module, "AVR", 0, Std$Integer$new_small(SF_FORMAT_AVR));
	Sys$Module$export(Module, "WAVEX", 0, Std$Integer$new_small(SF_FORMAT_WAVEX));
	Sys$Module$export(Module, "SD2", 0, Std$Integer$new_small(SF_FORMAT_SD2));
	Sys$Module$export(Module, "FLAC", 0, Std$Integer$new_small(SF_FORMAT_FLAC));
	Sys$Module$export(Module, "CAF", 0, Std$Integer$new_small(SF_FORMAT_CAF));
	Sys$Module$export(Module, "WVE", 0, Std$Integer$new_small(SF_FORMAT_WVE));
	Sys$Module$export(Module, "Ogg", 0, Std$Integer$new_small(SF_FORMAT_OGG));
	Sys$Module$export(Module, "MPC2K", 0, Std$Integer$new_small(SF_FORMAT_MPC2K));
	Sys$Module$export(Module, "RF64", 0, Std$Integer$new_small(SF_FORMAT_RF64));
	Sys$Module$export(Module, "PCM_S8", 0, Std$Integer$new_small(SF_FORMAT_PCM_S8));
	Sys$Module$export(Module, "PCM_16", 0, Std$Integer$new_small(SF_FORMAT_PCM_16));
	Sys$Module$export(Module, "PCM_24", 0, Std$Integer$new_small(SF_FORMAT_PCM_24));
	Sys$Module$export(Module, "PCM_32", 0, Std$Integer$new_small(SF_FORMAT_PCM_32));
	Sys$Module$export(Module, "PCM_U8", 0, Std$Integer$new_small(SF_FORMAT_PCM_U8));
	Sys$Module$export(Module, "Float", 0, Std$Integer$new_small(SF_FORMAT_FLOAT));
	Sys$Module$export(Module, "Double", 0, Std$Integer$new_small(SF_FORMAT_DOUBLE));
	Sys$Module$export(Module, "ULAW", 0, Std$Integer$new_small(SF_FORMAT_ULAW));
	Sys$Module$export(Module, "ALAW", 0, Std$Integer$new_small(SF_FORMAT_ALAW));
	Sys$Module$export(Module, "IMA_ADPCM", 0, Std$Integer$new_small(SF_FORMAT_IMA_ADPCM));
	Sys$Module$export(Module, "MS_ADPCM", 0, Std$Integer$new_small(SF_FORMAT_MS_ADPCM));
	Sys$Module$export(Module, "GSM610", 0, Std$Integer$new_small(SF_FORMAT_GSM610));
	Sys$Module$export(Module, "VOX_ADPCM", 0, Std$Integer$new_small(SF_FORMAT_VOX_ADPCM));
	Sys$Module$export(Module, "G721_32", 0, Std$Integer$new_small(SF_FORMAT_G721_32));
	Sys$Module$export(Module, "G723_24", 0, Std$Integer$new_small(SF_FORMAT_G723_24));
	Sys$Module$export(Module, "G723_40", 0, Std$Integer$new_small(SF_FORMAT_G723_40));
	Sys$Module$export(Module, "DWVW_12", 0, Std$Integer$new_small(SF_FORMAT_DWVW_12));
	Sys$Module$export(Module, "DWVW_16", 0, Std$Integer$new_small(SF_FORMAT_DWVW_16));
	Sys$Module$export(Module, "DWVW_24", 0, Std$Integer$new_small(SF_FORMAT_DWVW_24));
	Sys$Module$export(Module, "DWVW_N", 0, Std$Integer$new_small(SF_FORMAT_DWVW_N));
	Sys$Module$export(Module, "DPCM_8", 0, Std$Integer$new_small(SF_FORMAT_DPCM_8));
	Sys$Module$export(Module, "DPCM_16", 0, Std$Integer$new_small(SF_FORMAT_DPCM_16));
	Sys$Module$export(Module, "Vorbis", 0, Std$Integer$new_small(SF_FORMAT_VORBIS));
	Sys$Module$export(Module, "SubMask", 0, Std$Integer$new_small(SF_FORMAT_SUBMASK));
	Sys$Module$export(Module, "TypeMask", 0, Std$Integer$new_small(SF_FORMAT_TYPEMASK));
	Sys$Module$export(Module, "EndMask", 0, Std$Integer$new_small(SF_FORMAT_ENDMASK));
	return (Std$Object$t *)Module;
};

CONSTANT(Endian, Sys$Module$T) {
	Sys$Module$t *Module = Sys$Module$new("Endian");
	Sys$Module$export(Module, "File", 0, Std$Integer$new_small(SF_ENDIAN_FILE));
	Sys$Module$export(Module, "Little", 0, Std$Integer$new_small(SF_ENDIAN_LITTLE));
	Sys$Module$export(Module, "Big", 0, Std$Integer$new_small(SF_ENDIAN_BIG));
	Sys$Module$export(Module, "CPU", 0, Std$Integer$new_small(SF_ENDIAN_CPU));
	return (Std$Object$t *)Module;
};

SYMBOL($sample_rate, "sample_rate");
SYMBOL($channels, "channels");
SYMBOL($format, "format");
SYMBOL($seekable, "seekable");

GLOBAL_FUNCTION(FormatCheck, 1) {
	CHECK_ARG_TYPE(0, Std$Symbol$ArrayT);
	SF_INFO Info = {0,};
	Std$Symbol$array *Array = (Std$Symbol$array *)Args[0].Val;
	for (int I = 0; I < Array->Count; ++I) {
		if (Array->Values[I] == $sample_rate) {
			Info.samplerate = Std$Integer$get_small(Args[I + 1].Val);
		} else if (Array->Values[I] == $channels) {
			Info.channels = Std$Integer$get_small(Args[I + 1].Val);
		} else if (Array->Values[I] == $format) {
			Info.format = Std$Integer$get_small(Args[I + 1].Val);
		}
	}
	Result->Val = Args[0].Val;
	return sf_format_check(&Info) ? SUCCESS : FAILURE;
};

static sf_count_t sndfile_get_filelen(sndfile_t *SndFile) {
	int Current = SndFile->tell(SndFile->Stream);
	int FileLen = SndFile->seek(SndFile->Stream, 0, IO$Stream$SEEK_END);
	SndFile->seek(SndFile->Stream, Current, IO$Stream$SEEK_SET);
	return FileLen;
};

static sf_count_t sndfile_seek(sf_count_t Offset, int Whence, sndfile_t *SndFile) {
	switch (Whence) {
	case SEEK_SET: Whence = IO$Stream$SEEK_SET; break;
	case SEEK_CUR: Whence = IO$Stream$SEEK_CUR; break;
	case SEEK_END: Whence = IO$Stream$SEEK_END; break;
	}
	return SndFile->seek(SndFile->Stream, Offset, Whence);
};

static sf_count_t sndfile_read(void *Buffer, sf_count_t Count, sndfile_t *SndFile) {
	return SndFile->read(SndFile->Stream, Buffer, Count, 1);
};

static sf_count_t sndfile_write(void *Buffer, sf_count_t Count, sndfile_t *SndFile) {
	return SndFile->write(SndFile->Stream, Buffer, Count, 1);
};

static sf_count_t sndfile_tell(sndfile_t *SndFile) {
	return SndFile->tell(SndFile->Stream);
};

static sf_count_t sndfile_fallback() {
	return 0;
};

GLOBAL_FUNCTION(Open, 2) {
	CHECK_ARG_TYPE(0, IO$Stream$T);
	CHECK_ARG_TYPE(1, Std$Integer$SmallT);
	IO$Stream$t *Stream = (IO$Stream$t *)Args[0].Val;
	sndfile_t *SndFile = new(sndfile_t);
	SndFile->Type = T;
	SndFile->Stream = Stream;
	SndFile->read = Util$TypedFunction$get(IO$Stream$read, Stream->Type);
	if (SndFile->read == (void *)-1) SndFile->read = sndfile_fallback;
	SndFile->write = Util$TypedFunction$get(IO$Stream$write, Stream->Type);
	if (SndFile->write == (void *)-1) SndFile->write = sndfile_fallback;
	SndFile->seek = Util$TypedFunction$get(IO$Stream$seek, Stream->Type);
	if (SndFile->seek == (void *)-1) SndFile->seek = sndfile_fallback;
	SndFile->tell = Util$TypedFunction$get(IO$Stream$tell, Stream->Type);
	if (SndFile->tell == (void *)-1) SndFile->tell = sndfile_fallback;
	SF_VIRTUAL_IO VirtualIO = {
		sndfile_get_filelen,
		sndfile_seek,
		sndfile_read,
		sndfile_write,
		sndfile_tell
	};
	int Mode = Std$Integer$get_small(Args[1].Val);
	SF_INFO Info = {0,};
	if (Count > 2) {
		CHECK_ARG_TYPE(2, Std$Symbol$ArrayT);
		Std$Symbol$array *Array = (Std$Symbol$array *)Args[2].Val;
		for (int I = 0; I < Array->Count; ++I) {
			if (Array->Values[I] == $sample_rate) {
				Info.samplerate = Std$Integer$get_small(Args[I + 3].Val);
			} else if (Array->Values[I] == $channels) {
				Info.channels = Std$Integer$get_small(Args[I + 3].Val);
			} else if (Array->Values[I] == $format) {
				Info.format = Std$Integer$get_small(Args[I + 3].Val);
			} else if (Array->Values[I] == $seekable) {
				Info.channels = Args[I + 3].Val == $true;
			}
		}
	}
	if ((SndFile->Handle = sf_open_virtual(&VirtualIO, Mode, &Info, SndFile))) {
		SndFile->SampleRate = Std$Integer$new_small(Info.samplerate);
		SndFile->Channels = Std$Integer$new_small(Info.channels);
		SndFile->Format = Std$Integer$new_small(Info.format);
		Result->Val = (Std$Object$t *)SndFile;
		return SUCCESS;
	} else {
		Result->Val = Std$String$copy(sf_strerror(0));
		return MESSAGE;
	};
};

METHOD("sample_rate", TYP, T) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	Result->Val = SndFile->SampleRate;
	return SUCCESS;
};

METHOD("channels", TYP, T) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	Result->Val = SndFile->Channels;
	return SUCCESS;
};

METHOD("format", TYP, T) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	Result->Val = SndFile->Format;
	return SUCCESS;
};

METHOD("strerror", TYP, T) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	Result->Val = Std$String$new(sf_strerror(SndFile->Handle));
	return SUCCESS;
};

#define METHODS(RTYPE, CTYPE) \
METHOD("read", TYP, T, TYP, Num$Array$ ## RTYPE) { \
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val; \
	Num$Array$t * Array = (Num$Array$t *)Args[1].Val; \
	if (Array->Degree == 1) { \
		if (Array->Dimensions[0].Indices) SEND(Std$String$new("Invalid array type")); \
		if (Array->Dimensions[0].Stride != sizeof(CTYPE)) SEND(Std$String$new("Invalid array type")); \
		Result->Val = Std$Integer$new_small(sf_read_ ## CTYPE(SndFile->Handle, Array->Data, Array->Dimensions[0].Size)); \
	} else if (Array->Degree == 2) { \
		if (Array->Dimensions[0].Indices) SEND(Std$String$new("Invalid array type")); \
		if (Array->Dimensions[1].Indices) SEND(Std$String$new("Invalid array type")); \
		int NumChannels = Std$Integer$get_small(SndFile->Channels); \
		if (Array->Dimensions[1].Size != NumChannels) SEND(Std$String$new("Array shape does not number of channels")); \
		if (Array->Dimensions[1].Stride == sizeof(CTYPE) && Array->Dimensions[0].Stride == sizeof(CTYPE) * NumChannels) { \
			Result->Val = Std$Integer$new_small(sf_readf_ ## CTYPE(SndFile->Handle, Array->Data, Array->Dimensions[0].Size)); \
		} else { \
			int Total = 0; \
			int Remaining = Array->Dimensions[0].Size; \
			CTYPE Chunk[64 * NumChannels]; \
			char *Dest = Array->Data; \
			int Stride0 = Array->Dimensions[0].Stride; \
			int Stride1 = Array->Dimensions[1].Stride; \
			while (Remaining >= 64) { \
				int FramesRead = sf_readf_ ## CTYPE(SndFile->Handle, Chunk, 64); \
				Total += FramesRead; \
				CTYPE *Src = Chunk; \
				for (int J = 0; J < FramesRead; ++J) { \
					for (int K = 0; K < NumChannels; ++K) *(CTYPE *)(Dest + K * Stride0) = *Src++; \
					Dest += Stride1; \
				} \
				if (FramesRead < 64) RETURN(Std$Integer$new_small(Total)); \
				Remaining -= 64; \
			} \
			int FramesRead = sf_readf_ ## CTYPE(SndFile->Handle, Chunk, Remaining); \
			Total += FramesRead; \
			CTYPE *Src = Chunk; \
			for (int J = 0; J < FramesRead; ++J) { \
				for (int K = 0; K < NumChannels; ++K) *(CTYPE *)(Dest + K * Stride0) = *Src++; \
				Dest += Stride1; \
			} \
			Result->Val = Std$Integer$new_small(Total); \
		} \
	} else { \
		SEND(Std$String$new("Invalid array type")); \
	} \
	return SUCCESS; \
} \
\
METHOD("write", TYP, T, TYP, Num$Array$ ## RTYPE) { \
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val; \
	Num$Array$t * Array = (Num$Array$t *)Args[1].Val; \
	if (Array->Degree == 1) { \
		if (Array->Dimensions[0].Indices) SEND(Std$String$new("Invalid array type")); \
		if (Array->Dimensions[0].Stride != sizeof(CTYPE)) SEND(Std$String$new("Invalid array type")); \
		Result->Val = Std$Integer$new_small(sf_write_ ## CTYPE(SndFile->Handle, Array->Data, Array->Dimensions[0].Size)); \
	} else if (Array->Degree == 2) { \
		if (Array->Dimensions[0].Indices) SEND(Std$String$new("Invalid array type")); \
		if (Array->Dimensions[1].Indices) SEND(Std$String$new("Invalid array type")); \
		int NumChannels = Std$Integer$get_small(SndFile->Channels); \
		if (Array->Dimensions[1].Size != NumChannels) SEND(Std$String$new("Array shape does not number of channels")); \
		if (Array->Dimensions[1].Stride == sizeof(CTYPE) && Array->Dimensions[0].Stride == sizeof(CTYPE) * NumChannels) { \
			Result->Val = Std$Integer$new_small(sf_writef_ ## CTYPE(SndFile->Handle, Array->Data, Array->Dimensions[0].Size)); \
		} else { \
			int Total = 0; \
			int Remaining = Array->Dimensions[0].Size; \
			CTYPE Chunk[64 * NumChannels]; \
			char *Src = Array->Data; \
			int Stride0 = Array->Dimensions[0].Stride; \
			int Stride1 = Array->Dimensions[1].Stride; \
			while (Remaining >= 64) { \
				CTYPE *Dest = Chunk; \
				for (int J = 0; J < 64; ++J) { \
					for (int K = 0; K < NumChannels; ++K) *Dest++ = *(CTYPE *)(Src + K * Stride0); \
					Src += Stride1; \
				} \
				int FramesWritten = sf_writef_ ## CTYPE(SndFile->Handle, Chunk, 64); \
				Total += FramesWritten; \
				if (FramesWritten < 64) RETURN(Std$Integer$new_small(Total)); \
				Remaining -= 64; \
			} \
			CTYPE *Dest = Chunk; \
			for (int J = 0; J < Remaining; ++J) { \
				for (int K = 0; K < NumChannels; ++K) *Dest++ = *(CTYPE *)(Src + K * Stride0); \
				Src += Stride1; \
			} \
			int FramesWritten = sf_writef_ ## CTYPE(SndFile->Handle, Chunk, Remaining); \
			Total += FramesWritten; \
			Result->Val = Std$Integer$new_small(Total); \
		} \
	} else { \
		SEND(Std$String$new("Invalid array type")); \
	} \
	return SUCCESS; \
}

METHODS(Int16T, short);
METHODS(Int32T, int);
METHODS(Float32T, float);
METHODS(Float64T, double);

METHOD("read", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	void *Ptr = Std$Address$get_value(Args[1].Val);
	sf_count_t Bytes = Std$Integer$get_small(Args[2].Val);
	Result->Val = Std$Integer$new_small(sf_read_raw(SndFile->Handle, Ptr, Bytes));
	return SUCCESS;
};

METHOD("write", TYP, T, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	void *Ptr = Std$Address$get_value(Args[1].Val);
	sf_count_t Bytes = Std$Integer$get_small(Args[2].Val);
	Result->Val = Std$Integer$new_small(sf_write_raw(SndFile->Handle, Ptr, Bytes));
	return SUCCESS;
};

METHOD("flush", TYP, T) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	sf_write_sync(SndFile->Handle);
	return SUCCESS;
};

METHOD("close", TYP, T) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	return sf_close(SndFile->Handle) ? FAILURE : SUCCESS;
};

TYPED_INSTANCE(int, IO$Stream$read, T, sndfile_t *Stream, char *Buffer, int Count, int Block) {
	return sf_read_raw(Stream->Handle, Buffer, Count);
};

TYPED_INSTANCE(int, IO$Stream$write, T, sndfile_t *Stream, char *Buffer, int Count, int Block) {
	return sf_write_raw(Stream->Handle, Buffer, Count);
};

TYPED_INSTANCE(void, IO$Stream$flush, T, sndfile_t *Stream) {
	sf_write_sync(Stream->Handle);
};

TYPED_INSTANCE(void, IO$Stream$close, T, sndfile_t *Stream, int Mode) {
	sf_close(Stream->Handle);
};

CONSTANT(String, Sys$Module$T) {
	Sys$Module$t *Module = Sys$Module$new("String");
	Sys$Module$export(Module, "Title", 0, Std$Integer$new_small(SF_STR_TITLE));
	Sys$Module$export(Module, "Copyright", 0, Std$Integer$new_small(SF_STR_COPYRIGHT));
	Sys$Module$export(Module, "Software", 0, Std$Integer$new_small(SF_STR_SOFTWARE));
	Sys$Module$export(Module, "Artist", 0, Std$Integer$new_small(SF_STR_ARTIST));
	Sys$Module$export(Module, "Comment", 0, Std$Integer$new_small(SF_STR_COMMENT));
	Sys$Module$export(Module, "Date", 0, Std$Integer$new_small(SF_STR_DATE));
	Sys$Module$export(Module, "Album", 0, Std$Integer$new_small(SF_STR_ALBUM));
	Sys$Module$export(Module, "License", 0, Std$Integer$new_small(SF_STR_LICENSE));
	Sys$Module$export(Module, "TrackNumber", 0, Std$Integer$new_small(SF_STR_TRACKNUMBER));
	Sys$Module$export(Module, "Genre", 0, Std$Integer$new_small(SF_STR_GENRE));
	return (Std$Object$t *)Module;
};

METHOD("get_string", TYP, T, TYP, Std$Integer$SmallT) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	const char *Value = sf_get_string(SndFile->Handle, Std$Integer$get_small(Args[1].Val));
	if (Value) {
		Result->Val = Std$String$new(Value);
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("set_string", TYP, T, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	sndfile_t *SndFile = (sndfile_t *)Args[0].Val;
	return sf_set_string(SndFile->Handle, Std$Integer$get_small(Args[1].Val), Std$String$flatten(Args[2].Val)) ? FAILURE : SUCCESS;
};
