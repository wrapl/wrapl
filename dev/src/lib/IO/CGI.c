#include <IO/Stream.h>
#include <Std.h>
#include <Riva.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct cgi_reader_t {
	Std$Type$t *Type;
	int Length, Remaining;
} cgi_reader_t;

TYPE(ReaderT, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$T);

static IO$Stream$messaget OpenMessage[] = {{IO$Stream$MessageT, "Open Error"}};
static IO$Stream$messaget ReadMessage[] = {{IO$Stream$MessageT, "Read Error"}};

GLOBAL_FUNCTION(Open, 0) {
	char *LengthStr = getenv("CONTENT_LENGTH");
	if (LengthStr == 0) {
		Result->Val = OpenMessage;
		return MESSAGE;
	};
	cgi_reader_t *Stream = new(cgi_reader_t);
	Stream->Type = ReaderT;
	Stream->Length = Stream->Remaining = strtol(LengthStr, 0, 10);
	Result->Val = Stream;
	return SUCCESS;
};

static int cgi_reader_eoi(cgi_reader_t *Stream) {
	return Stream->Remaining == 0;
};

METHOD("read", TYP, ReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	cgi_reader_t *Stream = Args[0].Val;
	char *Buffer = ((Std$Address$t *)Args[1].Val)->Value;
	long Size = ((Std$Integer$smallt *)Args[2].Val)->Value;
	if (Size > Stream->Remaining) Size = Stream->Remaining;
	size_t BytesRead = read(STDIN_FILENO, Buffer, Size);
	if (BytesRead < 0) {
		Result->Val = ReadMessage;
		return MESSAGE;
	};
	Stream->Remaining -= BytesRead;
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
};

static int cgi_reader_read(cgi_reader_t *Stream, char *Buffer, int Count) {
	if (Count > Stream->Remaining) Count = Stream->Remaining;
	size_t BytesRead = read(STDIN_FILENO, Buffer, Count);
	if (BytesRead >= 0) Stream->Remaining -= BytesRead;
	return BytesRead;
};

static char cgi_reader_readc(cgi_reader_t *Stream) {
	if (Stream->Remaining == 0) return EOF;
	char Char;
	int Status = read(STDIN_FILENO, &Char, 1);
	if (Status < 0) return 0;
	if (Status == 0) return EOF;
	Stream->Remaining -= Status;
	return Char;
};

METHOD("read", TYP, ReaderT, TYP, Std$Integer$SmallT) {
	cgi_reader_t *Stream = Args[0].Val;
	unsigned long Length = ((Std$Integer$smallt *)Args[1].Val)->Value;
	if (Length > Stream->Remaining) Length = Stream->Remaining;
	char *Buffer = Riva$Memory$alloc_atomic(Length);
	size_t BytesRead = read(STDIN_FILENO, Buffer, Length);
	if (BytesRead == 0) return FAILURE;
	if (BytesRead < 0) {
		Result->Val = ReadMessage;
		return MESSAGE;
	};
	Stream->Remaining -= BytesRead;
	Result->Val = Std$String$new_length(Buffer, BytesRead);
	return SUCCESS;
};

static char *cgi_reader_readn(cgi_reader_t *Stream, int Count) {
	if (Count > Stream->Remaining) Count = Stream->Remaining;
	char *Buffer = Riva$Memory$alloc_atomic(Count + 1);
	size_t BytesRead = read(STDIN_FILENO, Buffer, Count);
	if (BytesRead >= 0) {
		Stream->Remaining -= BytesRead;
		return Buffer;
	} else {
		return 0;
	};
};

static int HexChars[] = {
	['0'] = 0,
	['1'] = 1,
	['2'] = 2,
	['3'] = 3,
	['4'] = 4,
	['5'] = 5,
	['6'] = 6,
	['7'] = 7,
	['8'] = 8,
	['9'] = 9,
	['A'] = 10,
	['B'] = 11,
	['C'] = 12,
	['D'] = 13,
	['E'] = 14,
	['F'] = 15
};

static char *_read_line_next(cgi_reader_t *Stream, unsigned long Index) {
	if (Stream->Remaining == 0) {
		if (Index == 0) return 0;
		char *Line = Riva$Memory$alloc_atomic(Index + 1);
		Line[Index] = 0;
		return Line;
	};
	char Char;
	int Status = read(STDIN_FILENO, &Char, 1);
	if (Status < 0) return 0;
	if (Status == 0) {
		if (Index == 0) return 0;
		char *Line = Riva$Memory$alloc_atomic(Index + 1);
		Line[Index] = 0;
		return Line;
	};
	Stream->Remaining -= Status;
	if (Char == '%') {
		char Char0, Char1;
		read(STDIN_FILENO, &Char0, 1);
		read(STDIN_FILENO, &Char1, 1);
		Char = HexChars[Char0] * 16 + HexChars[Char1];
	} else if (Char == '+') {
		Char = ' ';
	};
	if (Char == '\n') {
		char *Line = Riva$Memory$alloc_atomic(Index + 1);
		Line[Index] = 0;
		return Line;
	};
	if (Char == '\r') {
		char *Line = _read_line_next(Stream, Index);
		return Line;
	} else {
		char *Line = _read_line_next(Stream, Index + 1);
		if (Line) Line[Index] = Char;
		return Line;
	};
};

METHOD("read", TYP, ReaderT) {
	cgi_reader_t *Stream = Args[0].Val;
	char *Line = _read_line_next(Stream, 0);
	if (Line == 0) {
		return FAILURE;
	} else {
		Result->Val = Std$String$new(Line);
		return SUCCESS;
	};
};

static char *cgi_reader_readl(cgi_reader_t *Stream) {
	return _read_line_next(Stream, 0);
};

static IO$Stream$reader_methods _ReaderT_Methods = {
	cgi_reader_eoi,
	cgi_reader_read,
	cgi_reader_readc,
	cgi_reader_readn,
	cgi_reader_readl
};

INITIAL() {
	Util$TypeTable$put(IO$Stream$ReaderT_Methods, ReaderT, &_ReaderT_Methods);
};
