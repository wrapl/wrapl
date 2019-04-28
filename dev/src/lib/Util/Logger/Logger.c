#include <Std.h>
#include <IO/Stream.h>
#include <IO/Terminal.h>
#include <Riva/Memory.h>
#include <Sys/Module.h>
#include <Riva/Config.h>
#include <Riva/System.h>
#include <Riva/Debug.h>
#include <Sys/Time.h>
#include <Util/TypedFunction.h>
#include <pthread.h>

int LogLevel = 1;

static int NumLevels = 6;
static const char *LevelNames[] = {
	"TRACE",
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL"
};
static const char *MainModule = 0;

typedef struct {
	Std$Function$CFields
	Std$Object$t *Stream;
	IO$Stream_writefn write;
	const char *LoggerName;
	pthread_mutex_t Mutex[1];
	int Level;
} stream_writer_t;

static char Hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static void write_json_chars(const char *Chars, int Length, Std$Object$t *Stream, IO$Stream_writefn write) {
	const char *I = Chars;
	const char *J = I;
	const char *L = I + Length;
	for (; J < L; ++J) {
		char Char = *J;
		if (Char == '\"') {
			if (I < J) write(Stream, I, J - I, 1);
			write(Stream, "\\\"", 2, 1);
			I = J + 1;
		} else if (Char == '\\') {
			if (I < J) write(Stream, I, J - I, 1);
			write(Stream, "\\\\", 2, 1);
			I = J + 1;
		} else if (Char == '\t') {
			if (I < J) write(Stream, I, J - I, 1);
			write(Stream, "\\t", 2, 1);
			I = J + 1;
		} else if (Char == '\r') {
			if (I < J) write(Stream, I, J - I, 1);
			write(Stream, "\\r", 2, 1);
			I = J + 1;
		} else if (Char == '\n') {
			if (I < J) write(Stream, I, J - I, 1);
			write(Stream, "\\n", 2, 1);
			I = J + 1;
		} else if ((Char < ' ') || (Char >= 0x80)) {
			if (I < J) write(Stream, I, J - I, 1);
			char Tmp[4] = {'\\', 'x', Hex[Char / 16], Hex[Char % 16]};
			write(Stream, Tmp, 4, 1);
			I = J + 1;
		}
	}
	if (I < J) write(Stream, I, J - I, 1);
}

static inline write_json_string(Std$Object$t *Val, Std$Object$t *Stream, IO$Stream_writefn write) {
	if (Val->Type == Std$String$T) {
		for (Std$String$block *Block = ((Std$String$t *)Val)->Blocks; Block->Length.Value; ++Block) {
			write_json_chars(Block->Chars.Value, Block->Length.Value, Stream, write);
		}
	} else {
		write(Stream, "<value>", strlen("<value>"), 1);
	}
}

static Std$Function$status json_stream_writer(FUNCTION_PARAMS) {
	stream_writer_t *StreamWriter = (stream_writer_t *)Fun;
	if (StreamWriter->Level < LogLevel) return SUCCESS;
	pthread_mutex_lock(StreamWriter->Mutex);
	Std$Object$t *Stream = StreamWriter->Stream;
	IO$Stream_writefn write = StreamWriter->write;
	write(Stream, "{\"@timestamp\":\"", strlen("{\"@timestamp\":\""), 1);
	char Buffer[256];
	struct timeval Time;
	gettimeofday(&Time, 0);
	struct tm FullTime;
	localtime_r(&Time.tv_sec, &FullTime);
	size_t BufferLength = strftime(Buffer, 256, "%Y-%m-%dT%H:%M:%S.000%z", &FullTime);
	sprintf(Buffer + 20, "%03d", Time.tv_usec / 1000);
	Buffer[23] = '+';
	write(Stream, Buffer, BufferLength, 1);
	write(Stream, "\",\"component\":\"", strlen("\",\"component\":\""), 1);
	write_json_chars(MainModule, strlen(MainModule), Stream, write);
	write(Stream, "\",\"@version\":1,\"message\":\"", strlen("\",\"@version\":1,\"message\":\""), 1);
	Std$Symbol$t **FieldNames = 0;
	int Index;
	for (Index = 0; Index < Count; ++Index) {
		Std$Object$t *Val = Args[Index].Val;
		if (Val->Type == Std$Symbol$ArrayT) {
			FieldNames = ((Std$Symbol$array *)Val)->Values;
			++Index;
			break;
		} else if (Val->Type != Std$String$T) {
			Std$Function$result Result0;
			if (Std$Function$call(Std$String$Of, 1, &Result0, Val, 0) <= SUCCESS) {
				Val = Result0.Val;
			}
		}
		write_json_string(Val, Stream, write);
	}
	write(Stream, "\",\"logger_name\":\"", strlen("\",\"logger_name\":\""), 1);
	write(Stream, StreamWriter->LoggerName, strlen(StreamWriter->LoggerName), 1);
	write(Stream, "\",\"thread_name\":\"0\",\"level\":\"", strlen("\",\"thread_name\":\"0\",\"level\":\""), 1);
	write(Stream, LevelNames[StreamWriter->Level], strlen(LevelNames[StreamWriter->Level]), 1);
	write(Stream, "\",\"level_value\":", strlen("\",\"level_value\":"), 1);
	BufferLength = sprintf(Buffer, "%d", StreamWriter->Level);
	write(Stream, Buffer, BufferLength, 1);
	if (FieldNames) {
		while (Index < Count) {
			write(Stream, ",\"", 2, 1);
			write_json_string(FieldNames[0]->Name, Stream, write);
			write(Stream, "\":\"", 3, 1);
			Std$Object$t *Val = Args[Index].Val;
			if (Val->Type != Std$String$T) {
				Std$Function$result Result0;
				if (Std$Function$call(Std$String$Of, 1, &Result0, Val, 0) <= SUCCESS) {
					Val = Result0.Val;
				}
			}
			write_json_string(Val, Stream, write);
			write(Stream, "\"", 1, 1);
			++Index;
			++FieldNames;
		}
	}
	StreamWriter->write(Stream, "}\n", 2, 1);
	pthread_mutex_unlock(StreamWriter->Mutex);
	return SUCCESS;
}

GLOBAL_FUNCTION(JsonStream, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	Std$Object$t *Stream = IO$Terminal$Err;
	if (Count > 2) {
		CHECK_ARG_TYPE(2, IO$Stream$WriterT);
		Stream = Args[2].Val;
	}
	stream_writer_t *StreamWriter = new(stream_writer_t);
	StreamWriter->Type = Std$Function$CT;
	StreamWriter->Invoke = json_stream_writer;
	StreamWriter->Level = Std$Integer$get_small(Args[0].Val);
	StreamWriter->LoggerName = Std$String$flatten(Args[1].Val);
	StreamWriter->Stream = Stream;
	StreamWriter->write = Util$TypedFunction$get(IO$Stream$write, Stream->Type);
	pthread_mutex_init(StreamWriter->Mutex, 0);
	Result->Val = (Std$Object$t *)StreamWriter;
	return SUCCESS;
}

static inline write_text_string(Std$Object$t *Val, Std$Object$t *Stream, IO$Stream_writefn write) {
	if (Val->Type == Std$String$T) {
		for (Std$String$block *Block = ((Std$String$t *)Val)->Blocks; Block->Length.Value; ++Block) {
			write(Stream, Block->Chars.Value, Block->Length.Value, 1);
		}
	} else {
		write(Stream, "<value>", strlen("<value>"), 1);
	}
}

static Std$Function$status text_stream_writer(FUNCTION_PARAMS) {
	stream_writer_t *StreamWriter = (stream_writer_t *)Fun;
	if (StreamWriter->Level < LogLevel) return SUCCESS;
	pthread_mutex_lock(StreamWriter->Mutex);
	Std$Object$t *Stream = StreamWriter->Stream;
	IO$Stream_writefn write = StreamWriter->write;
	write(Stream, "[", 1, 1);
	write(Stream, LevelNames[StreamWriter->Level], strlen(LevelNames[StreamWriter->Level]), 1);
	write(Stream, "] ", 2, 1);
	char Buffer[256];
	struct timeval Time;
	gettimeofday(&Time, 0);
	struct tm FullTime;
	localtime_r(&Time, &FullTime);
	size_t BufferLength = strftime(Buffer, 256, "%Y-%m-%dT%H:%M:%S", &FullTime);
	write(Stream, Buffer, BufferLength, 1);
	write(Stream, " ", 1, 1);
	write(Stream, MainModule, strlen(MainModule), 1);
	write(Stream, " ", 1, 1);
	write(Stream, StreamWriter->LoggerName, strlen(StreamWriter->LoggerName), 1);
	write(Stream, " - ", 3, 1);
	Std$Symbol$t **FieldNames = 0;
	int Index;
	for (Index = 0; Index < Count; ++Index) {
		Std$Object$t *Val = Args[Index].Val;
		if (Val->Type == Std$Symbol$ArrayT) {
			FieldNames = ((Std$Symbol$array *)Val)->Values;
			++Index;
			break;
		} else if (Val->Type != Std$String$T) {
			Std$Function$result Result0;
			if (Std$Function$call(Std$String$Of, 1, &Result0, Val, 0) <= SUCCESS) {
				Val = Result0.Val;
			}
		}
		write_text_string(Val, Stream, write);
	}
	if (FieldNames) {
		while (Index < Count) {
			write(Stream, " ", 1, 1);
			write_text_string(FieldNames[0]->Name, Stream, write);
			write(Stream, "=", 1, 1);
			Std$Object$t *Val = Args[Index].Val;
			if (Val->Type != Std$String$T) {
				Std$Function$result Result0;
				if (Std$Function$call(Std$String$Of, 1, &Result0, Val, 0) <= SUCCESS) {
					Val = Result0.Val;
				}
			}
			write_text_string(Val, Stream, write);
			++Index;
			++FieldNames;
		}
	}
	StreamWriter->write(Stream, "\n", 1, 1);
	pthread_mutex_unlock(StreamWriter->Mutex);
	return SUCCESS;
}

GLOBAL_FUNCTION(TextStream, 2) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	Std$Object$t *Stream = IO$Terminal$Err;
	if (Count > 2) {
		CHECK_ARG_TYPE(2, IO$Stream$WriterT);
		Stream = Args[2].Val;
	}
	stream_writer_t *StreamWriter = new(stream_writer_t);
	StreamWriter->Type = Std$Function$CT;
	StreamWriter->Invoke = text_stream_writer;
	StreamWriter->Level = Std$Integer$get_small(Args[0].Val);
	StreamWriter->LoggerName = Std$String$flatten(Args[1].Val);
	StreamWriter->Stream = Stream;
	StreamWriter->write = Util$TypedFunction$get(IO$Stream$write, Stream->Type);
	pthread_mutex_init(StreamWriter->Mutex, 0);
	Result->Val = (Std$Object$t *)StreamWriter;
	return SUCCESS;
}

CONSTANT(LoggerFactory, Std$Object$T) {
	const char *ConfigPath = Riva$Config$get("Logger/Config");
	MainModule = Riva$Config$get("Riva/MainModule");
	Sys$Module$t *ConfigModule = 0;
	if (ConfigPath) ConfigModule = Sys$Module$load(0, ConfigPath);
	if (ConfigModule) {
		int IsRef;
		void *Data;
		if (Sys$Module$import(ConfigModule, "New", &IsRef, &Data)) {
			if (IsRef) return *(void **)Data;
			return Data;
		}
	}
	const char *LogLevelName = Riva$Config$get("Logger/Level");
	if (LogLevelName) {
		for (int Level = 0; Level < NumLevels; ++Level) {
			if (!strcmp(LogLevelName, LevelNames[Level])) {
				LogLevel = Level;
				break;
			}
		}
	}
	return TextStream;
}

GLOBAL_FUNCTION(SetLevel, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$Integer$SmallT);
	LogLevel = Std$Integer$get_small(Args[0].Val);
	Result->Arg = Args[0];
	return SUCCESS;
}

GLOBAL_FUNCTION(New, 1) {
	CHECK_EXACT_ARG_TYPE(0, Sys$Module$T);
	Sys$Module$t *Base = (Sys$Module$t *)Args[0].Val;
	const char *BaseName = Sys$Module$get_name(Base);
	char *LoggerName;
	asprintf(&LoggerName, "%s.Logger", BaseName);
	Sys$Module$t *Logger = Sys$Module$new(LoggerName);
	for (int Level = 0; Level < NumLevels; ++Level) {
		switch (Std$Function$call(LoggerFactory, 2, Result, Std$Integer$new_small(Level), 0, Std$String$new(BaseName), 0)) {
		case SUSPEND: case SUCCESS:
			Sys$Module$export(Logger, LevelNames[Level], 0, Result->Val);
			break;
		case FAILURE:
			Result->Val = Std$String$new("Failed to set up logger");
		case MESSAGE:
			return MESSAGE;
		}
	}
	Result->Val = (Std$Object$t *)Logger;
	return SUCCESS;
}
