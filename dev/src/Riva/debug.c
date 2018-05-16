#include "libriva.h"
#include <stdlib.h>

#ifdef USE_UDIS
#include <udis86.h>
#endif

#ifdef GC_debug
#define DEBUG_HDR_INDEX 4
#else
#define DEBUG_HDR_INDEX 0
#endif

const char DebugKey[] = "Debug.Key";
int DebugLevels = 10, MaxStackScan = 256;

extern void memory_log_close(void);

static int display_reg(const char *Name, void *Value, int ShowAlways) {
	const char **Base = GC_base(Value);
	if (Base) {
		const char *Module, *Symbol;
		if (Base[DEBUG_HDR_INDEX] == DebugKey) {
			debug_hdr *Hdr = (debug_hdr *)Base[DEBUG_HDR_INDEX + 1];
			printf("> %s = %s:%d[0x%x]\n", Name, Hdr->StrInfo, Hdr->IntInfo, (const char *)Value - (const char *)Base);
			return 1;
		} else if (module_lookup(Base, &Module, &Symbol)) {
			printf("- %s = %s.%s\n", Name, Module, Symbol);
			return 0;
		} else if (ShowAlways) {
			printf("? %s = 0x%x\n", Name, Base);
		};
	} else if (ShowAlways) {
		printf("? %s = near 0x%x\n", Name, Value);
	};
	return 0;
};

#ifdef LINUX || MACOSX
int stack_trace(void **Stack, char **Buffer, int BufferSize) {
	int Count = 0;
	for (int I = 0; (Count < BufferSize) && (I < MaxStackScan); ++I) {
		unsigned const char *Code = Stack[I];
		const char **Base = GC_base(Code);
		if (Base) {
			if (Base[DEBUG_HDR_INDEX] == DebugKey) {
				if (Code[-5] == 0xE8) {
					debug_hdr *Hdr = (debug_hdr *)Base[DEBUG_HDR_INDEX + 1];
					asprintf(Buffer + Count, "%s, %d (+0x%x)", Hdr->StrInfo, Hdr->IntInfo, (const char *)Stack[I] - (const char *)Base - 8);
					++Count;
				} else if (Code[-3] == 0xFF) {
					debug_hdr *Hdr = (debug_hdr *)Base[DEBUG_HDR_INDEX + 1];
					asprintf(Buffer + Count, "%s, %d (+0x%x)", Hdr->StrInfo, Hdr->IntInfo, (const char *)Stack[I] - (const char *)Base - 8);
					++Count;
				}
			};
		};
	};
	return Count;
};

void display_stack(void *Stack, int Levels) {
	for (int I = 0; I < MaxStackScan; ++I) {
		Levels -= display_reg("\t", ((void **)Stack)[I], 0);
		if (!Levels) break;
	};
};

static void segv_handler(int Signal, struct sigcontext Context) {
	memory_log_close();

	printf("Segmentation fault in thread %x", pthread_self());
	const char *Module, *Symbol;

	void *Eip = (void *)Context.eip;
	if (Eip == 0) {
		printf(" calling 0x0 ");
		Eip = ((void **)Context.esp)[0];
	};

	const char **Base = GC_base(Eip);
	if (Base) {
		if (Base[DEBUG_HDR_INDEX] == DebugKey) {
			debug_hdr *Hdr = (debug_hdr *)Base[DEBUG_HDR_INDEX + 1];
			printf(" in %s:%d @ %x\n", Hdr->StrInfo, Hdr->IntInfo, Eip - (void *)Base);
		} else if (module_lookup(Base, &Module, &Symbol)) {
			printf(" in %s.%s @ %x\n", Module, Symbol, Eip - (void *)Base);
		} else {
			printf(" in 0x%x @ %x\n", Base, Eip - (void *)Base);
		};
	} else {
		printf(" near 0x%x\n", Eip);
	};

	printf("eax = %08x, ebx = %08x, ecx = %08x, edx = %08x\n", Context.eax, Context.ebx, Context.ecx, Context.edx);
	printf("edi = %08x, esi = %08x, ebp = %08x, esp = %08x\n", Context.edi, Context.esi, Context.ebp, Context.esp);
#ifdef USE_UDIS
	ud_t UD[1];
	ud_init(UD);
	ud_set_input_buffer(UD, Eip, 100);
	ud_set_pc(UD, (uint32_t)Eip);
	ud_set_mode(UD, 32);
	ud_set_syntax(UD, UD_SYN_INTEL);
	for (int I = 10; (--I) && ud_disassemble(UD);) printf("\t0x%6x: %s\n", (uint32_t)ud_insn_off(UD), ud_insn_asm(UD));
#endif

	display_reg("eax", (void *)Context.eax, 1);
	display_reg("ebx", (void *)Context.ebx, 1);
	display_reg("ecx", (void *)Context.ecx, 1);
	display_reg("edx", (void *)Context.edx, 1);

	if (Context.ebp) display_stack((void *)Context.ebp, DebugLevels);
	
	exit(1);
};
#endif

debug_hdr *debug_get_hdr(void *Ptr) {
	const char **Base = GC_base((void *)Ptr);
	if (Base == 0) return 0;
#ifdef GC_DEBUG
	if (Base[4] != DebugKey) return 0;
	return (debug_hdr *)Base[5];
#else
	if (Base[0] != DebugKey) return 0;
	return (debug_hdr *)Base[1];
#endif
};

static void debug_gc_warn(char *Message, GC_word Data) {
	log_writef(Message, Data);
	if (Data) {
		debug_hdr *Hdr = debug_get_hdr((void *)Data);
		if (Hdr) log_writef("%s:%d\n", Hdr->StrInfo, Hdr->IntInfo);
	};
	//display_stack((char *)&Data + 4, DebugLevels);
};

void debug_init(void) {
#ifdef LINUX
	struct sigaction Action;
	Action.sa_handler = (void *)segv_handler;
	sigemptyset(&Action.sa_mask);
	Action.sa_flags = SA_RESTART;
	sigaction(SIGSEGV, &Action, NULL);
#endif
	GC_set_warn_proc(debug_gc_warn);
	module_t *Module = module_new("Riva/Debug");
	module_add_alias(Module, "library:/Riva/Debug");
	module_export(Module, "Key", 0, (void *)DebugKey);
	module_export(Module, "_stack_trace", 0, (void *)stack_trace);

#ifdef USE_UDIS
	Module = module_new("libudis");
	module_add_alias(Module, "library:/libudis");
	module_export(Module, "ud_disassemble", 0, (void *)ud_disassemble);
	module_export(Module, "ud_insn_off", 0, (void *)ud_insn_off);
	module_export(Module, "ud_insn_asm", 0, (void *)ud_insn_asm);
	module_export(Module, "ud_set_syntax", 0, (void *)ud_set_syntax);
	module_export(Module, "ud_translate_intel", 0, (void *)ud_translate_intel);
	module_export(Module, "ud_set_pc", 0, (void *)ud_set_pc);
	module_export(Module, "ud_set_mode", 0, (void *)ud_set_mode);
	module_export(Module, "ud_set_input_buffer", 0, (void *)ud_set_input_buffer);
	module_export(Module, "ud_init", 0, (void *)ud_init);
#endif
};
