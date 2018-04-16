#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <bfd.h>
#include <fcntl.h>
#include <zlib.h>
#include <sys/stat.h>
#include <lua.h>
#include <lauxlib.h>

#ifdef USE_UDIS
#include <udis86.h>
#endif

#define RIVA_VERSION 3

#define new(T) ((T *)malloc(sizeof(T)))

#include <libHX/map.h>

enum {
	SECT_GENERIC,
	SECT_TEXT,
	SECT_DATA,
	SECT_LIBRARY,
	SECT_IMPORT,
	SECT_BSS,
	SECT_SYMBOL,
	SECT_CONSTANT,
	SECT_INITIAL,
	SECT_METHODS,
	SECT_TYPEDFN,
	SECT_MODULE,
	SECT_TDATA,
	SECT_TBSS,
	SECT_GOT
};

#define EXP_CONSTANT 0
#define EXP_VARIABLE 1

#define RELOC_ABS	0
#define RELOC_REL	1
#define RELOC_IND	2
#define RELOC_GOT	3

#define LIBRARY_ABS	0
#define LIBRARY_REL	1

#define FLAG_GC		1
#define FLAG_DELAY	2

#define SEC_UNUSED 0xFFFFFFFF

typedef struct define_t define_t;

struct define_t {
	define_t *Next;
	const char *Name, *Value;
};

typedef struct section_t section_t;
typedef struct relocation_t relocation_t;
typedef struct export_t export_t;

typedef struct section_methods {
	void (*setup)(section_t *Section);
	void (*relocate)(section_t *Section, relocation_t *Relocation, uint32_t *Target);
	void (*export)(section_t *Section, uint32_t Offset, export_t *Export);
	void (*debug)(section_t *Section, FILE *File);
	uint32_t (*size)(section_t *Section);
	void (*write)(section_t *Section, gzFile File);
} section_methods;

struct section_t {
	section_t *Next;
	section_methods *Methods;
	uint32_t Index;
	const char *Name;
};

struct relocation_t {
	uint32_t Flags;
	uint32_t Position;
	uint32_t Size;
	section_t *Section;
};

struct export_t {
	export_t *Next;
	section_t *Section;
	const char *Internal, *External;
	uint32_t Flags;
	uint32_t Offset;
};

typedef struct require_t require_t;

struct require_t {
	require_t *Next;
	uint32_t Flags;
	const char *Library;
};

static struct {section_t *Head, *Tail;} Sections = {0, 0};
static struct {export_t *Head, *Tail;} Exports = {0, 0};
static struct {require_t *Head, *Tail;} Requires = {0, 0};

static section_t *InitialSection = 0;
static section_t *MethodsSection = 0;
static section_t *TypedFnSection = 0;
static section_t *GlobalOffsetTable = 0;

static struct HXmap *LibraryTable;
static struct HXmap *GlobalTable;
static struct HXmap *WeakTable;
static struct HXmap *SymbolTable;
static struct HXmap *ExportTable;
static struct HXmap *Dependencies;

static define_t *Defines = 0;
static int DependencyMode = 0;

static uint32_t NoOfSections = 0, NoOfExports = 0, NoOfRequires = 0;

#ifdef WINDOWS
static const char *Platform = "WINDOWS";
#else
#ifdef LINUX
static const char *Platform = "LINUX";
#else
static const char *Platform = "GENERIC";
#endif
#endif

static void new_require(uint32_t Flags, const char *Library) {
	require_t *Require = new(require_t);
	Require->Library = Library;
	Require->Flags = Flags;
	Require->Next = 0;
	++NoOfRequires;
	if (Requires.Head) {
		Requires.Tail->Next = Require;
		Requires.Tail = Require;
	} else {
		Requires.Head = Requires.Tail = Require;
	};
};

static void new_export(const char *Internal, const char *External, uint32_t Flags) {
	if (HXmap_get(ExportTable, External)) return;
	export_t *Export = new(export_t);
	Export->Internal = Internal;
	Export->External = External;
	Export->Flags = Flags;
	Export->Section = 0;
	Export->Next = 0;
	++NoOfExports;
	if (Exports.Head) {
		Exports.Tail->Next = Export;
		Exports.Tail = Export;
	} else {
		Exports.Head = Exports.Tail = Export;
	};
	HXmap_add(ExportTable, External, Export);
};

static inline void section_require(section_t *Section) {
	if (Section->Index != SEC_UNUSED) return;
	Section->Index = NoOfSections;
	NoOfSections++;
	Section->Next = 0;
	if (Sections.Head) {
		Sections.Tail->Next = Section;
		Sections.Tail = Section;
	} else {
		Sections.Head = Sections.Tail = Section;
	};
	Section->Methods->setup(Section);
};

static inline void section_write(section_t *Section, gzFile File) {
	Section->Methods->write(Section, File);
};

static inline void section_debug(section_t *Section, FILE *File) {
	Section->Methods->debug(Section, File);
};

static inline void section_relocate(section_t *Section, relocation_t *Relocation, uint32_t *Target) {
	Section->Methods->relocate(Section, Relocation, Target);
};

static inline void section_export(section_t *Section, uint32_t Offset, export_t *Export) {
	Section->Methods->export(Section, Offset, Export);
};

static void default_section_setup(section_t *Section) {
};

static void invalid_section_setup(section_t *Section) {
	fprintf(stderr, "%s: internal failure at line %d.\n", __FILE__, __LINE__);
	exit(1);
};

static void default_section_relocate(section_t *Section, relocation_t *Relocation, uint32_t *Target) {
	section_require(Section);
	Relocation->Section = Section;
};

static void invalid_section_relocate(section_t *Section, relocation_t *Relocation, uint32_t *Target) {
	fprintf(stderr, "%s: internal failure at line %d.\n", __FILE__, __LINE__);
	exit(1);
};

static void default_section_export(section_t *Section, uint32_t Offset, export_t *Export) {
	Export->Section = Section;
	Export->Offset = Offset;
};

static void invalid_section_export(section_t *Section, uint32_t Offset, export_t *Export) {
	fprintf(stderr, "%s: internal failure at line %d.\n", __FILE__, __LINE__);
	exit(1);
};

typedef struct symbol_t {
	const char *Name;
	section_t *Section;
	uint32_t Offset;
} symbol_t;

static symbol_t *new_symbol(const char *Name, section_t *Section, uint32_t Offset) {
	symbol_t *Symbol = new(symbol_t);
	Symbol->Name = Name;
	Symbol->Section = Section;
	Symbol->Offset = Offset;
	return Symbol;
};

typedef struct code_section_t {
	section_t Base;
	uint32_t Flags;
	uint32_t Size;
	uint32_t NoOfRelocs;
	uint8_t *Text;
	relocation_t Relocs[];
} code_section_t;

typedef struct library_section_t {
	section_t Base;
	uint32_t Flags;
	const char *Path;
	struct HXmap *Imports;
} library_section_t;

static void library_section_debug(library_section_t *Section, FILE *File) {
	fprintf(File, "%d: library section: %s\n", ((section_t *)Section)->Index, Section->Path);
};

static uint32_t library_section_size(library_section_t *Section) {
	if (Section->Path[0] == '.') {
		return 1 + 1 + 4 + strlen(Section->Path);
	} else {
		return 1 + 1 + 4 + strlen(Section->Path) + 1;
	};
};

static void library_section_write(library_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_LIBRARY; gzwrite(File, &Temp, 1);
	if (Section->Path[0] == '.') {
		Temp = LIBRARY_REL; gzwrite(File, &Temp, 1);
		Temp = strlen(Section->Path); gzwrite(File, &Temp, 4);
		gzwrite(File, Section->Path + 1, Temp);
	} else {
		Temp = LIBRARY_ABS; gzwrite(File, &Temp, 1);
		Temp = strlen(Section->Path) + 1; gzwrite(File, &Temp, 4);
		gzwrite(File, Section->Path, Temp);
	};
};

static library_section_t *new_library_section(const char *Path) {
	static section_methods Methods = {
		default_section_setup,
		default_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))library_section_debug,
		(uint32_t (*)(section_t *))library_section_size,
		(void (*)(section_t *, gzFile ))library_section_write
	};
	library_section_t *Section = new(library_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	Section->Imports = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
	Section->Path = Path;
	HXmap_add(LibraryTable, Path, Section);
	return Section;
};

typedef struct import_section_t {
	section_t Base;
	library_section_t *Library;
	const char *Name;
	uint32_t Type;
} import_section_t;

static void import_section_setup(import_section_t *Section) {
	section_require((section_t *)Section->Library);
};

static void import_section_debug(import_section_t *Section, FILE *File) {
	fprintf(File, "%d: import section: %d.%s\n", ((section_t *)Section)->Index, ((section_t *)Section->Library)->Index, Section->Name);
};

static uint32_t import_section_size(import_section_t *Section) {
	return 1 + 1 + 4 + 4 + strlen(Section->Name) + 1;
};

static void import_section_write(import_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_IMPORT; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	Temp = ((section_t *)Section->Library)->Index; gzwrite(File, &Temp, 4);
	Temp = strlen(Section->Name) + 1; gzwrite(File, &Temp, 4);
	gzwrite(File, Section->Name, Temp);
};

static import_section_t *new_import_section(library_section_t *Library, const char *Name, int Type) {
	static section_methods Methods = {
		(void (*)(section_t *))import_section_setup,
		default_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))import_section_debug,
		(uint32_t (*)(section_t *))import_section_size,
		(void (*)(section_t *, gzFile ))import_section_write
	};
	import_section_t *Section = new(import_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	Section->Name = Name;
	Section->Library = Library;
	Section->Type = Type;
	HXmap_add(Library->Imports, Name, Section);
	return Section;
};

typedef struct bss_section_t {
	section_t Base;
	uint32_t Size;
} bss_section_t;

static void bss_section_debug(bss_section_t *Section, FILE *File) {
	fprintf(File, "%d: bss section: %d\n", ((section_t *)Section)->Index, Section->Size);
};

static uint32_t bss_section_size(bss_section_t *Section) {
	return 1 + 1 + 4;
};

static void bss_section_write(bss_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_BSS; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	Temp = Section->Size; gzwrite(File, &Temp, 4);
};

static bss_section_t *new_bss_section(uint32_t Size) {
	static section_methods Methods = {
		default_section_setup,
		default_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))bss_section_debug,
		(uint32_t (*)(section_t *))bss_section_size,
		(void (*)(section_t *, gzFile ))bss_section_write
	};
	bss_section_t *Section = new(bss_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	Section->Size = Size;
	return Section;
};

typedef struct tbss_section_t {
	section_t Base;
	uint32_t Size;
} tbss_section_t;

static void tbss_section_relocate(tbss_section_t *Section, relocation_t *Relocation, uint32_t *Target) {
	section_require(Section);
	unsigned char *Temp = Target - 1;
	printf("%s: testing tbss relocations @ %d[%x, %d]:\n\t", ((section_t *)Section)->Name, Relocation->Position, Relocation->Flags, Relocation->Size);
	for (size_t I = 0; I < 12; ++I) printf("%x ", Temp[I]);
	printf("\n");
	Relocation->Section = Section;
};

static void tbss_section_debug(tbss_section_t *Section, FILE *File) {
	fprintf(File, "%d: tbss section: %d\n", ((section_t *)Section)->Index, Section->Size);
};

static uint32_t tbss_section_size(tbss_section_t *Section) {
	return 1 + 1 + 4;
};

static void tbss_section_write(tbss_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_TBSS; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	Temp = Section->Size; gzwrite(File, &Temp, 4);
};

static tbss_section_t *new_tbss_section(uint32_t Size) {
	static section_methods Methods = {
		default_section_setup,
		tbss_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))tbss_section_debug,
		(uint32_t (*)(section_t *))tbss_section_size,
		(void (*)(section_t *, gzFile ))tbss_section_write
	};
	tbss_section_t *Section = new(tbss_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	Section->Size = Size;
	return Section;
};

typedef struct symbol_section_t {
	section_t Base;
	const char *Name;
} symbol_section_t;

static void symbol_section_debug(symbol_section_t *Section, FILE *File) {
	fprintf(File, "%d: symbol section: \"%s\"\n", ((section_t *)Section)->Index, Section->Name);
};

static uint32_t symbol_section_size(symbol_section_t *Section) {
	if (Section->Name) {
		return 1 + 1 + 4 + strlen(Section->Name) + 1;
	} else {
		return 1 + 1 + 4;
	};
};

static void symbol_section_write(symbol_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_SYMBOL; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	if (Section->Name) {
		Temp = strlen(Section->Name) + 1; gzwrite(File, &Temp, 4);
		gzwrite(File, Section->Name, Temp);
	} else {
		Temp = 0xFFFFFFFF; gzwrite(File, &Temp, 4);
	};
};

static symbol_section_t *new_symbol_section(const char *Name) {
	static section_methods Methods = {
		default_section_setup,
		invalid_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))symbol_section_debug,
		(uint32_t (*)(section_t *))symbol_section_size,
		(void (*)(section_t *, gzFile ))symbol_section_write
	};
	symbol_section_t *Section = new(symbol_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	Section->Name = Name;
	return Section;
};

typedef struct symbols_section_t {
	section_t Base;
	const char *Symbols;
	symbol_section_t **ASymbols;
} symbols_section_t;

static void symbols_section_relocate(symbols_section_t *Section, relocation_t *Relocation, uint32_t *Target) {
	section_t *Symbol;
	if (Section->Symbols) {
		const char *Name = Section->Symbols + *Target;
		Symbol = (section_t *)HXmap_get(SymbolTable, Name);
		if (!Symbol) {
			Name = strdup(Name);
			Symbol = (section_t *)new_symbol_section(Name);
			HXmap_add(SymbolTable, Name, Symbol);
		};
	} else {
		Symbol = Section->ASymbols[*Target];
		if (!Symbol) {
			Section->ASymbols[*Target] = Symbol = (section_t *)new_symbol_section(0);
		};
	};
	*Target = 0;
	section_require(Symbol);
	Relocation->Section = Symbol;
};

static void symbols_section_export(symbols_section_t *Section, uint32_t Offset, export_t *Export) {
	section_t *Symbol;
	if (Section->Symbols) {
		const char *Name = Section->Symbols + Offset;
		Symbol = (section_t *)HXmap_get(SymbolTable, Name);
		if (!Symbol) {
			Name = strdup(Name);
			Symbol = (section_t *)new_symbol_section(Name);
			HXmap_add(SymbolTable, Name, Symbol);
		};
	} else {
		Symbol = Section->ASymbols[Offset];
		if (!Symbol) {
			Section->ASymbols[Offset] = Symbol = (section_t *)new_symbol_section(0);
		};
	};
	Export->Section = Symbol;
	Export->Offset = 0;
};

static symbols_section_t *new_symbols_section(asection *Sect, bfd *Bfd, int Anon) {
	static section_methods Methods = {
		invalid_section_setup,
		(void (*)(section_t *, relocation_t *, uint32_t *))symbols_section_relocate,
		(void (*)(section_t *, uint32_t, export_t *))symbols_section_export,
		0,
		0,
		0
	};
	symbols_section_t *Section = new(symbols_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	if (Anon) {
		Section->Symbols = 0;
		Section->ASymbols = calloc(bfd_get_section_size(Sect), sizeof(section_t *));
	} else {
		bfd_malloc_and_get_section(Bfd, Sect, (bfd_byte **)&Section->Symbols);
	};
	return Section;
};

typedef struct bfd_info_t {
	const char *FileName;
	struct HXmap *LocalTable;
	asymbol **Symbols;
} bfd_info_t;

typedef struct bfd_section_t bfd_section_t;

struct bfd_section_t {
	section_t Base;
	bfd *Bfd;
	bfd_info_t *BfdInfo;
	asection *Sect;
	uint32_t Flags;
	uint32_t Size;
	uint8_t *Code;
	uint32_t NoOfRelocs;
	relocation_t *Relocs;
	bfd_section_t *Next;
};

static int StopOnUnknown = 0;
static library_section_t *UnknownSymbols = 0;

static void bfd_section_setup(bfd_section_t *Section) {
	bfd *Bfd = Section->Bfd;
	bfd_info_t *BfdInfo = Section->BfdInfo;
	asection *Sect = Section->Sect;
	Section->Size = bfd_get_section_size(Sect);
	bfd_malloc_and_get_section(Bfd, Sect, &Section->Code);
	arelent **Relocs = (arelent **)malloc(bfd_get_reloc_upper_bound(Bfd, Sect));
	Section->NoOfRelocs = bfd_canonicalize_reloc(Bfd, Sect, Relocs, BfdInfo->Symbols);
	Section->Relocs = (relocation_t *)malloc(sizeof(relocation_t) * Section->NoOfRelocs);
	
	if (strcmp(Sect->name, ".ctors") == 0) {
		//printf("Reversing .ctors section...\n");
		// Reverse the order and position of relocations for a .ctors section
		int J = 0;
		for (int I = Section->NoOfRelocs - 1; I >= 0; --I, ++J) {
			relocation_t *Relocation = Section->Relocs + J;
			asymbol *Sym = *(Relocs[I]->sym_ptr_ptr);
			reloc_howto_type *Type = Relocs[I]->howto;
			Relocation->Position = Section->Size - Relocs[I]->address - 4;
			Relocation->Size = bfd_get_reloc_size(Type);
			if (!strcmp(Type->name, "R_386_GOT32")) {
				Relocation->Flags = RELOC_IND;
			} else if (!strcmp(Type->name, "R_386_TLS_IE")) {
				Relocation->Flags = RELOC_IND;
			} else {
				Relocation->Flags = Type->pc_relative ? RELOC_REL : RELOC_ABS;
			};
			uint32_t *Target = (uint32_t *)(Section->Code + Relocs[I]->address);
			if (Type->pc_relative) {
				// Continue to use the original position here
#ifdef WINDOWS
				*(long *)(Section->Code + Relocs[I]->address) -= Relocs[I]->address + 4;// why oh why is this here???
#else
		        *(long *)(Section->Code + Relocs[I]->address) -= Relocs[I]->address;
#endif
			};
			if (!strcmp(Sym->name, "_GLOBAL_OFFSET_TABLE_")) {
				Relocation->Flags = RELOC_GOT;
				section_relocate(Section, Relocation, Target);
			} else if ((Sym->section == bfd_und_section_ptr) || (Sym->section == bfd_com_section_ptr)) {
				symbol_t *Symbol;
				do {
					Symbol = (symbol_t *)HXmap_get(BfdInfo->LocalTable, Sym->name);
					if (Symbol) break;
					Symbol = (symbol_t *)HXmap_get(GlobalTable, Sym->name);
					if (Symbol) break;
					Symbol = (symbol_t *)HXmap_get(WeakTable, Sym->name);
					if (Symbol) break;
#ifdef WINDOWS
		            char *WindowsSizeHint = strrchr(Sym->name, '@');
		            if (WindowsSizeHint) {
		                *WindowsSizeHint = 0;
		                Symbol = (symbol_t *)HXmap_get(BfdInfo->LocalTable, Sym->name);
		                if (Symbol) break;
		                Symbol = (symbol_t *)HXmap_get(GlobalTable, Sym->name);
		                if (Symbol) break;
		                Symbol = (symbol_t *)HXmap_get(WeakTable, Sym->name);
		                if (Symbol) break;
		            }
#endif
		            fprintf(stderr, "%s: unresolved symbol %s.\n", Bfd->filename, Sym->name);
					if (StopOnUnknown) exit(1);
					Symbol = new_symbol(Sym->name, new_import_section(UnknownSymbols, Sym->name, 0), 0);
				} while (0);
				section_t *Section2 = Symbol->Section;
				if (Section2 == 0) {
					fprintf(stderr, "%s: unresolved symbol %s.\n", Bfd->filename, Sym->name);
					if (StopOnUnknown) exit(1);
					Section2 = new_import_section(UnknownSymbols, Sym->name, 0);
				};
				if (!memcmp(Section2->Name, ".tbss", 5)) {
					printf("DEBUG: %s, %d\n", Type->name, Type->type);
				};
				section_relocate(Section2, Relocation, Target);
				if (Type->partial_inplace) *Target += (uint32_t)Symbol->Offset;
			} else if (Sym->section->userdata) {
				section_t *Section2 = (section_t *)Sym->section->userdata;
				section_relocate(Section2, Relocation, Target);
				if (Type->partial_inplace) *Target += (uint32_t)Sym->value;
			} else {
				fprintf(stderr, "%s: unknown relocation type.\n", Bfd->filename);
				exit(1);
			};
		};
		// Need to reverse the order of the pointers in this section
		uint32_t *OldCode = Section->Code + Section->Size;
		uint32_t *NewCode = Section->Code = malloc(Section->Size);
		for (int I = Section->Size / 4; --I >= 0;) {
			--OldCode;
			*NewCode = *OldCode;
			++NewCode;
		};
	} else {
		for (int I = Section->NoOfRelocs - 1; I >= 0; --I) {
			relocation_t *Relocation = Section->Relocs + I;
			asymbol *Sym = *(Relocs[I]->sym_ptr_ptr);
			reloc_howto_type *Type = Relocs[I]->howto;
			Relocation->Position = Relocs[I]->address;
			Relocation->Size = bfd_get_reloc_size(Type);
			if (!strcmp(Type->name, "R_386_GOT32")) {
				Relocation->Flags = RELOC_IND;
			} else if (!strcmp(Type->name, "R_386_TLS_IE")) {
				Relocation->Flags = RELOC_IND;
			} else {
				Relocation->Flags = Type->pc_relative ? RELOC_REL : RELOC_ABS;
			};
			uint32_t *Target = (uint32_t *)(Section->Code + Relocs[I]->address);
			if (Type->pc_relative) {
#ifdef WINDOWS
				*(long *)(Section->Code + Relocs[I]->address) -= Relocs[I]->address + 4;// why oh why is this here???
#else
		        *(long *)(Section->Code + Relocs[I]->address) -= Relocs[I]->address;// + 4 on windows platforms ???;
#endif
			};
			if (!strcmp(Sym->name, "_nxweb_net_thread_data")) {
				printf("DEBUG: %s, %d\n", Type->name, Type->type);
			};
			if (!strcmp(Sym->name, "_GLOBAL_OFFSET_TABLE_")) {
				Relocation->Flags = RELOC_GOT;
				section_relocate(Section, Relocation, Target);
			} else if ((Sym->section == bfd_und_section_ptr) || (Sym->section == bfd_com_section_ptr)) {
				symbol_t *Symbol;
				do {
					Symbol = (symbol_t *)HXmap_get(BfdInfo->LocalTable, Sym->name);
					if (Symbol) break;
					Symbol = (symbol_t *)HXmap_get(GlobalTable, Sym->name);
					if (Symbol) break;
					Symbol = (symbol_t *)HXmap_get(WeakTable, Sym->name);
					if (Symbol) break;
#ifdef WINDOWS
		            char *WindowsSizeHint = strrchr(Sym->name, '@');
		            if (WindowsSizeHint) {
		                *WindowsSizeHint = 0;
		                Symbol = (symbol_t *)HXmap_get(BfdInfo->LocalTable, Sym->name);
		                if (Symbol) break;
		                Symbol = (symbol_t *)HXmap_get(GlobalTable, Sym->name);
		                if (Symbol) break;
		                Symbol = (symbol_t *)HXmap_get(WeakTable, Sym->name);
		                if (Symbol) break;
		            }
#endif
		            fprintf(stderr, "%s: unresolved symbol %s.\n", Bfd->filename, Sym->name);
					if (StopOnUnknown) exit(1);
					Symbol = new_symbol(Sym->name, new_import_section(UnknownSymbols, Sym->name, 0), 0);
				} while (0);
				section_t *Section2 = Symbol->Section;
				if (Section2 == 0) {
					fprintf(stderr, "%s: unresolved symbol %s.\n", Bfd->filename, Sym->name);
					if (StopOnUnknown) exit(1);
					Section2 = new_import_section(UnknownSymbols, Sym->name, 0);
				};
				section_relocate(Section2, Relocation, Target);
				if (Type->partial_inplace) *Target += (uint32_t)Symbol->Offset;
			} else if (Sym->section->userdata) {
				section_t *Section2 = (section_t *)Sym->section->userdata;
				section_relocate(Section2, Relocation, Target);
				if (Type->partial_inplace) *Target += (uint32_t)Sym->value;
			} else {
				fprintf(stderr, "%s: unknown relocation type.\n", Bfd->filename);
				exit(1);
			};
		};
	};
};

static void bfd_section_debug(bfd_section_t *Section, FILE *File) {
	fprintf(File, "%d: text section: %d[%s,%s] %s:%s[%x]", ((section_t *)Section)->Index, Section->Size, (Section->Flags & FLAG_GC) ? "GC" : "NOGC", (Section->Flags & FLAG_DELAY) ? "DELAY" : "NODELAY", Section->BfdInfo->FileName, ((section_t *)Section)->Name, Section->Sect->flags);

#ifdef USE_UDIS
	if (!(Section->Flags & FLAG_GC) && Section->Code) {
		fprintf(File, "\n");
		ud_t UD[1];
		ud_init(UD);
		ud_set_mode(UD, 32);
		ud_set_syntax(UD, UD_SYN_INTEL);
		ud_set_input_buffer(UD, Section->Code, Section->Size);
		while (ud_disassemble(UD)) fprintf(File, "\t%6x: %s\n", (uint32_t)ud_insn_off(UD), ud_insn_asm(UD));
	};
#endif
	if (Section->Flags & FLAG_GC) {
		for (size_t I = 0; I < Section->Size; ++I) {
			if (I % 16 == 0) fprintf(File, "\n\t%4x: ", I);
			fprintf(File, "%c", isprint(Section->Code[I]) ? Section->Code[I] : '.');
		};
	};

	fprintf(File, "\n");
	if (Section->NoOfRelocs > 0) {
		for (unsigned long I = 0; I < Section->NoOfRelocs; ++I) {
			relocation_t *Reloc = &Section->Relocs[I];
			fprintf(File, "\t%4x:\t%d[%d|%d]\n", Reloc->Position, Reloc->Section->Index, Reloc->Size, Reloc->Flags);
		};
	};
};

static uint32_t _bfd_section_size(bfd_section_t *Section) {
	return 1 + 1 + 4 + 4 + Section->Size + (1 + 1 + 4 + 4) * Section->NoOfRelocs;
};

static void bfd_section_write(bfd_section_t *Section, gzFile File) {
	uint32_t Temp;
	if (Section->Flags & FLAG_GC) {
		Temp = SECT_DATA; gzwrite(File, &Temp, 1);
	} else {
		Temp = SECT_TEXT; gzwrite(File, &Temp, 1);
	};
	Temp = Section->Flags; gzwrite(File, &Temp, 1);
	Temp = Section->Size; gzwrite(File, &Temp, 4);
	Temp = Section->NoOfRelocs; gzwrite(File, &Temp, 4);
	gzwrite(File, Section->Code, Section->Size);
	for (unsigned long I = 0; I < Section->NoOfRelocs; ++I) {
		relocation_t *Reloc = &Section->Relocs[I];
		Temp = Reloc->Size; gzwrite(File, &Temp, 1);
		Temp = Reloc->Flags; gzwrite(File, &Temp, 1);
		Temp = Reloc->Position; gzwrite(File, &Temp, 4);
		Temp = Reloc->Section->Index; gzwrite(File, &Temp, 4);
	};
};

static int DelayedLink = 1;

static bfd_section_t *new_bfd_section(asection *Sect, bfd *Bfd, bfd_info_t *BfdInfo) {
	static section_methods Methods = {
		(void (*)(section_t *))bfd_section_setup,
		default_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))bfd_section_debug,
		(uint32_t (*)(section_t *))_bfd_section_size,
		(void (*)(section_t *, gzFile ))bfd_section_write
	};
	bfd_section_t *Section = new(bfd_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	((section_t *)Section)->Name = "???";
	Section->Bfd = Bfd;
	Section->BfdInfo = BfdInfo;
	Section->Sect = Sect;
	Section->Next = 0;
	Section->Flags = 0;
	if (Sect->flags & SEC_DATA) {
		Section->Flags |= FLAG_GC;
	} else if (DelayedLink) {
		Section->Flags |= FLAG_DELAY;
	};
	return Section;
};

typedef struct tdata_section_t tdata_section_t;

struct tdata_section_t {
	bfd_section_t Base;
};

static void tdata_section_debug(bfd_section_t *Section, FILE *File) {
	fprintf(File, "%d: tdata section: %d[%s,%s] %s:%s[%x]", ((section_t *)Section)->Index, Section->Size, (Section->Flags & FLAG_GC) ? "GC" : "NOGC", (Section->Flags & FLAG_DELAY) ? "DELAY" : "NODELAY", Section->BfdInfo->FileName, ((section_t *)Section)->Name, Section->Sect->flags);

#ifdef USE_UDIS
	if (!(Section->Flags & FLAG_GC)) {
		fprintf(File, "\n");
		ud_t UD[1];
		ud_init(UD);
		ud_set_mode(UD, 32);
		ud_set_syntax(UD, UD_SYN_INTEL);
		ud_set_input_buffer(UD, Section->Code, Section->Size);
		while (ud_disassemble(UD)) fprintf(File, "\t%6x: %s\n", (uint32_t)ud_insn_off(UD), ud_insn_asm(UD));
	};
#endif
	if (Section->Flags & FLAG_GC) {
		for (size_t I = 0; I < Section->Size; ++I) {
			if (I % 16 == 0) fprintf(File, "\n\t%4x: ", I);
			fprintf(File, "%c", isprint(Section->Code[I]) ? Section->Code[I] : '.');
		};
	};

	fprintf(File, "\n");
	if (Section->NoOfRelocs > 0) {
		for (unsigned long I = 0; I < Section->NoOfRelocs; ++I) {
			relocation_t *Reloc = &Section->Relocs[I];
			fprintf(File, "\t%4x:\t%d[%d|%d]\n", Reloc->Position, Reloc->Section->Index, Reloc->Size, Reloc->Flags);
		};
	};
};

static void tdata_section_write(bfd_section_t *Section, gzFile File) {
	uint32_t Temp;
	Temp = SECT_TDATA; gzwrite(File, &Temp, 1);
	Temp = Section->Flags; gzwrite(File, &Temp, 1);
	Temp = Section->Size; gzwrite(File, &Temp, 4);
	Temp = Section->NoOfRelocs; gzwrite(File, &Temp, 4);
	gzwrite(File, Section->Code, Section->Size);
	for (unsigned long I = 0; I < Section->NoOfRelocs; ++I) {
		relocation_t *Reloc = &Section->Relocs[I];
		Temp = Reloc->Size; gzwrite(File, &Temp, 1);
		Temp = Reloc->Flags; gzwrite(File, &Temp, 1);
		Temp = Reloc->Position; gzwrite(File, &Temp, 4);
		Temp = Reloc->Section->Index; gzwrite(File, &Temp, 4);
	};
};

static tdata_section_t *new_tdata_section(asection *Sect, bfd *Bfd, bfd_info_t *BfdInfo) {
	static section_methods Methods = {
		(void (*)(section_t *))bfd_section_setup,
		default_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))tdata_section_debug,
		(uint32_t (*)(section_t *))_bfd_section_size,
		(void (*)(section_t *, gzFile ))tdata_section_write
	};
	bfd_section_t *Section = new(bfd_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	((section_t *)Section)->Name = "???";
	Section->Bfd = Bfd;
	Section->BfdInfo = BfdInfo;
	Section->Sect = Sect;
	Section->Next = 0;
	Section->Flags = 0;
	if (Sect->flags & SEC_DATA) {
		Section->Flags |= FLAG_GC;
	} else if (DelayedLink) {
		Section->Flags |= FLAG_DELAY;
	};
	return Section;
};

typedef struct got_entry_t got_entry_t;

typedef struct {
	section_t Base;
} got_section_t;

static void got_section_setup(got_section_t *Section) {
};

static void got_section_relocate(got_section_t *Section, relocation_t *Relocation, uint32_t *Target) {
	section_require(Section);
	Relocation->Section = Section;
};

static void got_section_debug(got_section_t *Section, FILE *File) {
	
};

static uint32_t got_section_size(got_section_t *Section) {
};

static void got_section_write(got_section_t *Section, gzFile File) {
};

static got_section_t *new_got_section() {
	static section_methods Methods = {
		(void (*)(section_t *))default_section_setup,
		got_section_relocate,
		invalid_section_export,
		(void (*)(section_t *, FILE *))got_section_debug,
		(uint32_t (*)(section_t *))got_section_size,
		(void (*)(section_t *, gzFile ))got_section_write
	};
	if (GlobalOffsetTable == 0) {
		got_section_t *Section = new(got_section_t);
		((section_t *)Section)->Index = SEC_UNUSED;
		((section_t *)Section)->Methods = &Methods;
		((section_t *)Section)->Name = "_GLOBAL_OFFSET_TABLE_";
		GlobalOffsetTable = Section;
	};
	return GlobalOffsetTable;
};

typedef struct methods_section_t {
	section_t Base;
	bfd_section_t *Blocks;
	uint32_t Size;
	uint32_t NoOfRelocs;
} methods_section_t;

static void method_section_setup(methods_section_t *Section) {
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		bfd_section_setup(Block);
		Section->Size += Block->Size;
		Section->NoOfRelocs += Block->NoOfRelocs;
	};
};

static void method_section_debug(methods_section_t *Section, FILE *File) {
	fprintf(File, "%d: methods section: %d\n", ((section_t *)Section)->Index, Section->Size);
};

static uint32_t method_section_size(methods_section_t *Section) {
	uint32_t Size = 1 + 1 + 4 + 4 + 4;
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		Size += Block->Size + (1 + 1 + 4 + 4) * Block->NoOfRelocs;
	};
	return Size;
};

static void method_section_write(methods_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_METHODS; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	Temp = Section->Size + 4; gzwrite(File, &Temp, 4);
	Temp = Section->NoOfRelocs; gzwrite(File, &Temp, 4);
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) gzwrite(File, Block->Code, Block->Size);
	Temp = 0xFFFFFFFF; gzwrite(File, &Temp, 4);
	uint32_t Offset = 0;
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		for (unsigned long I = 0; I < Block->NoOfRelocs; ++I) {
			relocation_t *Reloc = &Block->Relocs[I];
			Temp = Reloc->Size; gzwrite(File, &Temp, 1);
			Temp = Reloc->Flags; gzwrite(File, &Temp, 1);
			Temp = Reloc->Position + Offset; gzwrite(File, &Temp, 4);
			Temp = Reloc->Section->Index; gzwrite(File, &Temp, 4);
		};
		Offset += Block->Size;
	};
};

static methods_section_t *new_method_section() {
	static section_methods Methods = {
		(void (*)(section_t *))method_section_setup,
		invalid_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))method_section_debug,
		(uint32_t (*)(section_t *))method_section_size,
		(void (*)(section_t *, gzFile ))method_section_write
	};
	if (MethodsSection == 0) {
		methods_section_t *Section = new(methods_section_t);
		((section_t *)Section)->Methods = &Methods;
		((section_t *)Section)->Index = SEC_UNUSED;
		Section->Blocks = 0;
		Section->Size = 0;
		Section->NoOfRelocs = 0;
		MethodsSection = Section;
	};
	return MethodsSection;
};

typedef struct typedfn_section_t {
	section_t Base;
	bfd_section_t *Blocks;
	uint32_t Size;
	uint32_t NoOfRelocs;
} typedfn_section_t;

static void typedfn_section_setup(typedfn_section_t *Section) {
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		bfd_section_setup(Block);
		Section->Size += Block->Size;
		Section->NoOfRelocs += Block->NoOfRelocs;
	};
};

static void typedfn_section_debug(typedfn_section_t *Section, FILE *File) {
	fprintf(File, "%d: typedfn section: %d\n", ((section_t *)Section)->Index, Section->Size);
};

static uint32_t typedfn_section_size(typedfn_section_t *Section) {
	uint32_t Size = 1 + 1 + 4 + 4 + 4;
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		Size += Block->Size + (1 + 1 + 4 + 4) * Block->NoOfRelocs;
	};
	return Size;
};

static void typedfn_section_write(typedfn_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_TYPEDFN; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	Temp = Section->Size + 4; gzwrite(File, &Temp, 4);
	Temp = Section->NoOfRelocs; gzwrite(File, &Temp, 4);
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) gzwrite(File, Block->Code, Block->Size);
	Temp = 0xFFFFFFFF; gzwrite(File, &Temp, 4);
	uint32_t Offset = 0;
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		for (unsigned long I = 0; I < Block->NoOfRelocs; ++I) {
			relocation_t *Reloc = &Block->Relocs[I];
			Temp = Reloc->Size; gzwrite(File, &Temp, 1);
			Temp = Reloc->Flags; gzwrite(File, &Temp, 1);
			Temp = Reloc->Position + Offset; gzwrite(File, &Temp, 4);
			Temp = Reloc->Section->Index; gzwrite(File, &Temp, 4);
		};
		Offset += Block->Size;
	};
};

static typedfn_section_t *new_typedfn_section() {
	static section_methods Methods = {
		(void (*)(section_t *))typedfn_section_setup,
		invalid_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))typedfn_section_debug,
		(uint32_t (*)(section_t *))typedfn_section_size,
		(void (*)(section_t *, gzFile ))typedfn_section_write
	};
	if (TypedFnSection == 0) {
		typedfn_section_t *Section = new(typedfn_section_t);
		((section_t *)Section)->Methods = &Methods;
		((section_t *)Section)->Index = SEC_UNUSED;
		Section->Blocks = 0;
		Section->Size = 0;
		Section->NoOfRelocs = 0;
		TypedFnSection = Section;
	};
	return TypedFnSection;
};

typedef struct initial_section_t {
	section_t Base;
	bfd_section_t *Blocks;
	uint32_t Size;
	uint32_t NoOfRelocs;
} initial_section_t;

static void initial_section_setup(initial_section_t *Section) {
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		bfd_section_setup(Block);
		Section->Size += Block->Size;
		Section->NoOfRelocs += Block->NoOfRelocs;
	};
};

static void initial_section_debug(initial_section_t *Section, FILE *File) {
	fprintf(File, "%d: initial section: %d\n", ((section_t *)Section)->Index, Section->Size);
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		for (unsigned long I = 0; I < Block->NoOfRelocs; ++I) {
			relocation_t *Reloc = &Block->Relocs[I];
			fprintf(File, "\t%4x:\t%d[%d|%d]\n", Reloc->Position, Reloc->Section->Index, Reloc->Size, Reloc->Flags);
		};
	};
};

static uint32_t initial_section_size(initial_section_t *Section) {
	uint32_t Size = 1 + 1 + 4 + 4 + 4;
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		Size += Block->Size + (1 + 1 + 4 + 4) * Block->NoOfRelocs;
	};
	return Size;
};

static void initial_section_write(initial_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_INITIAL; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	Temp = Section->Size + 4; gzwrite(File, &Temp, 4);
	Temp = Section->NoOfRelocs; gzwrite(File, &Temp, 4);
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) gzwrite(File, Block->Code, Block->Size);
	Temp = 0xFFFFFFFF; gzwrite(File, &Temp, 4);
	uint32_t Offset = 0;
	for (bfd_section_t *Block = Section->Blocks; Block; Block = Block->Next) {
		for (unsigned long I = 0; I < Block->NoOfRelocs; ++I) {
			relocation_t *Reloc = &Block->Relocs[I];
			Temp = Reloc->Size; gzwrite(File, &Temp, 1);
			Temp = Reloc->Flags; gzwrite(File, &Temp, 1);
			Temp = Reloc->Position + Offset; gzwrite(File, &Temp, 4);
			Temp = Reloc->Section->Index; gzwrite(File, &Temp, 4);
		};
		Offset += Block->Size;
	};
};

static initial_section_t *new_initial_section() {
	static section_methods Initials = {
		(void (*)(section_t *))initial_section_setup,
		invalid_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))initial_section_debug,
		(uint32_t (*)(section_t *))initial_section_size,
		(void (*)(section_t *, gzFile ))initial_section_write
	};
	if (InitialSection == 0) {
		initial_section_t *Section = new(initial_section_t);
		((section_t *)Section)->Methods = &Initials;
		((section_t *)Section)->Index = SEC_UNUSED;
		Section->Blocks = 0;
		Section->Size = 0;
		Section->NoOfRelocs = 0;
		InitialSection = Section;
	};
	return InitialSection;
};

typedef struct constant_section_t {
	section_t Base;
	uint32_t Flags;
	section_t *Init;
	uint32_t Offset;
} constant_section_t;

static void constant_section_debug(constant_section_t *Section, FILE *File) {
	fprintf(File, "%d: constant section: %d[%d]\n", ((section_t *)Section)->Index, Section->Init->Index, Section->Offset);
};

static uint32_t constant_section_size(constant_section_t *Section) {
	return 1 + 1 + 4 + 4;
};

static void constant_section_write(constant_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_CONSTANT; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	Temp = ((section_t *)Section->Init)->Index; gzwrite(File, &Temp, 4);
	Temp = Section->Offset; gzwrite(File, &Temp, 4);
};

static void constant_section_setup(constant_section_t *Section) {
	section_require(Section->Init);
};

static constant_section_t *new_constant_section(void) {
	static section_methods Methods = {
		(void (*)(section_t *))constant_section_setup,
		default_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))constant_section_debug,
		(uint32_t (*)(section_t *))constant_section_size,
		(void (*)(section_t *, gzFile ))constant_section_write
	};
	constant_section_t *Section = new(constant_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	Section->Flags = 0;
	return Section;
};

typedef struct constants_section_t {
	section_t Base;
	asection *Sect;
	bfd *Bfd;
	bfd_info_t *BfdInfo;
	uint32_t NoOfConstants;
	constant_section_t **Constants;
} constants_section_t;

static void constants_section_relocate(constants_section_t *Section, relocation_t *Relocation, uint32_t *Target) {
	if (Section->Sect) {
		asection *Sect = Section->Sect;
		bfd *Bfd = Section->Bfd;
		bfd_info_t *BfdInfo = Section->BfdInfo;
		int Count = bfd_get_section_size(Sect) / 4;
		Section->Constants = malloc(sizeof(constant_section_t *) * Count);
		for (int I = 0; I < Count; I++) Section->Constants[I] = new_constant_section();
		uint8_t *Code;
		bfd_malloc_and_get_section(Bfd, Sect, &Code);
		arelent **Relocs = (arelent **)malloc(bfd_get_reloc_upper_bound(Bfd, Sect));
		for (int I = bfd_canonicalize_reloc(Bfd, Sect, Relocs, BfdInfo->Symbols) - 1; I >= 0; --I) {
			asymbol *Sym = *(Relocs[I]->sym_ptr_ptr);
			reloc_howto_type *Type = Relocs[I]->howto;
			constant_section_t *Constant = Section->Constants[Relocs[I]->address / 4];
			uint32_t *Target = (uint32_t *)(Code + Relocs[I]->address);
			if (Type->partial_inplace) *Target += (uint32_t)Sym->value;
			Constant->Init = (section_t *)Sym->section->userdata;
			Constant->Offset = *Target;
		};
		Section->Sect = 0;
	};
	constant_section_t *Constant = Section->Constants[*Target / 4];
	*Target = 0;
	section_require(Constant);
	Relocation->Section = Constant;
};

static void constants_section_export(constants_section_t *Section, uint32_t Offset, export_t *Export) {
	if (Section->Sect) {
		asection *Sect = Section->Sect;
		bfd *Bfd = Section->Bfd;
		bfd_info_t *BfdInfo = Section->BfdInfo;
		int Count = bfd_get_section_size(Sect) / 4;
		Section->Constants = malloc(sizeof(constant_section_t *) * Count);
		for (int I = 0; I < Count; I++) Section->Constants[I] = new_constant_section();
		uint8_t *Code;
		bfd_malloc_and_get_section(Bfd, Sect, &Code);
		arelent **Relocs = (arelent **)malloc(bfd_get_reloc_upper_bound(Bfd, Sect));
		for (int I = bfd_canonicalize_reloc(Bfd, Sect, Relocs, BfdInfo->Symbols) - 1; I >= 0; --I) {
			asymbol *Sym = *(Relocs[I]->sym_ptr_ptr);
			reloc_howto_type *Type = Relocs[I]->howto;
			constant_section_t *Constant = Section->Constants[Relocs[I]->address / 4];
			uint32_t *Target = (uint32_t *)(Code + Relocs[I]->address);
			if (Type->partial_inplace) *Target += (uint32_t)Sym->value;
			Constant->Init = (section_t *)Sym->section->userdata;
			Constant->Offset = *Target;
		};
		Section->Sect = 0;
	};
	constant_section_t *Constant = Section->Constants[Offset / 4];
	Export->Section = Constant;
	Export->Offset = 0;
};

static constants_section_t *new_constants_section(asection *Sect, bfd *Bfd, bfd_info_t *BfdInfo) {
	static section_methods Methods = {
		invalid_section_setup,
		(void (*)(section_t *, relocation_t *, uint32_t *))constants_section_relocate,
		(void (*)(section_t *, uint32_t, export_t *))constants_section_export,
		0,
		0,
		0
	};
	constants_section_t *Section = new(constants_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	Section->Sect = Sect;
	Section->Bfd = Bfd;
	Section->BfdInfo = BfdInfo;
	return Section;
};

static void add_bfd_section(bfd *Bfd, asection *Sect, bfd_info_t *BfdInfo) {
	if (strcmp(Sect->name, ".symbols") == 0) {
		if (Sect->size) Sect->userdata = new_symbols_section(Sect, Bfd, 0);
	} else if (strcmp(Sect->name, ".asymbol") == 0) {
		Sect->userdata = new_symbols_section(Sect, Bfd, 1);
	} else if (strcmp(Sect->name, ".methods") == 0) {
		if (Sect->size) {
			methods_section_t *Section = new_method_section();
			bfd_section_t *Block = new_bfd_section(Sect, Bfd, BfdInfo);
			Block->Next = Section->Blocks;
			Section->Blocks = Block;
			Sect->userdata = Block;
		};
	} else if (strcmp(Sect->name, ".typedfn") == 0) {
		if (Sect->size) {
			typedfn_section_t *Section = new_typedfn_section();
			bfd_section_t *Block = new_bfd_section(Sect, Bfd, BfdInfo);
			Block->Next = Section->Blocks;
			Section->Blocks = Block;
			Sect->userdata = Block;
		};
	} else if (strcmp(Sect->name, ".initial") == 0) {
		if (Sect->size) {
			initial_section_t *Section = new_initial_section();
			bfd_section_t *Block = new_bfd_section(Sect, Bfd, BfdInfo);
			Block->Next = Section->Blocks;
			Section->Blocks = Block;
			Sect->userdata = Block;
		};
	} else if (strcmp(Sect->name, ".ctors") == 0) {
		//printf("New .ctors handling code...\n");
		if (Sect->size) {
			initial_section_t *Section = new_initial_section();
			bfd_section_t *Block = new_bfd_section(Sect, Bfd, BfdInfo);
			Block->Next = Section->Blocks;
			Section->Blocks = Block;
			Sect->userdata = Block;
		};
	} else if (strcmp(Sect->name, ".dtors") == 0) {
		//printf("New .dtors handling code...\n");
	} else if (strcmp(Sect->name, ".constants") == 0) {
		if (Sect->size) Sect->userdata = new_constants_section(Sect, Bfd, BfdInfo);
	} else if (Sect->flags & SEC_LOAD) {
		if (Sect->flags & SEC_THREAD_LOCAL) {
			//printf("%s: .tdata support is experimental!\n", Bfd->filename);
			Sect->userdata = new_tdata_section(Sect, Bfd, BfdInfo);
		} else {
			Sect->userdata = new_bfd_section(Sect, Bfd, BfdInfo);
		};
	} else if (Sect->flags & SEC_ALLOC) {
		if (Sect->flags & SEC_THREAD_LOCAL) {
			//printf("%s: .tbss support is experimental!\n", Bfd->filename);
			Sect->userdata = new_tbss_section(bfd_get_section_size(Sect));
		} else {
			Sect->userdata = new_bss_section(bfd_get_section_size(Sect));
		};
	} else if (strcmp(Sect->name, ".group") == 0) {
	} else if (strcmp(Sect->name, ".comment") == 0) {
	} else if (strcmp(Sect->name, ".note.GNU-stack") == 0) {
	} else if (strcmp(Sect->name, ".note.GNU-split-stack") == 0) {
	} else if (strcmp(Sect->name, ".note.GNU-no-split-stack") == 0) {
	} else if (strncmp(Sect->name, ".gnu.warning", 12) == 0) {
	} else if (strncmp(Sect->name, ".gnu.glibc-stub", 15) == 0) {
	} else {
		fprintf(stderr, "%s: unknown bfd section type: %s,%x\n", Bfd->filename, Sect->name, Sect->flags);
	};
};

typedef void (*bfd_map)(bfd *, asection *, void *);

static void add_bfd(bfd *Bfd, int AutoExport) {
	if (bfd_check_format(Bfd, bfd_object)) {
		bfd_info_t *BfdInfo = new(bfd_info_t);
		BfdInfo->FileName = Bfd->filename;
		BfdInfo->LocalTable = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
		memset(BfdInfo->LocalTable, 0, sizeof(BfdInfo->LocalTable));
		BfdInfo->Symbols = (asymbol **)malloc(bfd_get_symtab_upper_bound(Bfd));
		int NoOfSymbols = bfd_canonicalize_symtab(Bfd, BfdInfo->Symbols);
		bfd_map_over_sections(Bfd, (bfd_map)add_bfd_section, BfdInfo);
		for (int I = NoOfSymbols - 1; I >= 0; --I) {
			asymbol *Sym = BfdInfo->Symbols[I];
			if (Sym->flags & BSF_GLOBAL) {
				const char *Name = strdup(Sym->name);
				symbol_t *Symbol = new_symbol(Name, (section_t *)Sym->section->userdata, Sym->value);
				if (Sym->section->userdata && Sym->value == 0) {
					((section_t *)Sym->section->userdata)->Name = Name;
				};
				HXmap_add(BfdInfo->LocalTable, Name, Symbol);
				HXmap_add(GlobalTable, Name, Symbol);
#ifdef WINDOWS
				if (AutoExport) new_export(Name, Name + 1, 0);
#else
				if (AutoExport) new_export(Name, Name, 0);
#endif
			} else if (Sym->section == bfd_com_section_ptr) {
				symbol_t *Symbol = (symbol_t *)HXmap_get(GlobalTable, Sym->name);
				bss_section_t *Section;
				if (!Symbol) {
					const char *Name = strdup(Sym->name);
					Section = new_bss_section(0);
					Symbol = new_symbol(Name, (section_t *)Section, 0);
					HXmap_add(GlobalTable, Name, Symbol);
					HXmap_add(BfdInfo->LocalTable, Name, Symbol);
				} else {
					Section = (bss_section_t *)Symbol->Section;
				};
				if (Sym->value > Section->Size) Section->Size = Sym->value;
			} else if (Sym->flags & BSF_LOCAL) {
				const char *Name = strdup(Sym->name);
				symbol_t *Symbol = new_symbol(Name, (section_t *)Sym->section->userdata, Sym->value);
				if (Sym->section->userdata && Sym->value == 0) ((section_t *)Sym->section->userdata)->Name = Name;
				HXmap_add(BfdInfo->LocalTable, Name, Symbol);
			} else if (Sym->flags & BSF_WEAK || Sym->flags & BSF_GNU_UNIQUE) {
				const char *Name = strdup(Sym->name);
				symbol_t *Symbol = new_symbol(Name, (section_t *)Sym->section->userdata, Sym->value);
				if (Sym->section->userdata && Sym->value == 0) ((section_t *)Sym->section->userdata)->Name = Name;
				HXmap_add(WeakTable, Name, Symbol);
/*
#ifdef WINDOWS
				if (AutoExport) new_export(Name, Name + 1, 0);
#else
				if (AutoExport) new_export(Name, Name, 0);
#endif
*/
			} else if (Sym->section == bfd_und_section_ptr) {
			} else if (Sym->flags & BSF_DEBUGGING) {
			    // This may be supported later
			} else {
				fprintf(stderr, "%s: unknown symbol type: %8x.\n", Bfd->filename, Sym->flags);
				//exit(1);
			};
		};
	} else if (bfd_check_format(Bfd, bfd_archive)) {
		bfd *Bfd2 = 0;
		while ((Bfd2 = bfd_openr_next_archived_file(Bfd, Bfd2))) add_bfd(Bfd2, AutoExport);
	};
};

static void add_object_file(const char *FileName, int AutoExport) {
	if (DependencyMode) {
		HXmap_add(Dependencies, FileName, 0);
		return;
	};
	bfd *Bfd = bfd_openr(FileName, 0);
	if (Bfd == 0) {
		fprintf(stderr, "%s: error reading file.\n", FileName);
		return;
	};
	add_bfd(Bfd, AutoExport);
};

static void add_dependency_file(const char *FileName, int AutoExport) {
	HXmap_add(Dependencies, FileName, 0);
};

typedef struct module_section_t {
	section_t Base;
	export_t *Exports;
	int NoOfExports;
	const char *Name;
} module_section_t;

static void module_section_setup(module_section_t *Section) {
	for (export_t *Export = Section->Exports; Export; Export = Export->Next) {
		if (Export->Section == 0) {
			symbol_t *Symbol = (symbol_t *)HXmap_get(GlobalTable, Export->Internal);
			if (Symbol == 0) Symbol = (symbol_t *)HXmap_get(WeakTable, Export->Internal);
			if (Symbol) {
				section_export(Symbol->Section, Symbol->Offset, Export);
			} else {
				fprintf(stderr, "exported symbol not found: %s.\n", Export->Internal);
				exit(1);
			};
		};
		section_require(Export->Section);
	};
};

static void module_section_debug(module_section_t *Section, FILE *File) {
	fprintf(File, "%d: module section: %s\n", ((section_t *)Section)->Index, Section->Name);
	for (export_t *Export = Section->Exports; Export; Export = Export->Next) {
		fprintf(File, "\texport: %s -> %d[%d]\n", Export->External, Export->Section->Index, Export->Offset);
	};
};

static uint32_t module_section_size(module_section_t *Section) {
	uint32_t Size = 1 + 1 + 4;
	for (export_t *Export = Section->Exports; Export; Export = Export->Next) {
		Size += 1 + 4 + 4 + 4 + strlen(Export->External) + 1;
	};
	return Size;
};

static void module_section_write(module_section_t *Section, gzFile File) {
	uint32_t Temp = SECT_MODULE; gzwrite(File, &Temp, 1);
	Temp = 0; gzwrite(File, &Temp, 1);
	Temp = Section->NoOfExports; gzwrite(File, &Temp, 4);
	for (export_t *Export = Section->Exports; Export; Export = Export->Next) {
		Temp = Export->Flags; gzwrite(File, &Temp, 1);
		Temp = Export->Section->Index; gzwrite(File, &Temp, 4);
		Temp = Export->Offset; gzwrite(File, &Temp, 4);
		Temp = strlen(Export->External) + 1; gzwrite(File, &Temp, 4);
		gzwrite(File, Export->External, Temp);
	};
};

static module_section_t *new_module_section(const char *Name) {
	static section_methods Methods = {
		(void (*)(section_t *))module_section_setup,
		invalid_section_relocate,
		default_section_export,
		(void (*)(section_t *, FILE *))module_section_debug,
		(uint32_t (*)(section_t *))module_section_size,
		(void (*)(section_t *, gzFile ))module_section_write
	};
	module_section_t *Section = new(module_section_t);
	((section_t *)Section)->Index = SEC_UNUSED;
	((section_t *)Section)->Methods = &Methods;
	Section->Exports = 0;
	Section->NoOfExports = 0;
	Section->Name = Name;
	return Section;
};

static const char EXPORT_CONSTANT[] = "CONSTANT";
static const char EXPORT_VARIABLE[] = "VARIABLE";

typedef struct export_type {
    const char *Name;
    int Flags;
} export_type;

static export_type ExportTypes[] = {
    {EXPORT_CONSTANT, 0},
    {EXPORT_VARIABLE, 1},
    {0, 0}
};

static const char *Platforms[] = {
    "WINDOWS",
    "LINUX",
    "MACOSX",
    0
};

typedef struct library_file_t {
    const char *FileName;
    const char *Prefix;
    library_section_t *Section;
} library_file_t;

static void add_define(const char *Name, const char *Value) {
	define_t *Define = new(define_t);
	Define->Name = Name;
	Define->Value = Value;
	Define->Next = Defines;
	Defines = Define;
};

static int script_file_module(lua_State *State) {
    int NoOfArgs = lua_gettop(State);
    lua_getglobal(State, "Library");
    library_file_t *Library = lua_touserdata(State, -1);
    lua_pop(State, 1);

    int PathSize = 0;
    for (int I = 1; I <= NoOfArgs; ++I) PathSize += strlen(lua_tostring(State, I)) + 1;
    char *Path = (char *)malloc(PathSize), *PathPtr = Path;
#ifdef WINDOWS
    char *Name = (char *)malloc(PathSize + 1), *NamePtr = Name + 1;
    Name[0] = '_';
#else
	char *Name = (char *)malloc(PathSize), *NamePtr = Name;
#endif
    for (int I = 1; I <= NoOfArgs; ++I) {
        char *Temp = lua_tostring(State, I);
		PathPtr = stpcpy(PathPtr, Temp);
		*(PathPtr++) = '/';
		NamePtr = stpcpy(NamePtr, Temp);
		*(NamePtr++) = '$';
    };
    PathPtr[-1] = 0;
	NamePtr[-1] = 0;

	Library->Prefix = "";

	library_section_t *LibrarySection = (library_section_t *)HXmap_get(LibraryTable, Path);
	if (LibrarySection == 0) {
        LibrarySection = new_library_section(Path);
        HXmap_add(GlobalTable, Name, new_symbol(Name, (section_t *)LibrarySection, 0));
	};

    Library->Section = LibrarySection;
    return 0;
};

static int script_file_prefix(lua_State *State) {
    lua_getglobal(State, "Library");
    library_file_t *Library = lua_touserdata(State, -1);
    lua_pop(State, 1);
    Library->Prefix = strdup(lua_tostring(State, 1));
    return 0;
};

static int script_file_import(lua_State *State) {
    int NoOfArgs = lua_gettop(State);
    lua_getglobal(State, "Library");
    library_file_t *Library = lua_touserdata(State, -1);
    lua_pop(State, 1);
    int Flags = 0;
    const char *Internal = 0;
    const char *External = 0;
    for (int I = 1; I <= NoOfArgs; ++I) {
        if (lua_isstring(State, I)) {
            if (Internal == 0) {
                Internal = strdup(lua_tostring(State, I));
            } else {
                External = strdup(lua_tostring(State, I));
            };
        } else if (lua_isuserdata(State, I)) {
            void *Name = lua_touserdata(State, I);
            for (export_type *E = ExportTypes; E->Name; ++E) {
                if (E->Name == Name) {
                    Flags = E->Flags;
                    break;
                };
            };
        };
    };
    if (External == 0) asprintf(&External, "%s%s", Library->Prefix, Internal);

    //printf("Adding import: %s -> %s\n", Internal, External);

    import_section_t *ImportSection = (import_section_t *)HXmap_get(Library->Section->Imports, Internal);
    if (ImportSection == 0) {
        ImportSection = new_import_section(Library->Section, Internal, Flags);
    };
    symbol_t *Symbol = new_symbol(External, (section_t *)ImportSection, 0);
    HXmap_add(GlobalTable, External, Symbol);
    return 0;
};

static void add_file(const char *FileName, int AutoExport);

static int script_file_include(lua_State *State) {
	int NoOfArgs = lua_gettop(State);
	int AutoExport = NoOfArgs > 1 ? lua_toboolean(State, 2) : 0;
	add_file(lua_tostring(State, 1), AutoExport);
	return 0;
};

static int script_file_export(lua_State *State) {
    const char *Internal = 0;
    const char *External = 0;
    int Flags = 0;
    int NoOfArgs = lua_gettop(State);
    for (int I = 1; I <= NoOfArgs; ++I) {
        if (lua_isstring(State, I)) {
            if (Internal == 0) {
                External = Internal = strdup(lua_tostring(State, I));
            } else {
                External = strdup(lua_tostring(State, I));
            };
        } else if (lua_isuserdata(State, I)) {
            void *Name = lua_touserdata(State, I);
            for (export_type *E = ExportTypes; E->Name; ++E) {
                if (E->Name == Name) {
                    Flags = E->Flags;
                    break;
                };
            };
        };
    };
    //printf("Adding export %s -> %s[%d]\n", Internal, External, Flags);
    new_export(Internal, External, Flags);
    return 0;
};

static int script_file_require(lua_State *State) {
	char *Library = lua_tostring(State, 1);
	if (Library[0] == '.') {
		new_require(LIBRARY_REL, strdup(Library + 1));
	} else {
		new_require(LIBRARY_ABS, strdup(Library));
	};
	return 0;
};

static int script_file_submodule(lua_State *State) {
    const char *Internal = 0;
    const char *External = 0;
    int NoOfArgs = lua_gettop(State);
    for (int I = 1; I <= NoOfArgs; ++I) {
        if (lua_isstring(State, I)) {
            if (Internal == 0) {
                External = Internal = strdup(lua_tostring(State, I));
            } else {
                External = strdup(lua_tostring(State, I));
            };
        } else {
        	External = 0;
        };
    };
    //printf("Adding submodule %s -> %s\n", Internal, External);
    if (External) new_export(Internal, External, 0);
    HXmap_add(GlobalTable, Internal, new_symbol(Internal, (section_t *)new_module_section(External), 0));
    return 0;
};

static int script_file_subexport(lua_State *State) {
	const char *Internal = 0;
    const char *External = 0;
    int Flags = 0;
    int NoOfArgs = lua_gettop(State);
    const char *SubName = lua_tostring(State, 1);
    module_section_t *SubModule = ((symbol_t *)HXmap_get(GlobalTable, SubName))->Section;
    if (SubModule == 0) {
    	fprintf(stderr, "submodule not found: %s.\n", SubName);
    	exit(1);
    };
    for (int I = 2; I <= NoOfArgs; ++I) {
        if (lua_isstring(State, I)) {
            if (Internal == 0) {
                External = Internal = strdup(lua_tostring(State, I));
            } else {
                External = strdup(lua_tostring(State, I));
            };
        } else if (lua_isuserdata(State, I)) {
            void *Name = lua_touserdata(State, I);
            for (export_type *E = ExportTypes; E->Name; ++E) {
                if (E->Name == Name) {
                    Flags = E->Flags;
                    break;
                };
            };
        };
    };
    //printf("Adding subexport %s -> %s.%s[%d]\n", Internal, SubName, External, Flags);
    export_t *Export = new(export_t);
	Export->Internal = Internal;
	Export->External = External;
	Export->Flags = Flags;
	Export->Section = 0;
	Export->Next = 0;
	++SubModule->NoOfExports;
	Export->Next = SubModule->Exports;
	SubModule->Exports = Export;
    return 0;
};

static void add_script_file(const char *FileName, int AutoExport) {
	if (DependencyMode) HXmap_add(Dependencies, FileName, 0);
    lua_State *State = luaL_newstate();
    lua_register(State, "module", script_file_module);
    lua_register(State, "prefix", script_file_prefix);
    lua_register(State, "export", script_file_export);
    lua_register(State, "require", script_file_require);
    lua_register(State, "include", script_file_include);
    lua_register(State, "import", script_file_import);
    lua_register(State, "submodule", script_file_submodule);
    lua_register(State, "subexport", script_file_subexport);
    lua_pushstring(State, Platform);
    lua_setglobal(State, "Platform");
    for (const char **P = Platforms; *P; ++P) {
        lua_pushboolean(State, strcmp(Platform, *P) == 0);
        lua_setglobal(State, *P);
    };
    for (export_type *E = ExportTypes; E->Name; ++E) {
        lua_pushlightuserdata(State, E->Name);
        lua_setglobal(State, E->Name);
    };
    for (define_t *Define = Defines; Define; Define = Define->Next) {
    	lua_pushstring(State, Define->Value);
    	lua_setglobal(State, Define->Name);
    };
    library_file_t *Library = new(library_file_t);
    Library->FileName = FileName;
    Library->Prefix = "";
    Library->Section = 0;
    lua_pushlightuserdata(State, Library);
    lua_setglobal(State, "Library");
    if (luaL_dofile(State, FileName)) {
        fprintf(stderr, "\e[31m%s: error: %s\e[0m", FileName, lua_tostring(State, -1));
    };
};

static void add_library_file(const char *FileName, int AutoExport) {
	if (DependencyMode) HXmap_add(Dependencies, FileName, 0);
    lua_State *State = luaL_newstate();
    lua_register(State, "module", script_file_module);
    lua_register(State, "prefix", script_file_prefix);
    lua_register(State, "export", script_file_import);
    lua_register(State, "include", script_file_include);
    lua_pushstring(State, Platform);
    lua_setglobal(State, "Platform");
    for (const char **P = Platforms; *P; ++P) {
        lua_pushboolean(State, strcmp(Platform, *P) == 0);
        lua_setglobal(State, *P);
    };
    for (export_type *E = ExportTypes; E->Name; ++E) {
        lua_pushlightuserdata(State, E->Name);
        lua_setglobal(State, E->Name);
    };
    for (define_t *Define = Defines; Define; Define = Define->Next) {
    	lua_pushstring(State, Define->Value);
    	lua_setglobal(State, Define->Name);
    };

    library_file_t *Library = new(library_file_t);
    Library->FileName = FileName;
    Library->Prefix = "";
    Library->Section = 0;
    lua_pushlightuserdata(State, Library);
    lua_setglobal(State, "Library");

    if (luaL_dofile(State, FileName)) {
        fprintf(stderr, "\e[31m%s: error: %s\e[0m", FileName, lua_tostring(State, -1));
    };
};

static int definition_file_symbol(lua_State *State) {
    const char *Internal = strdup(lua_tostring(State, 1));
    const char *External = strdup(lua_tostring(State, 2));
 
    //printf("Adding symbol: %s -> %s\n", Internal, External);

	new_export(Internal, External, 0);
};

static void add_definition_file(const char *FileName, int AutoExport) {
	if (DependencyMode) HXmap_add(Dependencies, FileName, 0);
    lua_State *State = luaL_newstate();
    lua_register(State, "export", script_file_export);
	lua_register(State, "symbol", definition_file_symbol);
    lua_register(State, "library", script_file_include);
    lua_register(State, "require", script_file_require);
    lua_pushstring(State, Platform);
    lua_setglobal(State, "Platform");
    for (const char **P = Platforms; *P; ++P) {
        lua_pushboolean(State, strcmp(Platform, *P) == 0);
        lua_setglobal(State, *P);
    };
    for (export_type *E = ExportTypes; E->Name; ++E) {
        lua_pushlightuserdata(State, E->Name);
        lua_setglobal(State, E->Name);
    };
    for (define_t *Define = Defines; Define; Define = Define->Next) {
    	lua_pushstring(State, Define->Value);
    	lua_setglobal(State, Define->Name);
    };
    if (luaL_dofile(State, FileName)) {
		fprintf(stderr, "\e[31m%s: error: %s\e[0m", FileName, lua_tostring(State, -1));
    };
};

static void add_so(bfd *Bfd, library_section_t *LibrarySection) {
	if (bfd_check_format(Bfd, bfd_object)) {
		bfd_info_t *BfdInfo = new(bfd_info_t);
		BfdInfo->LocalTable = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
#ifdef LINUX
		BfdInfo->Symbols = (asymbol **)malloc(bfd_get_dynamic_symtab_upper_bound(Bfd));
		int NoOfSymbols = bfd_canonicalize_dynamic_symtab(Bfd, BfdInfo->Symbols);
#endif
#ifdef WINDOWS
		BfdInfo->Symbols = (asymbol **)malloc(bfd_get_symtab_upper_bound(Bfd));
		int NoOfSymbols = bfd_canonicalize_symtab(Bfd, BfdInfo->Symbols);
#endif
		if (NoOfSymbols == -1) {
			printf("%s\n", bfd_errmsg(bfd_get_error()));
		};
		//printf("NoOfSymbols = %d\n", NoOfSymbols);
		for (int I = NoOfSymbols - 1; I >= 0; --I) {
			asymbol *Sym = BfdInfo->Symbols[I];
			//printf("Sym->name = %s\n", Sym->name);
#ifdef WINDOWS
			if (Sym->flags & BSF_GLOBAL && Sym->flags & BSF_FUNCTION) {
#else
			if (Sym->flags & BSF_GLOBAL) {
#endif
				const char *Name = strdup(Sym->name);
				//printf("\tAdding import %s\n", Name);
				import_section_t *ImportSection = (import_section_t *)HXmap_get(LibrarySection->Imports, Name);
				if (ImportSection == 0) {
					ImportSection = new_import_section(LibrarySection, Name, 0);
					symbol_t *Symbol = new_symbol(Name, (section_t *)ImportSection, 0);
					HXmap_add(GlobalTable, Name, Symbol);
#ifdef WINDOWS
					//char *_Name = malloc(strlen(Name) + 2);
					//_Name[0] = '_';
					//strcpy(_Name + 1, Name);
					//HXmap_add(GlobalTable, _Name, Symbol);
#endif
				};
			};
		};
	} else if (bfd_check_format(Bfd, bfd_archive)) {
		bfd *Bfd2 = 0;
		while ((Bfd2 = bfd_openr_next_archived_file(Bfd, Bfd2))) add_so(Bfd2, LibrarySection);
	};
};

static void add_shared_lib(const char *FileName, int AutoExport) {
	if (DependencyMode) {
		HXmap_add(Dependencies, FileName, 0);
		return;
	};
	char *Module = strdup(basename(FileName));
#ifdef LINUX
	Module[strlen(Module) - 3] = 0;
#endif
#ifdef WINDOWS
	Module[strlen(Module) - 4] = 0;
#endif
	//printf("Adding shared library %s -> %s\n", FileName, Module);
	
	library_section_t *LibrarySection = (library_section_t *)HXmap_get(LibraryTable, Module);
	if (LibrarySection == 0) {
		LibrarySection = new_library_section(Module);
		bfd *Bfd = bfd_openr(FileName, 0);
		if (Bfd == 0) {
			printf("%s: error reading file.\n", FileName);
			return;
		};
		add_so(Bfd, LibrarySection);
	};
};

static struct HXMap *SupportedFiles;

typedef void (*file_adder)(const char *, int);

typedef struct search_path_t {
	const char *Path;
	struct search_path_t *Next;
} search_path_t;

static search_path_t SearchPath = {"", 0};
static search_path_t *SearchPathTail = &SearchPath;

static void add_path(const char *Path) {
	search_path_t *Node = new(search_path_t);
	Node->Path = strdup(Path);
	Node->Next = 0;
	SearchPathTail->Next = Node;
	SearchPathTail = Node;
};

static void add_file(const char *FileName, int AutoExport) {
	const char *Extension = strrchr(FileName, '.');
	if (Extension == 0) {
		printf("%s: filename must include extension.\n", FileName);
		return;
	};
	file_adder _add = (file_adder)HXmap_get(SupportedFiles, Extension);
	if (_add == 0) {
		printf("%s: unable to determine file type from extension.\n", FileName);
		return;
	};
	struct stat Stat;
	char FullFileName[256];
	for (search_path_t *Node = &SearchPath; Node; Node = Node->Next) {
		sprintf(FullFileName, "%s%s", Node->Path, FileName);
		if (stat(FullFileName, &Stat) == 0) {
			_add(strdup(FullFileName), AutoExport);
			return;
		};
		sprintf(FullFileName, "%s/%s", Node->Path, FileName);
		if (stat(FullFileName, &Stat) == 0) {
			_add(strdup(FullFileName), AutoExport);
			return;
		};
	};
	if (DependencyMode) return;
	fprintf(stderr, "%s: file not found.\n", FileName);
	exit(1);
};

static void find_shared_lib(const char *Name) {
	struct stat Stat;
	char FullFileName[256];
	for (search_path_t *Node = &SearchPath; Node; Node = Node->Next) {
#ifdef LINUX
		sprintf(FullFileName, "%slib%s.so", Node->Path, Name);
		if (stat(FullFileName, &Stat) == 0) goto found;
		sprintf(FullFileName, "%s/lib%s.so", Node->Path, Name);
		if (stat(FullFileName, &Stat) == 0) goto found;
#endif
#ifdef WINDOWS
		sprintf(FullFileName, "%s%s.rlib", Node->Path, Name);
		if (stat(FullFileName, &Stat) == 0) {
			add_file(FullFileName, 1);
			return;
		};
		sprintf(FullFileName, "%s/%s.rlib", Node->Path, Name);
		if (stat(FullFileName, &Stat) == 0) {
			add_file(FullFileName, 1);
			return;
		};
		
		/*
		/// TODO: The following line(s) is a cheap hack
		if (!strcmp(Name, "gdi32")) return;
		if (!strcmp(Name, "m")) return;
		
		sprintf(FullFileName, "%slib%s.dll", Node->Path, Name);
		if (stat(FullFileName, &Stat) == 0) goto found;
		sprintf(FullFileName, "%scyg%s.dll", Node->Path, Name);
		if (stat(FullFileName, &Stat) == 0) goto found;
		for (int I = 20; I >= 0; --I) {
			sprintf(FullFileName, "%slib%s-%d.dll", Node->Path, Name, I);
			if (stat(FullFileName, &Stat) == 0) goto found;
			sprintf(FullFileName, "%scyg%s-%d.dll", Node->Path, Name, I);
			if (stat(FullFileName, &Stat) == 0) goto found;
			sprintf(FullFileName, "%s%s%d.dll", Node->Path, Name, I);
			if (stat(FullFileName, &Stat) == 0) goto found;
		};
		*/
#endif
	};
	fprintf(stderr, "%s: shared library not found.\n", Name);
	exit(1);
found:
	add_shared_lib(strdup(FullFileName), 1);
};

static bool print_dependency(const struct HXmap_node *Node, void *Arg) {
	printf("\t%s\n", Node->skey);
	return true;
};

int main(int Argc, char **Argv) {
	HX_init();
	LibraryTable = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
	GlobalTable = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
	WeakTable = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
	SymbolTable = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
	ExportTable = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
	SupportedFiles = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY);
	Dependencies = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY | HXMAP_SINGULAR);
	int Version = 0;
	if (Argc < 2) {
		puts("Usage: rlink [-o output] [-l listing] [-L directory] inputs ... ");
	} else {
		bfd_init();
		//HXmap_add(GlobalTable, "_GLOBAL_OFFSET_TABLE_", new_symbol("_GLOBAL_OFFSET_TABLE_", new_got_section(), 0));
		UnknownSymbols = new_library_section("__unknown__");
		HXmap_add(SupportedFiles, ".rlib", (void *)add_library_file);
		HXmap_add(SupportedFiles, ".rdef", (void *)add_definition_file);
		HXmap_add(SupportedFiles, ".rlink", (void *)add_script_file);
		HXmap_add(SupportedFiles, ".a", (void *)add_object_file);
		HXmap_add(SupportedFiles, ".o", (void *)add_object_file);
		HXmap_add(SupportedFiles, ".obj", (void *)add_object_file);
		HXmap_add(SupportedFiles, ".mo", (void *)add_object_file);
		HXmap_add(SupportedFiles, ".io", (void *)add_object_file);
		HXmap_add(SupportedFiles, ".so", (void *)add_shared_lib);
		HXmap_add(SupportedFiles, ".dll", (void *)add_shared_lib);
		char *OutFile = 0;
		char *ListFile = 0;
		char *ExportFile = 0;
		char *ModuleName = 0;
		for (int I = 1; I < Argc; ++I) {
			if (Argv[I][0] == '-') {
				switch (Argv[I][1]) {
				case 'o':
					if (Argv[I][2]) {
						OutFile = Argv[I] + 2;
					} else {
						OutFile = Argv[++I];
					}
					break;
				case 'l':
					//add_file(Argv[I] + 2, 0);
					find_shared_lib(Argv[I] + 2);
					break;
				case 'x':
					add_file(Argv[I] + 2, 1);
					break;
				case 'L':
					if (Argv[I][2]) {
						add_path(Argv[I] + 2);
					} else {
						add_path(Argv[++I]);
					}
				case 's':
					break;
                case 'm':
                    Platform = Argv[I] + 2;
                    break;
				case '?':
					if (Argv[I][2]) {
						ListFile = Argv[I] + 2;
					} else {
						ListFile = Argv[++I];
					}
					break;
				case 'X':
					if (Argv[I][2]) {
						ExportFile = Argv[I] + 2;
					} else {
						ExportFile = Argv[++I];
					}
					break;
				case 'N':
					if (Argv[I][2]) {
						ModuleName = Argv[I] + 2;
					} else {
						ModuleName = Argv[++I];
					}
					break;
				case 'v':
					Version = atoi(Argv[I] + 2);
					break;
				case 'd':
					DelayedLink = 0;
					break;
				case 'Z':
					StopOnUnknown = 0;
					break;
				case 'D': {
					char *Equals = strchr(Argv[I] + 2, '=');
					if (Equals) {
						*Equals = 0;
						add_define(Argv[I] + 2, Equals + 1);
					} else {
						add_define(Argv[I] + 2, "TRUE");
					};
					break;
				};
				case 'M': {
					DependencyMode = 1;
					break;
				};
				};
			} else {
				add_file(Argv[I], 0);
			};
		};
		if (DependencyMode) {
			printf("Printing dependencies [%d]...\n", Dependencies->items);
			HXmap_qfe(Dependencies, print_dependency, 0);
			return 0;
		};
		for (export_t *Export = Exports.Head; Export; Export = Export->Next) {
			if (Export->Section == 0) {
				symbol_t *Symbol = (symbol_t *)HXmap_get(GlobalTable, Export->Internal);
				if (Symbol == 0) Symbol = (symbol_t *)HXmap_get(WeakTable, Export->Internal);
				if (Symbol) {
					if (Symbol->Section) section_export(Symbol->Section, Symbol->Offset, Export);
				} else {
					fprintf(stderr, "exported symbol not found: %s.\n", Export->Internal);
					exit(1);
				};
			};
			if (Export->Section) section_require(Export->Section);
		};
		if (ExportFile) {
			FILE *File = fopen(ExportFile, "w");
			if (ModuleName) {
				fprintf(File, "module(");
				char *PartStart = ModuleName;
				char *PartEnd = strchr(PartStart, '/');
				while (PartEnd) {
					*PartEnd = 0;
					fprintf(File, "\"%s\", ", PartStart);
					PartStart = PartEnd + 1;
					PartEnd = strchr(PartStart, '/');
				}
				fprintf(File, "\"%s\")\n", PartStart);
			}
			for (export_t *Export = Exports.Head; Export; Export = Export->Next) {
				if (Export->Section) fprintf(File, "import(\"%s\")\n", Export->External);
			};
			fclose(File);
		};
		if (InitialSection) section_require(InitialSection);
		if (MethodsSection) section_require(MethodsSection);
		if (TypedFnSection) section_require(TypedFnSection);
		if (ListFile) {
			FILE *File = fopen(ListFile, "w");
			fprintf(File, "version: %d, %d\n", RIVA_VERSION, Version);
			for (section_t *Section = Sections.Head; Section; Section = Section->Next) section_debug(Section, File);
			for (export_t *Export = Exports.Head; Export; Export = Export->Next) {
				if (Export->Section) fprintf(File, "export: %s -> %d[%d]\n", Export->External, Export->Section->Index, Export->Offset);
			};
			for (require_t *Require = Requires.Head; Require; Require = Require->Next) {
				fprintf(File, "require: %s[%d]\n", Require->Library, Require->Flags);
			};
			fclose(File);
		};
		if (OutFile) {
			int Fd = open(OutFile, O_WRONLY | O_CREAT, 0644);
			write(Fd, "RIVA", 4);
			
			gzFile File = gzdopen(Fd, "wb9");
			uint32_t Temp;
			Temp = RIVA_VERSION; gzwrite(File, &Temp, 4);
			Temp = Version; gzwrite(File, &Temp, 4);
			Temp = NoOfSections; gzwrite(File, &Temp, 4);
			Temp = NoOfExports; gzwrite(File, &Temp, 4);
			Temp = NoOfRequires; gzwrite(File, &Temp, 4);
			for (section_t *Section = Sections.Head; Section; Section = Section->Next) section_write(Section, File);
			for (export_t *Export = Exports.Head; Export; Export = Export->Next) {
				if (Export->Section) {
					Temp = Export->Flags; gzwrite(File, &Temp, 1);
					Temp = Export->Section->Index; gzwrite(File, &Temp, 4);
					Temp = Export->Offset; gzwrite(File, &Temp, 4);
					Temp = strlen(Export->External) + 1; gzwrite(File, &Temp, 4);
					gzwrite(File, Export->External, Temp);
				};
			};
			for (require_t *Require = Requires.Head; Require; Require = Require->Next) {
				Temp = Require->Flags; gzwrite(File, &Temp, 1);
				Temp = strlen(Require->Library) + 1; gzwrite(File, &Temp, 4);
				gzwrite(File, Require->Library, Temp);
			};
			gzclose(File);
		};
	};
};
