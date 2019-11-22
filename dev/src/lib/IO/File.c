#include <IO/File.h>
#include <Riva/Memory.h>
#include <Sys/Module.h>
#include <Std.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

TYPE(T, IO$Native$(SeekerT), IO$Native$(T), IO$Stream$SeekerT, IO$Stream$T);

TYPE(ReaderT, T, IO$Native$(ReaderT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(WriterT, T, IO$Native$(WriterT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(ReaderWriterT, ReaderT, WriterT, T, IO$Native$(ReaderT), IO$Native$(WriterT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

TYPE(TextReaderT, ReaderT, T, IO$Native$(TextReaderT), IO$Native$(ReaderT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(TextWriterT, WriterT, T, IO$Native$(TextWriterT), IO$Native$(WriterT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(TextReaderWriterT, ReaderT, WriterT, T, IO$Native$(TextReaderT), IO$Native$(TextWriterT), IO$Native$(ReaderT), IO$Native$(WriterT), IO$Native$(SeekerT), IO$Native$(T), IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

Std$Integer$smallt READ[] = {{Std$Integer$SmallT, IO$File$OPEN_READ}};
Std$Integer$smallt WRITE[] = {{Std$Integer$SmallT, IO$File$OPEN_WRITE}};
Std$Integer$smallt TEXT[] = {{Std$Integer$SmallT, IO$File$OPEN_TEXT}};
Std$Integer$smallt APPEND[] = {{Std$Integer$SmallT, IO$File$OPEN_APPEND}};
Std$Integer$smallt NOBLOCK[] = {{Std$Integer$SmallT, IO$File$OPEN_NOBLOCK}};
Std$Integer$smallt EXCL[] = {{Std$Integer$SmallT, IO$File$OPEN_EXCLUSIVE}};
Std$Integer$smallt TRUNC[] = {{Std$Integer$SmallT, IO$File$OPEN_TRUNCATE}};

SUBMODULE(Flag);
//@Read : Std$Integer$SmallT
// Open a file for reading.
//@Write : Std$Integer$SmallT
// Open a file for writing.
//@Append : Std$Integer$SmallT
// Start at the end of the file.
//@Text : Std$Integer$SmallT
// Open a file for reading or writing text. This adds <id>IO/Stream/TextReaderT</id> and / or <id>IO/Stream/TextWriterT</id> as parents.
//@NoBlock : Std$Integer$SmallT
// Open a file in non-blocking mode.

#if defined(WINDOWS) && !defined(CYGWIN)

typedef struct {const Std$Type$t *Type; int Access, Create;} openmode_t;

static openmode_t OpenModes[] = {
	{T, 0, 0}, {ReaderT, GENERIC_READ, OPEN_EXISTING},
	{WriterT, GENERIC_WRITE, 0}, {ReaderWriterT, GENERIC_READ | GENERIC_WRITE, 0},
	{T, 0, 0}, {TextReaderT, GENERIC_READ, OPEN_EXISTING, 0},
	{TextWriterT, GENERIC_WRITE, 0}, {TextReaderWriterT, GENERIC_READ | GENERIC_WRITE, 0}
};

#else

typedef struct {const Std$Type$t *Type; int Flags;} openmode_t;

static openmode_t OpenModes[] = {
	{T, 0}, {ReaderT, O_RDONLY}, {WriterT, O_WRONLY | O_CREAT}, {ReaderWriterT, O_RDWR | O_CREAT},
	{T, 0}, {TextReaderT, O_RDONLY}, {TextWriterT, O_WRONLY | O_CREAT}, {TextReaderWriterT, O_RDWR | O_CREAT}
};

#endif

GLOBAL_FUNCTION(Open, 2) {
//@filename : Std$String$T
//@mode : Std$Integer$T
//:T
//Opens filename with the correct mode and returns a file object
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	const Std$String$t *Arg0 = (Std$String$t *)Args[0].Val;
	int Flags = ((Std$Integer$smallt *)Args[1].Val)->Value;
#if defined(WINDOWS) && !defined(CYGWIN)
    char *FileName = Std$String$flatten(Args[0].Val);
    openmode_t OpenMode = OpenModes[Flags % 8];
    if (Flags & IO$File$OPEN_WRITE) {
    	OpenMode.Create = (Flags & IO$File$OPEN_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS;
	};
    HANDLE Handle = CreateFile(FileName, OpenMode.Access, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OpenMode.Create, 0, 0);
    if (Handle == INVALID_HANDLE_VALUE) {
        Result->Val = Sys$Program$error_from_errno(IO$Stream$OpenMessageT);
		return MESSAGE;
	};
	if (Flags & IO$File$OPEN_APPEND) {
		SetFilePointer(Handle, 0, 0, FILE_END);
	};
	Result->Val = IO$Native$(new)(OpenMode.Type, Handle);
	return SUCCESS;
#else
	char FileName[Arg0->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
	openmode_t OpenMode = OpenModes[Flags % 8];
	int Flags0 = OpenMode.Flags;
	if (Flags & IO$File$OPEN_NOBLOCK) Flags0 |= O_NONBLOCK;
	if (Flags & IO$File$OPEN_EXCLUSIVE) Flags0 |= O_EXCL;
	if (Flags & IO$File$OPEN_TRUNCATE) Flags0 |= O_TRUNC;
	int Handle = open64(FileName, Flags0, 0644);
	if (Handle < 0) {
		Result->Val = Sys$Program$error_from_errno(IO$Stream$OpenMessageT);
		return MESSAGE;
	};
	if (Flags & IO$File$OPEN_APPEND) {
		lseek(Handle, 0, SEEK_END);
	} else if (Flags & IO$File$OPEN_WRITE) {
		int Tmp = ftruncate(Handle, 0);
	};
	Result->Val = IO$Native$(new)(OpenMode.Type, Handle);
	return SUCCESS;
#endif
};

#if defined(WINDOWS) && !defined(CYGWIN)
IO$Native$(_t) *__file_open(const char *FileName, int Flags) {
    openmode_t OpenMode = OpenModes[Flags % 8];
    HANDLE Handle = CreateFile(FileName, OpenMode.Access, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OpenMode.Create, 0, 0);
	if (Handle == INVALID_HANDLE_VALUE) return 0;
	if (Flags & IO$File$OPEN_APPEND) SetFilePointer(Handle, 0, 0, FILE_END);
	return IO$Native$(new)(OpenMode.Type, Handle);
};

GLOBAL_FUNCTION(Temp, 0) {
};

#else

Std$Object$t *__file_open(const char *FileName, int Flags) {
	int Mode;
	openmode_t OpenMode = OpenModes[Flags % 8];
	int Flags0 = OpenMode.Flags;
	if (Flags & IO$File$OPEN_NOBLOCK) Flags += O_NONBLOCK;
	int Handle = open(FileName, Flags0, 0644);
	if (Handle < 0) return 0;
	if (Flags & IO$File$OPEN_APPEND) lseek(Handle, 0, SEEK_END);
	return IO$Native$(new)(OpenMode.Type, Handle);
};

GLOBAL_FUNCTION(Temp, 0) {
//:T
// Creates and opens a temporary file. The file is automatically deleted when closed.
	int Handle = fileno(tmpfile());
	if (Handle) {
		Result->Val = IO$Native$(new)(TextReaderWriterT, Handle);;
		return SUCCESS;
	} else {
		Result->Val = Sys$Program$error_new_format(IO$Stream$OpenMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	};
};

GLOBAL_FUNCTION(Pipe, 2) {
//@rd+:
//@wr+:
// Creates a pipe and storing the reader / writer streams in <var>rd</var> / <var>wr</var> respectively.
	int Handles[2];
	if (pipe(Handles)) {
		Result->Val = Sys$Program$error_new_format(IO$Stream$OpenMessageT, "%s:%d", __FILE__, __LINE__);
		return MESSAGE;
	} else {
		if (Args[0].Ref) Args[0].Ref[0] = IO$Native$(new)(TextReaderT, Handles[0]);
		if (Args[1].Ref) Args[1].Ref[0] = IO$Native$(new)(TextWriterT, Handles[1]);
		return SUCCESS;
	};
};

#endif
