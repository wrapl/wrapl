#include <Std.h>
#include <Sys/Time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <Riva/Memory.h>
#include <Sys/Module.h>

#ifdef WINDOWS
#include <windows.h>
#define PATHSEPCHR '\\'
#define PATHSEPSTR "\\"
#else
#define PATHSEPCHR '/'
#define PATHSEPSTR "/"
#endif

#ifdef WINDOWS
Std$Integer_smallt BLOCKFILE[] = {{Std$Integer$SmallT, 0}};
Std$Integer_smallt CHARFILE[] = {{Std$Integer$SmallT, 0}};
Std$Integer_smallt FIFOFILE[] = {{Std$Integer$SmallT, 0}};
Std$Integer_smallt REGFILE[] = {{Std$Integer$SmallT, 0}};
Std$Integer_smallt DIRFILE[] = {{Std$Integer$SmallT, 0}};
Std$Integer_smallt LINKFILE[] = {{Std$Integer$SmallT, 0}};
#else
Std$Integer_smallt BLOCKFILE[] = {{Std$Integer$SmallT, S_IFBLK}};
Std$Integer_smallt CHARFILE[] = {{Std$Integer$SmallT, S_IFCHR}};
Std$Integer_smallt FIFOFILE[] = {{Std$Integer$SmallT, S_IFIFO}};
Std$Integer_smallt REGFILE[] = {{Std$Integer$SmallT, S_IFREG}};
Std$Integer_smallt DIRFILE[] = {{Std$Integer$SmallT, S_IFDIR}};
Std$Integer_smallt LINKFILE[] = {{Std$Integer$SmallT, S_IFLNK}};
#endif

CONSTANT(Type, Sys$Module$T) {
// <code>.Block</code>: Block device.
// <code>.Char</code>: Char file.
// <code>.Fifo</code>: Fifo file.
// <code>.Reg</code>: Regular file.
// <code>.Dir</code>: Directory.
// <code>.Link</code>: Symbolic link.
	Sys$Module_t *Module = Sys$Module$new("Type");
	Sys$Module$export(Module, "Block", 0, BLOCKFILE);
	Sys$Module$export(Module, "Char", 0, CHARFILE);
	Sys$Module$export(Module, "Fifo", 0, FIFOFILE);
	Sys$Module$export(Module, "Reg", 0, REGFILE);
	Sys$Module$export(Module, "Dir", 0, DIRFILE);
	Sys$Module$export(Module, "Link", 0, LINKFILE);
	return Module;
};

typedef struct message_t {
	const Std$Type_t *Type;
	const char *Message;
} message_t;

TYPE(MessageT);
// Base type for error messages sent from file system methods.

AMETHOD(Std$String$Of, TYP, MessageT) {
//@msg
//:Std$String$T
	message_t *Msg = Args[0].Val;
	Result->Val = Std$String$new(Msg->Message);
	return SUCCESS;
};

TYPE(DirectoryCreateMessageT, MessageT);
TYPE(DirectoryRemoveMessageT, MessageT);
TYPE(DirectoryOpenMessageT, MessageT);
TYPE(FileRemoveMessageT, MessageT);
TYPE(FileRenameMessageT, MessageT);
TYPE(FileLinkMessageT, MessageT);
TYPE(FileSetPermissionsMessageT, MessageT);

static message_t DirectoryCreateMessage[] = {{DirectoryCreateMessageT, "Directory Create Error"}};
static message_t DirectoryRemoveMessage[] = {{DirectoryRemoveMessageT, "Directory Remove Error"}};
static message_t DirectoryOpenMessage[] = {{DirectoryOpenMessageT, "Directory Open Error"}};
static message_t FileRemoveMessage[] = {{FileRemoveMessageT, "File Remove Error"}};
static message_t FileRenameMessage[] = {{FileRenameMessageT, "File Rename Error"}};
static message_t FileLinkMessage[] = {{FileLinkMessageT, "File Link Error"}};
static message_t FileSetPermissionsMessage[] = {{FileSetPermissionsMessageT, "File Set Permissions Error"}};


CONSTANT(Message, Sys$Module$T) {
// <code>.Error</code>: Base type for error messages sent by file system methods.
// <code>.DirectoryCreateError</code>: An error occured while creating a directory.
// <code>.DirectoryRemoveError</code>: An error occured while removing a directory.
// <code>.DirectoryOpenError</code>: An error occured while opening a directory.
// <code>.FileRemoveError</code>: An error occured while removing a file.
// <code>.FileRenameError</code>: An error occured while renaming a file.
// <code>.FileSetPermissionsError</code>: An error occured while setting permissions for a file.
	Sys$Module_t *Module = Sys$Module$new("Message");
	Sys$Module$export(Module, "Error", 0, MessageT);
	Sys$Module$export(Module, "DirectoryCreateError", 0, DirectoryCreateMessageT);
	Sys$Module$export(Module, "DirectoryRemoveError", 0, DirectoryRemoveMessageT);
	Sys$Module$export(Module, "DirectoryOpenError", 0, DirectoryOpenMessageT);
	Sys$Module$export(Module, "FileRemoveError", 0, FileRemoveMessageT);
	Sys$Module$export(Module, "FileRenameError", 0, FileRenameMessageT);
	Sys$Module$export(Module, "FileLinkError", 0, FileLinkMessageT);
	Sys$Module$export(Module, "FileSetPermissions", 0, FileSetPermissionsMessageT);
	return Module;
};

TYPE(InfoT);
// Contains information about files

typedef struct info_t {
	const Std$Type_t *Type;
	const Std$Object_t *Name;
	const Std$Object_t *Kind;
	const Std$Object_t *Size;
	const Std$Object_t *Created;
	const Std$Object_t *Modified;
	const Std$Object_t *Accessed;
} info_t;

METHOD("name", TYP, InfoT) {
//@info
//:Std$String$T
// Returns the name of the file.
	Result->Val = ((info_t *)Args[0].Val)->Name;
	return SUCCESS;
};

METHOD("type", TYP, InfoT) {
//@info
//:Std$Integer$SmallT
// Returns a bitmask of the types of the file.
	Result->Val = ((info_t *)Args[0].Val)->Kind;
	return SUCCESS;
};

METHOD("size", TYP, InfoT) {
//@info
//:Std$Integer$T
// Returns the size of the file.
	Result->Val = ((info_t *)Args[0].Val)->Size;
	return SUCCESS;
};

METHOD("modified", TYP, InfoT) {
//@info
//:Std$Integer$T
// Returns the modified time of the file.
	Result->Val = ((info_t *)Args[0].Val)->Modified;
	return SUCCESS;
};

GLOBAL_FUNCTION(Path, 0) {

};

GLOBAL_FUNCTION(BaseName, 1) {
//@filename:Std$String$T
// Returns the base part of <var>filename</var>
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Result->Val = Std$String$copy(basename(Std$String$flatten(Args[0].Val)));
	return SUCCESS;
};

GLOBAL_FUNCTION(DirName, 1) {
//@filename:Std$String$T
// Returns the directory part of <var>filename</var>
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	char Buffer[Std$String$get_length(Args[0].Val) + 1];
	Std$String$flatten_to(Args[0].Val, Buffer);
	if (Count > 1) {
		CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
		for (int I = Std$Integer$get_small(Args[1].Val) + 1; --I;) dirname(Buffer);
		Result->Val = Std$String$copy(Buffer);
	} else {
		Result->Val = Std$String$copy(dirname(Buffer));
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(MakeDir, 2) {
//@path:Std$String$T
//@mode:Std$Integer$SmallT
// Creates a new directory at <var>path</var> with permissions <var>mode</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
#ifdef WINDOWS
	if (CreateDirectory(Std$String$flatten(Args[0].Val), 0) == 0) {
#else
	if (mkdir(Std$String$flatten(Args[0].Val), ((Std$Integer_smallt *)Args[1].Val)->Value)) {
#endif
		Result->Val = DirectoryCreateMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(RemoveDir, 1) {
//@path:Std$String$T
// Removes the directory <var>path</var>. The directory should be empty.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	char DirName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, DirName);
#ifdef WINDOWS
	if (RemoveDirectory(DirName) == 0) {
#else
	if (rmdir(DirName)) {
#endif
		Result->Val = DirectoryRemoveMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(Copy, 2) {
//@source:Std$String$T
//@dest:Std$String$T
// Copies <var>source</var> to <var>dest</var>.
// <em>Not implemented yet</em>.
};

GLOBAL_FUNCTION(Link, 2) {
//@old:Std$String$T
//@new:Std$String$T
// Links the file <var>old</var> to <var>new</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	char OldName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	char NewName[((Std$String_t *)Args[1].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[1].Val, NewName);
	Std$String$flatten_to(Args[0].Val, OldName);
#ifdef WINDOWS
	//if (MoveFile(OldName, NewName) == 0) {
#else
	if (link(OldName, NewName) == -1) {
#endif
		Result->Val = FileLinkMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(SymLink, 2) {
//@old:Std$String$T
//@new:Std$String$T
// Links the file <var>old</var> to <var>new</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	char OldName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	char NewName[((Std$String_t *)Args[1].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[1].Val, NewName);
	Std$String$flatten_to(Args[0].Val, OldName);
#ifdef WINDOWS
	//if (MoveFile(OldName, NewName) == 0) {
#else
	if (symlink(OldName, NewName) == -1) {
#endif
		Result->Val = FileLinkMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(Remove, 1) {
//@file:Std$String$T
// Removes the file <var>file</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	char FileName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
#ifdef WINDOWS
	if (DeleteFile(FileName) == 0) {
#else
	if (unlink(FileName)) {
#endif
		Result->Val = FileRemoveMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(Rename, 2) {
//@old:Std$String$T
//@new:Std$String$T
// Renames the file <var>old</var> to <var>new</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, Std$String$T);
	char OldName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	char NewName[((Std$String_t *)Args[1].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[1].Val, NewName);
	Std$String$flatten_to(Args[0].Val, OldName);
#ifdef WINDOWS
	if (MoveFile(OldName, NewName) == 0) {
#else
	if (rename(OldName, NewName) == -1) {
#endif
		Result->Val = FileRenameMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_FUNCTION(SetPermissions, 2) {
//@file:Std$String$T
//@perm:Std$Integer$SmallT
// Sets the permissions for the file <var>file</var> to <var>perm</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	CHECK_EXACT_ARG_TYPE(1, Std$Integer$SmallT);
	char FileName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
#ifdef WINDOWS
#else
	if (chmod(FileName, Std$Integer$get_small(Args[1].Val))) {
#endif
		Result->Val = FileSetPermissionsMessage;
		return MESSAGE;
	};
	return SUCCESS;
};

typedef struct listdir_generator {
	Std$Function_cstate State;
#ifdef WINDOWS
#else
	DIR *Dir;
#endif
	char *PathName, *FileName;
} listdir_generator;

static void listdir_finalize(listdir_generator *Generator, void *Arg) {
#ifdef LINUX
	if (Generator) closedir(Generator->Dir);
#endif
};

static Std$Function$status listdir_resume(Std$Function$result *Result) {
#ifdef WINDOWS
#else
	listdir_generator *Generator = (listdir_generator *)Result->State;
	struct dirent *Entry = readdir(Generator->Dir);
	if (Entry == 0) {
		closedir(Generator->Dir);
		Generator->Dir = 0;
		Riva$Memory$register_finalizer(Generator, 0, 0, 0, 0);
		return FAILURE;
	};
	Result->Val = Std$String$copy(Entry->d_name);
	return SUSPEND;
#endif
};

GLOBAL_FUNCTION(ListDir, 1) {
//@dir:Std$String$T
//:Std$String$T
// Generates the names of the files in <var>dir</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
#ifdef WINDOWS
#else
	char DirName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, DirName);
	DIR *Dir = opendir(DirName);
	if (Dir == 0) {
		Result->Val = DirectoryOpenMessage;
		return MESSAGE;
	};
	struct dirent *Entry = readdir(Dir);
	if (Entry == 0) {
		closedir(Dir);
		return FAILURE;
	};
	Result->Val = Std$String$copy(Entry->d_name);
	listdir_generator *Generator = new(listdir_generator);
	Riva$Memory$register_finalizer(Generator, listdir_finalize, 0, 0, 0);
	if (Generator->State.Chain) Riva$Memory$register_disappearing_link(&Generator->State.Chain, Generator->State.Chain);
	Generator->State.Run = Std$Function$resume_c;
	Generator->State.Invoke = listdir_resume;
	Generator->Dir = Dir;
	Result->State = Generator;
	return SUSPEND;
#endif
};

static long listdirinfo_resume(Std$Function$result *Result) {
#ifdef WINDOWS
#else
	listdir_generator *Generator = (listdir_generator *)Result->State;
	struct dirent *Entry = readdir(Generator->Dir);
	if (Entry == 0) {
		closedir(Generator->Dir);
		Generator->Dir = 0;
		Riva$Memory$register_finalizer(Generator, 0, 0, 0, 0);
		return FAILURE;
	};
	info_t *Info = new(info_t);
	Info->Type = InfoT;
	Info->Name = Std$String$copy(Entry->d_name);
	switch (Entry->d_type) {
	case DT_REG:
		Info->Kind = REGFILE;
		break;
	case DT_DIR:
		Info->Kind = DIRFILE;
		break;
	case DT_FIFO:
		Info->Kind = FIFOFILE;
		break;
//	case DT_SOCK:
//		Info->Kind = SOCKFILE;
//		break;
	case DT_CHR:
		Info->Kind = CHARFILE;
		break;
	case DT_BLK:
		Info->Kind = BLOCKFILE;
		break;
	default:
		Info->Kind = Std$Object$Nil;
		break;
	};
	struct stat Stat;
	strcpy(Generator->FileName, Entry->d_name);
	if (stat(Generator->PathName, &Stat) == 0) {
		Info->Size = Std$Integer$new_small(Stat.st_size);
		Info->Modified = Sys$Time$new(Stat.st_mtime);
	} else {
		Info->Size = Std$Object$Nil;
		Info->Modified = Std$Object$Nil;
	};
	Result->Val = Info;
	return SUSPEND;
#endif
};

GLOBAL_FUNCTION(ListDirInfo, 1) {
//@dir:Std$String$T
//:InfoT
// Generates an instance of <id>InfoT</id> for each file in <var>dir</var>.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
#ifdef WINDOWS
#else
	char *PathName = Riva$Memory$alloc_atomic(((Std$String_t *)Args[0].Val)->Length.Value + 1 + 256);
	Std$String$flatten_to(Args[0].Val, PathName);
	DIR *Dir = opendir(PathName);
	if (Dir == 0) {
		Result->Val = DirectoryOpenMessage;
		return MESSAGE;
	};
	char *FileName = PathName + ((Std$String_t *)Args[0].Val)->Length.Value;
	*(FileName++) = '/';
	struct dirent *Entry = readdir(Dir);
	if (Entry == 0) {
		closedir(Dir);
		return FAILURE;
	};
	info_t *Info = new(info_t);
	Info->Type = InfoT;
	Info->Name = Std$String$copy(Entry->d_name);
	switch (Entry->d_type) {
	case DT_REG:
		Info->Kind = REGFILE;
		break;
	case DT_DIR:
		Info->Kind = DIRFILE;
		break;
	case DT_FIFO:
		Info->Kind = FIFOFILE;
		break;
//	case DT_SOCK:
//		Info->Kind = SOCKFILE;
//		break;
	case DT_CHR:
		Info->Kind = CHARFILE;
		break;
	case DT_BLK:
		Info->Kind = BLOCKFILE;
		break;
	default:
		Info->Kind = Std$Object$Nil;
		break;
	};
	struct stat Stat;
	strcpy(FileName, Entry->d_name);
	if (stat(PathName, &Stat) == 0) {
		Info->Size = Std$Integer$new_small(Stat.st_size);
		Info->Modified = Sys$Time$new(Stat.st_mtime);
	} else {
		Info->Size = Std$Object$Nil;
		Info->Modified = Std$Object$Nil;
	};
	Result->Val = Info;
	listdir_generator *Generator = new(listdir_generator);
	Riva$Memory$register_finalizer(Generator, listdir_finalize, 0, 0, 0);
	Riva$Memory$register_disappearing_link(&Generator->State.Chain, Generator->State.Chain);
	Generator->State.Run = Std$Function$resume_c;
	Generator->State.Invoke = listdirinfo_resume;
	Generator->Dir = Dir;
	Generator->PathName = PathName;
	Generator->FileName = FileName;
	Result->State = Generator;
	return SUSPEND;
#endif
};

GLOBAL_FUNCTION(Exists, 1) {
//@filename:Std$String$T
//:Std$String$T
// Returns <var>filename</var> if it denotes an existing file/directory. Fails otherwise.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
#ifdef WINDOWS
	char FileName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
	if (GetFileAttributes(FileName) == INVALID_FILE_ATTRIBUTES) {
		return FAILURE;
	} else {
		Result->Val = Args[0].Val;
		return SUCCESS;
	};
#else
	char FileName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
	struct stat Stat;
	if (stat(FileName, &Stat) == 0) {
		Result->Val = Args[0].Val;
		return SUCCESS;
	} else {
		return FAILURE;
	};
#endif
};

GLOBAL_FUNCTION(FileSize, 1) {
//@filename:Std$String$T
//:Std$Integer$T
// Returns the type of <var>filename</var> if it exists. Fails otherwise.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	char FileName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
#ifdef WINDOWS
#else
	struct stat Stat;
	if (stat(FileName, &Stat) == 0) {
		Result->Val = Std$Integer$new_small(Stat.st_size);
		return SUCCESS;
	} else {
		return FAILURE;
	};
#endif
};

GLOBAL_FUNCTION(FileType, 1) {
//@filename:Std$String$T
//:Std$Integer$T
// Returns the type of <var>filename</var> if it exists. Fails otherwise.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	char FileName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
#ifdef WINDOWS
#else
	struct stat Stat;
	if (stat(FileName, &Stat) == 0) {
		Result->Val = Std$Integer$new_small(Stat.st_mode);
		return SUCCESS;
	} else {
		return FAILURE;
	};
#endif
};

GLOBAL_FUNCTION(FileTime, 1) {
//@filename:Std$String$T
//:Sys$Time$T
// Returns the modification time of <var>filename</var> if it exists. Fails otherwise.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	char FileName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
#ifdef WINDOWS
#else
	struct stat Stat;
	if (stat(FileName, &Stat) == 0) {
		Result->Val = Sys$Time$new(Stat.st_mtime);
		return SUCCESS;
	} else {
		return FAILURE;
	};
#endif
};

GLOBAL_FUNCTION(FileInfo, 1) {
//@filename:Std$String$T
//:InfoT
// Returns an instance of <id>InfoT</id> describing <var>filename</var> if it exists. Fail otherwise.
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	char FileName[((Std$String_t *)Args[0].Val)->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
#ifdef WINDOWS
#else
	struct stat Stat;
	if (stat(FileName, &Stat) == 0) {
		info_t *Info = new(info_t);
		Info->Type = InfoT;
		Info->Name = Args[0].Val;
		Info->Kind = Std$Integer$new_small(Stat.st_mode);
		Info->Size = Std$Integer$new_small(Stat.st_size);
		Result->Val = Info;
		return SUCCESS;
	} else {
		return FAILURE;
	};
#endif
};

GLOBAL_FUNCTION(TempFile, 0) {
//:Std$String$T
// Returns the name of a temporary file.
	Result->Val = Std$String$copy(tmpnam(0));
	return SUCCESS;
};

GLOBAL_FUNCTION(MakePipe, 1) {
	CHECK_EXACT_ARG_TYPE(0, Std$String$T);
	Std$String_t *Arg0 = Args[0].Val;
#ifdef WINDOWS
#else
	char FileName[Arg0->Length.Value + 1];
	Std$String$flatten_to(Args[0].Val, FileName);
	if (mkfifo(FileName, 0644)) return FAILURE;
	Result->Val = Arg0;
	return SUCCESS;
#endif
};
