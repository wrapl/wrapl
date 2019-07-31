#include <Riva.h>
#include <Std/Object.h>
#include <Agg/StringTable.h>
#include <stdio.h>
#include <stdlib.h>

static Agg$StringTable$t GTypeModules[] = {Agg$StringTable$INIT};

INITIAL() {
	FILE *MapFile = fopen("Types.map", "r");
	char Buffer[256];
	while (fgets(Buffer, 256, MapFile)) {
		int Length = strlen(Buffer);
		char *Temp = strchr(Buffer, '=');
		if (Temp) {
			int L0 = Temp - Buffer - 1;
			int L1 = Length - L0 - 4;
			char *GtkName = Riva$Memory$alloc_atomic(L0 + 1);
			char *RivaName = Riva$Memory$alloc_atomic(L1 + 1);
			memcpy(GtkName, Buffer, L0);
			GtkName[L0] = 0;
			memcpy(RivaName, Temp + 2, L1);
			RivaName[L1] = 0;
			printf("Adding type: %s -> %s\n", GtkName, RivaName);
			Agg$StringTable$put(GTypeModules, GtkName, strlen(GtkName), (void *)RivaName);
		} else {
			printf("Skipping line: %s", Buffer);
		};
	};
	fclose(MapFile);
	FILE *OutFile = fopen("TypeMap.c", "w");
	fprintf(OutFile, "#include <Agg/StringTable.h>\n\n");
	fprintf(OutFile, "static Agg$StringTable$node __Entries__[%d] = {\n", GTypeModules->Size);
	for (int I = 0; I < GTypeModules->Size; ++I) {
		Agg$StringTable$node *Node = GTypeModules->Entries + I;
		if (Node->Key) {
			fprintf(OutFile, "\t{\"%s\", %d, 0x%x, \"%s\"},\n", Node->Key, Node->Length, Node->Hash, Node->Value);
		} else {
			fprintf(OutFile, "\t{0, 0, 0, 0},\n");
		};
	};
	fprintf(OutFile, "};\n\n");
	fprintf(OutFile, "Agg$StringTable$t Table[] = {Agg$StringTable$T, %d, %d, __Entries__};\n", GTypeModules->Size, GTypeModules->Space);
	fclose(OutFile);
	exit(0);
};
