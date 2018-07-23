#include "libriva.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <zlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef WINDOWS
#include <windows.h>
#endif

#define RIVA_VERSION 3

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

#define RELOC_ABS	0
#define RELOC_REL	1
#define RELOC_IND	2
#define RELOC_GOT	3

#define LIBRARY_ABS	0
#define LIBRARY_REL	1

#define EXP_CONSTANT	0
#define EXP_VARIABLE	1

#define FLAG_GC		1
#define FLAG_DELAY	2

typedef struct reloc_t {
	uint32_t Size;
	uint32_t Flags;
	uint32_t Position;
	struct section_t *Section;
} reloc_t;

#define UNLOADED	0
#define LOADING		1
#define LOADED		2

typedef struct section_t {
	uint32_t (*Fixup)(struct section_t *Section, jmp_buf OnError);
	uint32_t Flags;
	uint32_t NoOfFixups;
	uint8_t *Data;
	union {
		struct {
			uint32_t NoOfRelocs;
			reloc_t *Relocs;
			uint8_t *TData;
		};
		struct {
			char *Name;
			union {
				struct riva_t *SubRiva;
				struct section_t *Library;
				const char *Path;
			};
		};
		struct {
			struct section_t *Init;
			uint32_t Offset;
		};
	};
} section_t;

typedef struct export_t {
	uint32_t Flags;
	section_t *Section;
	uint32_t Offset;
} export_t;

typedef struct riva_t {
	stringtable_t Exports;
	const char *FileName;
} riva_t;

typedef struct code_header_t {
	debug_hdr Hdr;
	section_t *Section;
	uint32_t NoOfRelocs;
	uint32_t Restore[2];
	void *Links[];
} code_header_t;

static uint32_t fixup_text_section(section_t *Section, jmp_buf OnError) {
	uint8_t *Data = Section->Data;
	if (Section->NoOfFixups) {
		code_header_t *Header = ((code_header_t **)Data)[-1];
		while (Section->NoOfFixups) {
			int Index = Section->NoOfRelocs - Section->NoOfFixups--;
			const reloc_t *Reloc = &Section->Relocs[Index];
			uint32_t Value;
			if (Reloc->Section == Section) {
				Value = (uint32_t)Data;
			} else {
				Value = Reloc->Section->Fixup(Reloc->Section, OnError);
			};
			Header->Links[Index] = (void *)Value;
			if (Reloc->Flags == RELOC_REL) {
				Value -= (uint32_t)Data;
			} else if (Reloc->Flags == RELOC_IND) {
				uint32_t Offset = *(uint32_t *)(Data + Reloc->Position);
				*(uint32_t *)(Data + Reloc->Position) = 0;
				Header->Links[Index] += Offset;
				Value = (uint32_t)&Header->Links[Index];
			} else if (Reloc->Flags == RELOC_GOT) {
				Value = -Value;
			};
			switch (Reloc->Size) {
			case 1:
				*(uint8_t *)(Data + Reloc->Position) += Value;
				break;
			case 2:
				*(uint16_t *)(Data + Reloc->Position) += Value;
				break;
			case 4:
				*(uint32_t *)(Data + Reloc->Position) += Value;
				break;
			};
		};
		Section->Relocs = 0;
	};
	return (uint32_t)Data;
};

extern void init_text_section(void);

void relocate_text_section(uint8_t *Data) {
	code_header_t *Header = ((code_header_t **)Data)[-1];

	//pthread_mutex_lock(LibRivaMutex);
	section_t *Section = Header->Section;
	if (!Section->Relocs) return;
	
	jmp_buf OnError;
	if (setjmp(OnError)) {
		printf("Error relocating [%x] %s:%d\n", Data, Header->Hdr.StrInfo, Header->Hdr.IntInfo);
		exit(1);
	};

	//printf("Relocating [%x] %s:%d\n", Data, Header->Hdr.StrInfo, Header->Hdr.IntInfo);
	((uint32_t *)Data)[0] = Header->Restore[0];
	((uint32_t *)Data)[1] = Header->Restore[1];
	
	for (int Index = 0; Index < Section->NoOfRelocs; ++Index) {
		const reloc_t *Reloc = &Section->Relocs[Index];
		uint32_t Value;
		if (Reloc->Section == Section) {
			Value = (uint32_t)Data;
		} else {
			Value = Reloc->Section->Fixup(Reloc->Section, OnError);
			Header->Links[Index] = (void *)Value;
		};
		if (Reloc->Flags == RELOC_REL) Value -= (uint32_t)Data;
		switch (Reloc->Size) {
		case 1:
			*(uint8_t *)(Data + Reloc->Position) += Value;
			break;
		case 2:
			*(uint16_t *)(Data + Reloc->Position) += Value;
			break;
		case 4:
			*(uint32_t *)(Data + Reloc->Position) += Value;
			break;
		};
	};
	Section->Relocs = 0;
	//pthread_mutex_unlock(LibRivaMutex);

	//printf("Relocated [%x] %s:%d\n", Data, Header->Hdr.StrInfo, Header->Hdr.IntInfo);
};

static uint32_t fixup_text_section_delayed(section_t *Section, jmp_buf OnError) {
	uint8_t *Data = Section->Data;
	if (Section->NoOfFixups) {
		code_header_t *Header = ((code_header_t **)Data)[-1];
		Header->Section = Section;
		Header->NoOfRelocs = Section->NoOfRelocs;
		Header->Restore[0] = ((uint32_t *)Data)[0];
		Header->Restore[1] = ((uint32_t *)Data)[1];
		Data[0] = 0xE8;
		*(uint32_t *)(Data + 1) = ((unsigned char *)init_text_section - Data) - 5;
		Section->NoOfFixups = 0;
	};
	return (uint32_t)Data;
};

static uint32_t fixup_data_section(section_t *Section, jmp_buf OnError) {
	uint8_t *Data = Section->Data;
	if (Section->NoOfFixups) {
		while (Section->NoOfFixups) {
			int Index = Section->NoOfRelocs - Section->NoOfFixups--;
			const reloc_t *Reloc = &Section->Relocs[Index];
			uint32_t Value;
			if (Reloc->Section == Section) {
				Value = (uint32_t)Data;
			} else {
				Value = Reloc->Section->Fixup(Reloc->Section, OnError);
			};
			if (Reloc->Flags == RELOC_REL) {
				Value -= (uint32_t)Data;
			} else if (Reloc->Flags == RELOC_GOT) {
				Value = -Value;
			};
			switch (Reloc->Size) {
			case 1:
				*(uint8_t *)(Data + Reloc->Position) += Value;
				break;
			case 2:
				*(uint16_t *)(Data + Reloc->Position) += Value;
				break;
			case 4:
				*(uint32_t *)(Data + Reloc->Position) += Value;
				break;
			};
		};
		Section->Relocs = 0;
	};
	return (uint32_t)Data;
};

static uint32_t fixup_tdata_section(section_t *Section, jmp_buf OnError) {
	uint8_t *TData = Section->TData;
	if (Section->NoOfFixups) {
		while (Section->NoOfFixups) {
			int Index = Section->NoOfRelocs - Section->NoOfFixups--;
			const reloc_t *Reloc = &Section->Relocs[Index];
			uint32_t Value;
			if (Reloc->Section == Section) {
				Value = (uint32_t)TData;
			} else {
				Value = Reloc->Section->Fixup(Reloc->Section, OnError);
			};
			if (Reloc->Flags == RELOC_REL) {
				Value -= (uint32_t)TData;
			} else if (Reloc->Flags == RELOC_GOT) {
				Value = -Value;
			};
			switch (Reloc->Size) {
			case 1:
				*(uint8_t *)(TData + Reloc->Position) += Value;
				break;
			case 2:
				*(uint16_t *)(TData + Reloc->Position) += Value;
				break;
			case 4:
				*(uint32_t *)(TData + Reloc->Position) += Value;
				break;
			};
		};
		Section->Relocs = 0;
	};
	return (uint32_t)Section->Data;
};

static uint32_t fixup_library_section(section_t *Section, jmp_buf OnError) {
	if (Section->NoOfFixups) {
		module_t *Module = module_load(Section->Path, Section->Name);
		if (Module == 0) {
			log_errorf("Error: module not found %s\n", Section->Name);
			longjmp(OnError, 1);
		};
		Section->Data = (uint8_t *)Module;
		Section->NoOfFixups = 0;
	};
	return (uint32_t)Section->Data;
};

static uint32_t fixup_import_section(section_t *Section, jmp_buf OnError) {
	if (Section->NoOfFixups) {
		module_t *Module = (module_t *)Section->Library->Fixup(Section->Library, OnError);
		int IsRef;
		if (module_import(Module, Section->Name, &IsRef, (void **)&Section->Data)) {
			Section->NoOfFixups = 0;
		} else {
			log_errorf("Error: symbol %s not exported from %s\n", Section->Name, Section->Library->Name);
			longjmp(OnError, 1);
		};
	};
	return (uint32_t)Section->Data;
};

static uint32_t fixup_bss_section(section_t *Section, jmp_buf OnError) {
	return (uint32_t)Section->Data;
};

static uint32_t fixup_tbss_section(section_t *Section, jmp_buf OnError) {
	printf("TBSS SECTION: 0x%x\n", Section->Data);
	return (uint32_t)Section->Data;
};

static uint32_t fixup_symbol_section(section_t *Section, jmp_buf OnError) {
	if (Section->NoOfFixups) {
		if (Section->Name) {
			int IsRef;
			module_import(Symbol, Section->Name, &IsRef, (void **)&Section->Data);
		} else {
			Section->Data = make_symbol("<anonymous>");
		};
		Section->NoOfFixups = 0;
	};
	return (uint32_t)Section->Data;
};

static uint32_t fixup_constant_section(section_t *Section, jmp_buf OnError) {
	if (Section->NoOfFixups) {
		void *(*init)(void) = (void *)(Section->Init->Fixup(Section->Init, OnError) + Section->Offset);
		Section->Data = init();
		Section->NoOfFixups = 0;
	};
	return (uint32_t)Section->Data;
};

static int riva_import(riva_t *Riva, const char *Symbol, int *IsRef, void **Data);

static uint32_t fixup_module_section(section_t *Section, jmp_buf OnError) {
	if (Section->NoOfFixups) {
		Section->NoOfFixups = 0;
	};
	return (uint32_t)Section->Data;
};

static void *check_import(riva_t *Riva, const char *Symbol, jmp_buf OnError) {
	const export_t *Export = stringtable_get(Riva->Exports, Symbol);
	if (Export) {
		void *Data = Export->Section->Data + Export->Offset;
		Export->Section->Fixup(Export->Section, OnError);
		if (Export->Section->NoOfFixups) log_errorf("(check_import) Section still has %d unapplied fixups.\n", Export->Section->NoOfFixups);
		return Data;
	} else {
		return 0;
	};
};

static void init_add_typedfns(void *Data);
static void (*add_typedfns)(void *Data) = init_add_typedfns;

static void init_add_typedfns(void *Data) {
	module_t *Module = module_load(0, "Util/TypedFunction");
	int IsRef0;
	module_import(Module, "_add_typedfns", &IsRef0, (void **)&add_typedfns);
	return add_typedfns(Data);
};

static int riva_import(riva_t *Riva, const char *Symbol, int *IsRef, void **Data) {
	const export_t *Export = stringtable_get(Riva->Exports, Symbol);
	if (Export) {
		jmp_buf OnError;
		if (setjmp(OnError)) return 0;
		*IsRef = Export->Flags & EXP_VARIABLE;
		Export->Section->Fixup(Export->Section, OnError);
		if (Export->Section->NoOfFixups) log_errorf("(riva_import) Section still has %d unapplied fixups.\n", Export->Section->NoOfFixups);
		*Data = Export->Section->Data + Export->Offset;
		return 1;
	} else {
		return 0;
	};
};

static void *riva_find(const char *Base) {
	char Buffer[strlen(Base) + 6];
	strcpy(stpcpy(Buffer, Base), ".riva");
	struct stat Stat[1];
	if (stat(Buffer, Stat)) return 0;
	return GC_STRDUP(Buffer);
};

#ifdef WINDOWS
int RivaDelayedLink = 0;
#else
int RivaDelayedLink = 0;
#endif

static int riva_load(module_provider_t *Provider, const char *FileName) {
	riva_t *Riva = new(riva_t);

	Riva->FileName = FileName;

	module_importer_set(Provider, Riva, (module_import_func)riva_import);

	int Fd = open(FileName, O_RDONLY);
	if (Fd < 0) return 0;
	uint32_t Magic;
	if (read(Fd, &Magic, 4) < 4) {close(Fd); return 0;};
	if (Magic != 0x41564952) {close(Fd); return 0;};

	gzFile File = gzdopen(Fd, "rb");

	uint32_t RivaVersion; gzread(File, &RivaVersion, 4);
	uint32_t SourceVersion; gzread(File, &SourceVersion, 4);
	uint32_t NoOfSections; gzread(File, &NoOfSections, 4);
	uint32_t NoOfExports; gzread(File, &NoOfExports, 4);
	uint32_t NoOfRequires; gzread(File, &NoOfRequires, 4);

	if (RivaVersion != RIVA_VERSION) {
		log_errorf("Warning: rlink version mismatch:%s (%d/%d)\n", FileName, RivaVersion, RIVA_VERSION);
	};

	jmp_buf OnError;

	if (setjmp(OnError)) {gzclose(File); return 0;};

	section_t *InitialSection = 0;
	section_t *MethodsSection = 0;
	section_t *TypedFnSection = 0;

	section_t **Sections = (section_t **)GC_MALLOC(NoOfSections * sizeof(section_t *));
	for (int I = 0; I < NoOfSections; ++I) Sections[I] = new(section_t);
	for (int I = 0; I < NoOfSections; ++I) {
		section_t *Section = Sections[I];

		uint8_t Type; gzread(File, &Type, 1);
		switch (Type) {
		case SECT_TEXT: {
			gzread(File, &Section->Flags, 1);
			Section->Fixup = (RivaDelayedLink && (Section->Flags & FLAG_DELAY)) ? fixup_text_section_delayed : fixup_text_section;
			uint32_t Length; gzread(File, &Length, 4);
			uint32_t NoOfRelocs; gzread(File, &NoOfRelocs, 4);
			Section->NoOfRelocs = NoOfRelocs;
			Section->NoOfFixups = NoOfRelocs;

			reloc_t *Relocs = (Section->Relocs = (reloc_t *)GC_MALLOC(NoOfRelocs * sizeof(reloc_t)));

			code_header_t **Code0 = memory_alloc_code(Length > 16 ? Length : 16);
			code_header_t *Header = Code0[0] = GC_MALLOC(sizeof(code_header_t) + NoOfRelocs * sizeof(void *));
			Header->Hdr.StrInfo = FileName;
			Header->Hdr.IntInfo = I;
			Section->Data = (uint8_t *)(Code0 + 1);
			gzread(File, Section->Data, Length);
			for (int J = 0; J < NoOfRelocs; ++J) {
				reloc_t *Reloc = &Relocs[J];
				gzread(File, &Reloc->Size, 1);
				gzread(File, &Reloc->Flags, 1);
				gzread(File, &Reloc->Position, 4);
				uint32_t Index; gzread(File, &Index, 4);
				Reloc->Section = Sections[Index];
			};
		break;};
		case SECT_METHODS: MethodsSection = Section; goto data_section;
		case SECT_INITIAL: InitialSection = Section; goto data_section;
		case SECT_TYPEDFN: TypedFnSection = Section; goto data_section;
		case SECT_DATA: data_section: {
			Section->Fixup = fixup_data_section;
			gzread(File, &Section->Flags, 1);
			uint32_t Length; gzread(File, &Length, 4);
			uint32_t NoOfRelocs; gzread(File, &NoOfRelocs, 4);
			Section->NoOfRelocs = NoOfRelocs;
			Section->NoOfFixups = NoOfRelocs;

			reloc_t *Relocs = (Section->Relocs = (reloc_t *)GC_MALLOC(NoOfRelocs * sizeof(reloc_t)));
			//Section->Data = GC_MALLOC_UNCOLLECTABLE(Length);
			uint8_t *Data = GC_MALLOC(Length + 16);
			//uint8_t *Data = GC_MALLOC_UNCOLLECTABLE(Length + 16);
			Data += 15;
			Data -= (uint32_t)Data % 16;
			Section->Data = Data;
			gzread(File, Section->Data, Length);
			for (int J = 0; J < NoOfRelocs; ++J) {
				reloc_t *Reloc = &Relocs[J];
				gzread(File, &Reloc->Size, 1);
				gzread(File, &Reloc->Flags, 1);
				gzread(File, &Reloc->Position, 4);
				uint32_t Index; gzread(File, &Index, 4);
				Reloc->Section = Sections[Index];
			};
		break;};
		case SECT_LIBRARY: {
			Section->Fixup = fixup_library_section;
			gzread(File, &Section->Flags, 1);
			uint32_t Length; gzread(File, &Length, 4);
			gzread(File, Section->Name = (char *)GC_MALLOC_ATOMIC(Length), Length);
			Section->NoOfFixups = 1;
			if (Section->Flags == LIBRARY_ABS) {
				Section->Path = 0;
			} else if (Section->Flags == LIBRARY_REL) {
				Section->Path = module_get_path(Provider->Module);
			};
		break;};
		case SECT_IMPORT: {
			Section->Fixup = fixup_import_section;
			gzread(File, &Section->Flags, 1);
			uint32_t Index; gzread(File, &Index, 4);
			Section->Library = Sections[Index];
			uint32_t Length; gzread(File, &Length, 4);
			gzread(File, Section->Name = (char *)GC_MALLOC_ATOMIC(Length), Length);
			Section->NoOfFixups = 1;
		break;};
		case SECT_BSS: {
			Section->Fixup = fixup_bss_section;
			gzread(File, &Section->Flags, 1);
			uint32_t Size; gzread(File, &Size, 4);
			uint8_t *Data = GC_MALLOC(Size + 16);
			Data += 15;
			Data -= (uint32_t)Data % 16;
			Section->Data = Data;
			Section->NoOfFixups = 0;
		break;};
		case SECT_SYMBOL: {
			Section->Fixup = fixup_symbol_section;
			gzread(File, &Section->Flags, 1);
			uint32_t Length; gzread(File, &Length, 4);
			if (Length == 0xFFFFFFFF) {
				Section->Name = 0;
			} else {
				gzread(File, Section->Name = (char *)GC_MALLOC_ATOMIC(Length), Length);
			};
			Section->NoOfFixups = 1;
		break;};
		case SECT_CONSTANT: {
			Section->Fixup = fixup_constant_section;
			gzread(File, &Section->Flags, 1);
			uint32_t Index; gzread(File, &Index, 4);
			Section->Init = Sections[Index];
			gzread(File, &Section->Offset, 4);
			Section->NoOfFixups = 1;
		break;};
		case SECT_MODULE: {
			Section->Fixup = fixup_module_section;
			gzread(File, &Section->Flags, 1);
			char *SubFileName;
			riva_t *SubRiva = Section->SubRiva = new(riva_t);
			asprintf(&SubFileName, "%s/%d", FileName, I) == 0;
			SubRiva->FileName = SubFileName;
			module_t *Module = module_new(SubRiva->FileName);
			module_importer_set(Module->Providers, SubRiva, (module_import_func)riva_import);
			Section->Data = (uint8_t *)Module;
			Section->NoOfFixups = 0;
			uint32_t NoOfExports; gzread(File, &NoOfExports, 4);
			for (int I = 0; I < NoOfExports; ++I) {
				export_t *Export = new(export_t);
				gzread(File, &Export->Flags, 1);
				uint32_t Index; gzread(File, &Index, 4);
				Export->Section = Sections[Index];
				gzread(File, &Export->Offset, 4);
				uint32_t Length; gzread(File, &Length, 4);
				char *Name = (char *)GC_MALLOC_ATOMIC(Length);
				gzread(File, Name, Length);
				stringtable_put(SubRiva->Exports, Name, Export);
			};
		break;};
		case SECT_TDATA: {
			puts("Error: tdata sections are not implemented yet!");
			exit(0);
		break;};
		case SECT_TBSS: {
			static size_t tbss_offset = 0;
			Section->Fixup = fixup_tbss_section;
			gzread(File, &Section->Flags, 1);
			uint32_t Size; gzread(File, &Size, 4);
			tbss_offset -= Size;
			Section->Data = tbss_offset;
			/*uint8_t *Data = GC_MALLOC(Size + 16);
			Data += (16 - (uint32_t)Data & 15) & 15;
			Section->Data = Data;*/
			Section->NoOfFixups = 0;
		break;};
		case SECT_GOT: {
		break;};
		};
	};
	for (int I = 0; I < NoOfExports; ++I) {
		export_t *Export = new(export_t);
		gzread(File, &Export->Flags, 1);
		uint32_t Index; gzread(File, &Index, 4);
		Export->Section = Sections[Index];
		gzread(File, &Export->Offset, 4);
		uint32_t Length; gzread(File, &Length, 4);
		char *Name = (char *)GC_MALLOC_ATOMIC(Length);
		gzread(File, Name, Length);
		stringtable_put(Riva->Exports, Name, Export);
	};
	for (int I = 0; I < NoOfRequires; ++I) {
		uint8_t Flags; gzread(File, &Flags, 1);
		uint32_t Length; gzread(File, &Length, 4);
		char *Name = (char *)GC_MALLOC_ATOMIC(Length);
		const char *Path = 0;
		gzread(File, Name, Length);
		if (Flags == LIBRARY_REL) Path = module_get_path(Provider->Module);
		module_load(Path, Name);
	};
	gzclose(File);

	if (MethodsSection) {
		log_writef("Adding methods for %s\n", FileName);
		MethodsSection->Fixup(MethodsSection, OnError);
		add_methods(MethodsSection->Data);
		log_writef("Added methods for %s\n", FileName);
	};

	if (TypedFnSection) {
		log_writef("Adding typed functions for %s\n", FileName);
		TypedFnSection->Fixup(TypedFnSection, OnError);
		add_typedfns(TypedFnSection->Data);
		log_writef("Added typed functions for %s\n", FileName);
	};

	if (InitialSection) {
		log_writef("Initializing %s\n", FileName);
		InitialSection->Fixup(InitialSection, OnError);
		for (void (**Initial)(module_provider_t *) = (void *)InitialSection->Data; Initial[0] != (void *)0xffffffff; ++Initial) Initial[0](Provider);
		log_writef("Initialized %s\n", FileName);
	};

	return 1;
};

void riva_init(void) {
	module_add_loader("Riva", 100, riva_find, riva_load);
};
