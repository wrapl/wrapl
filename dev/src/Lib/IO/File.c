#include <IO/File.h>
#include <Riva/Memory.h>
#include <Sys/Module.h>
#include <Std.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

TYPE(T, NATIVE($SeekerT), NATIVE($T), IO$Stream$SeekerT, IO$Stream$T);

TYPE(ReaderT, T, NATIVE($ReaderT), NATIVE($SeekerT), NATIVE($T), IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(WriterT, T, NATIVE($WriterT), NATIVE($SeekerT), NATIVE($T), IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(ReaderWriterT, ReaderT, WriterT, T, NATIVE($ReaderT), NATIVE($WriterT), NATIVE($SeekerT), NATIVE($T), IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

TYPE(TextReaderT, ReaderT, T, NATIVE($TextReaderT), NATIVE($ReaderT), NATIVE($SeekerT), NATIVE($T), IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(TextWriterT, WriterT, T, NATIVE($TextWriterT), NATIVE($WriterT), NATIVE($SeekerT), NATIVE($T), IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(TextReaderWriterT, ReaderT, WriterT, T, NATIVE($TextReaderT), NATIVE($TextWriterT), NATIVE($ReaderT), NATIVE($WriterT), NATIVE($SeekerT), NATIVE($T), IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

TYPE(OpenMessageT, IO$Stream$MessageT);
static IO$Stream_messaget OpenMessage[] = {{OpenMessageT, "Open Error"}};

CONSTANT(Message, Sys$Module$T) {
	Sys$Module_t *Module = Sys$Module$new("Message");
	Sys$Module$export(Module, "OpenErrorT", 0, (void *)OpenMessageT);
	return (Std$Object_t *)Module;
};

Std$Integer_smallt READ[] = {{Std$Integer$SmallT, IO$File$OPEN_READ}};
Std$Integer_smallt WRITE[] = {{Std$Integer$SmallT, IO$File$OPEN_WRITE}};
Std$Integer_smallt APPEND[] = {{Std$Integer$SmallT, IO$File$OPEN_APPEND}};
Std$Integer_smallt TEXT[] = {{Std$Integer$SmallT, IO$File$OPEN_TEXT}};
Std$Integer_smallt NOBLOCK[] = {{Std$Integer$SmallT, IO$File$OPEN_NOBLOCK}};

CONSTANT(Flag, Sys$Module$T) {
// Flags for selecting opening mode <dl class="submodule">
// <dt><code>.</code>Read : <id>Std/Integer/SmallT</id></dt><dd>Open a file for reading.</dd>
// <dt><code>.</code>Write : <id>Std/Integer/SmallT</id></dt><dd>Open a file for writing.</dd>
// <dt><code>.</code>Text : <id>Std/Integer/SmallT</id></dt><dd>Open a file in text mode (returned object is a <id>IO/Stream/TextReaderT</id> or <id>IO/Stream/TextWriterT</id>.</dd>
// <dt><code>.</code>Append : <id>Std/Integer/SmallT</id></dt><dd>For files opened in write mode, append new output to the end of the file.</dd>
// </dl>
	Sys$Module_t *Module = Sys$Module$new("Flag");
	Sys$Module$export(Module, "Read", 0, (void *)READ);
	Sys$Module$export(Module, "Write", 0, (void *)WRITE);
	Sys$Module$export(Module, "Text", 0, (void *)TEXT);
	Sys$Module$export(Module, "Append", 0, (void *)APPEND);
	Sys$Module$export(Module, "NoBlock", 0, (void *)NOBLOCK);
	return (Std$Object_t *)Module;
};

#if defined(WINDOWS) && !defined(CYGWIN)

typedef struct {const Std$Type_t *Type; int Access, Create;} openmode_t;

static openmode_t OpenModes[] = {
	{T, 0, 0}, {ReaderT, GENERIC_READ, OPEN_EXISTING},
	{WriterT, GENERIC_WRITE, 0}, {ReaderWriterT, GENERIC_READ | GENERIC_WRITE, 0},
	{T, 0, 0}, {TextReaderT, GENERIC_READ, OPEN_EXISTING, 0},
	{TextWriterT, GENERIC_WRITE, 0}, {TextReaderWriterT, GENERIC_READ | GENERIC_WRITE, 0}
};

#else

typedef struct {const Std$Type_t *Type; int Flags;} openmode_t;

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
	const Std$String_t *Arg0 = (Std$String_t *)Args[0].Val;
	int Flags = ((Std$Integer_smallt *)Args[1].Val)->Value;
#if defined(WINDOWS) && !defined(CYGWIN)
    char *FileName = Std$String$flatten(Args[0].Val);
    openmode_t OpenMode = OpenModes[Flags % 8];
    if (Flags & IO$File$OPEN_WRITE) {
    	OpenMode.Create = (Flags & IO$File$OPEN_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS;
	};
    HANDLE Handle = CreateFile(FileName, OpenMode.Access, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OpenMode.Create, 0, 0);
    if (Handle == INVALID_HANDLE_VALUE) {
        Result->Val = (Std$Object_t *)OpenMessage;
		return MESSAGE;
	};
	if (Flags & IO$File$OPEN_APPEND) {
		SetFilePointer(Handle, 0, 0, FILE_END);
	};
	Result->Val = NATIVE($new)(OpenMode.Type, Handle);
	return SUCCESS;
#else
	char FileName[Arg0->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
	openmode_t OpenMode = OpenModes[Flags % 8];
	int Flags0 = OpenMode.Flags;
	if (Flags & IO$File$OPEN_NOBLOCK) Flags0 += O_NONBLOCK;
	int Handle = open(FileName, Flags0, 0644);
	if (Handle < 0) {
		Result->Val = (Std$Object_t *)OpenMessage;
		return MESSAGE;
	};
	if (Flags & IO$File$OPEN_APPEND) {
		lseek(Handle, 0, SEEK_END);
	} else if (Flags & IO$File$OPEN_WRITE) {
		int Tmp = ftruncate(Handle, 0);
	};
	Result->Val = NATIVE($new)(OpenMode.Type, Handle);
	return SUCCESS;
#endif
};

#if defined(WINDOWS) && !defined(CYGWIN)
NATIVE(_t) *__file_open(const char *FileName, int Flags) {
    openmode_t OpenMode = OpenModes[Flags % 8];
    HANDLE Handle = CreateFile(FileName, OpenMode.Access, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OpenMode.Create, 0, 0);
	if (Handle == INVALID_HANDLE_VALUE) return 0;
	if (Flags & IO$File$OPEN_APPEND) SetFilePointer(Handle, 0, 0, FILE_END);
	return NATIVE($new)(OpenMode.Type, Handle);
};

GLOBAL_FUNCTION(Temp, 0) {
};

#else

Std$Object_t *__file_open(const char *FileName, int Flags) {
	int Mode;
	openmode_t OpenMode = OpenModes[Flags % 8];
	int Flags0 = OpenMode.Flags;
	if (Flags & IO$File$OPEN_NOBLOCK) Flags += O_NONBLOCK;
	int Handle = open(FileName, Flags0, 0644);
	if (Handle < 0) return 0;
	if (Flags & IO$File$OPEN_APPEND) lseek(Handle, 0, SEEK_END);
	return NATIVE($new)(OpenMode.Type, Handle);
};

GLOBAL_FUNCTION(Temp, 0) {
//:T
// Creates and opens a temporary file. The file is automatically deleted when closed.
	int Handle = fileno(tmpfile());
	if (Handle) {
		Result->Val = NATIVE($new)(TextReaderWriterT, Handle);;
		return SUCCESS;
	} else {
		Result->Val = (Std$Object_t *)OpenMessage;
		return MESSAGE;
	};
};

GLOBAL_FUNCTION(Pipe, 2) {
//rd:
//wr:
// Creates a pipe and storing the reader / writer streams in <var>rd</var> / <var>wr</var> respectively.
	int Handles[2];
	if (pipe(Handles)) {
		Result->Val = (Std$Object_t *)OpenMessage;
		return MESSAGE;
	} else {
		if (Args[0].Ref) Args[0].Ref[0] = NATIVE($new)(TextReaderT, Handles[0]);
		if (Args[1].Ref) Args[1].Ref[0] = NATIVE($new)(TextWriterT, Handles[1]);
		return SUCCESS;
	};
};

#endif
