#include <Riva.h>
#include <Std/Object.h>
#include <Riva/System.h>
#include <Agg/StringTable.h>
#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>

INITIAL() {
	FILE *OutFile = fopen("Entities.c", "w");
	fprintf(OutFile, "#include <Std/String.h>\n");
	fprintf(OutFile, "#include <Agg/StringTable.h>\n\n");

	Agg$StringTable_t ByName[] = {Agg$StringTable$INIT};
	json_error_t JsonError;
	json_t *EntitiesJson = json_load_file(Riva$System$_Args[0], 0, &JsonError);
	const char *EntityName;
	json_t *EntityValue;
	int J = 0;
	json_object_foreach(EntitiesJson, EntityName, EntityValue) {
		json_t *CodePoints = json_object_get(EntityValue, "codepoints");
		char Buffer[128], *P = Buffer;
		for (int I = 0; I < json_array_size(CodePoints); ++I) {
			int CodePoint = json_integer_value(json_array_get(CodePoints, I));
			if (CodePoint <= 0x7F) {
				P += sprintf(P, "\\x%02x", CodePoint);
			} else if (CodePoint <= 0x7FF) {
				int Byte1 = 0xC0 | (CodePoint >> 6);
				int Byte2 = 0x80 | (CodePoint & 0x3F);
				P += sprintf(P, "\\x%02x\\x%02x", Byte1, Byte2);
			} else if (CodePoint <= 0xFFFF) {
				int Byte1 = 0xE0 | (CodePoint >> 12);
				int Byte2 = 0x80 | ((CodePoint >> 6) & 0x3F);
				int Byte3 = 0x80 | (CodePoint & 0x3F);
				P += sprintf(P, "\\x%02x\\x%02x\\x%02x", Byte1, Byte2, Byte3);
			} else if (CodePoint <= 0x10FFFF) {
				int Byte1 = 0xF0 | (CodePoint >> 18);
				int Byte2 = 0x80 | ((CodePoint >> 12) & 0x3F);
				int Byte3 = 0x80 | ((CodePoint >> 6) & 0x3F);
				int Byte4 = 0x80 | (CodePoint & 0x3F);
				P += sprintf(P, "\\x%02x\\x%02x\\x%02x\\%x02x", Byte1, Byte2, Byte3, Byte4);
			} else {
				P += sprintf(P, "\\U%08x", CodePoint);
			}
		}
		fprintf(OutFile, "STRING(String%d, \"%s\");\n", J, Buffer);
		Agg$StringTable$put(ByName, Riva$Memory$strdup(EntityName), strlen(EntityName), (void *)J);
		++J;
	}

	fprintf(OutFile, "static Agg$StringTable_node __Entries__[%d] = {\n", ByName->Size);
	for (int I = 0; I < ByName->Size; ++I) {
		Agg$StringTable_node *Node = ByName->Entries + I;
		if (Node->Key) {
			fprintf(OutFile, "\t{\"%s\", %d, 0x%x, String%d},\n", Node->Key, Node->Length, Node->Hash, (int)Node->Value);
		} else {
			fprintf(OutFile, "\t{0, 0, 0, 0},\n");
		};
	};
	fprintf(OutFile, "};\n\n");
	fprintf(OutFile, "Agg$StringTable_t ByName[] = {Agg$StringTable$T, %d, %d, __Entries__};\n", ByName->Size, ByName->Space);
	fclose(OutFile);
	exit(0);
};
