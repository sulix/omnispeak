/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2023 Omnispeak Authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "../../src/id_mm.h"
#include "../../src/id_str.h"
#include "../../src/ck_act.h"

bool CA_LoadFile(const char *filename, mm_ptr_t *ptr, int *memsize)
{
	FILE *f = fopen(filename, "r");

	if (!f)
		return false;

	//Get length of file
	fseek(f, 0, SEEK_END);
	int length = ftell(f);
	fseek(f, 0, SEEK_SET);

	MM_GetPtr(ptr, length);

	if (memsize)
		*memsize = length;

	int amountRead = fread(*ptr, 1, length, f);

	fclose(f);

	if (amountRead != length)
		return false;
	return true;
}

void Quit(const char *msg)
{
	if (msg)
	{
		printf(msg);
		exit(1);
	}
	exit(0);
}

extern STR_Table *ck_varTable;
extern ID_MM_Arena *ck_varArena;
extern CK_action *ck_actionData;
extern int ck_actionsUsed;

typedef enum CK_VAR_VarType
{
	VAR_Invalid,
	VAR_EOF,
	VAR_Bool,
	VAR_Int,
	VAR_String,
	VAR_Action,
	VAR_TOK_Include
} CK_VAR_VarType;

#ifdef CK_VAR_TYPECHECK
typedef struct CK_VAR_Variable
{
	CK_VAR_VarType type;
	void *value;
} CK_VAR_Variable;
#endif


void PrintQuotedString(FILE *f, const char *str)
{
	fprintf(f, "\"");
	for (char c = *str; *str != '\0'; c = *(++str))
	{
		switch (c)
		{
		case '\n':
			if (*(str+1))
				fprintf(f, "\\n\" \\\n\t\"");
			else
				fprintf(f, "\\n");
			break;
		case '\t':
			fprintf(f, "\\t");
			break;
		case '\\':
			fprintf(f, "\\\\");
			break;
		case '\"':
			fprintf(f, "\\\"");
			break;
		default:
			fprintf(f, "%c", c);
			break;
		}
	}
	fprintf(f, "\"");
}

// Global options
bool staticStrings = false;	/* Strings are generated as variables if true, defines otherwise */
const char *prefix = "";	/* Non-action variable prefix. */
int maxVarNameLen = 30;		/* Warn if a variable name exceeds this length. */


void OutputHeaderVars(FILE *outfile)
{
	size_t index = 0;
	CK_VAR_Variable *currentVar = NULL;
	/* Prototypes for action structs, so ->next works. */
	while ((currentVar = (CK_VAR_Variable *)STR_GetNextEntry(ck_varTable, &index)))
	{
		int varNameLen = strlen(ck_varTable->arr[index-1].str) + strlen(prefix);
		if (varNameLen > maxVarNameLen)
			printf("/* Var '%s' has length %d */\n", ck_varTable->arr[index-1].str, varNameLen);
		if (currentVar->type == VAR_Action)
		{
			fprintf(outfile, "extern CK_action %s;\n", ck_varTable->arr[index-1].str);
		}
		else if (currentVar->type == VAR_String)
		{
			fprintf(outfile, "#define %sSTRING_%s ", prefix, ck_varTable->arr[index-1].str);
			PrintQuotedString(outfile, (const char *)currentVar->value);
			fprintf(outfile, "\n");
		}
		else
			fprintf(outfile, "#define %sINT_%s %d\n", prefix, ck_varTable->arr[index-1].str, currentVar->value);
	}

}

void OutputStaticStrings(FILE *outfile)
{
	size_t index = 0;
	CK_VAR_Variable *currentVar = NULL;
	/* Prototypes for action structs, so ->next works. */
	while ((currentVar = (CK_VAR_Variable *)STR_GetNextEntry(ck_varTable, &index)))
	{
		if (currentVar->type == VAR_String)
		{
			fprintf(outfile, "const char *%sSTRING_%s = ", prefix, ck_varTable->arr[index-1].str);
			PrintQuotedString(outfile, (const char *)currentVar->value);
			fprintf(outfile, ";\n");
		}
	}

}
void OutputActionsDOS16(FILE *outfile)
{
	size_t index = 0;
	CK_VAR_Variable *currentVar = NULL;
	while ((currentVar = (CK_VAR_Variable *)STR_GetNextEntry(ck_varTable, &index)))
	{
		if (currentVar->type == VAR_Action)
		{
			CK_action *act = (CK_action *)currentVar->value;
			fprintf(outfile, "CK_action %s = {%d, %d, %d, %d, %d, %d, %d, %d, %s, %s, %s, %c%s};\n",
				ck_varTable->arr[index-1].str,
				act->chunkLeft,
				act->chunkRight,
				act->type,
				act->protectAnimation, act->stickToGround,
				act->timer,
				act->velX, act->velY,
				act->think,
				act->collide,
				act->draw,
				strcmp((const char *)act->next, "NULL") ? '&' : ' ',
				act->next
			);
		}
	}
}

void OutputActionsOmnispeak(FILE *outfile)
{
	size_t index = 0;
	CK_VAR_Variable *currentVar = NULL;
	while ((currentVar = (CK_VAR_Variable *)STR_GetNextEntry(ck_varTable, &index)))
	{
		if (currentVar->type == VAR_Action)
		{
			CK_action *act = (CK_action *)currentVar->value;
			fprintf(outfile, "CK_action %s = {%d, %d, %d, %d, %d, %d, %d, %d, %s, %s, %s, %c%s, 0x%X};\n",
				ck_varTable->arr[index-1].str,
				act->chunkLeft,
				act->chunkRight,
				act->type,
				act->protectAnimation, act->stickToGround,
				act->timer,
				act->velX, act->velY,
				act->think,
				act->collide,
				act->draw,
				strcmp((const char *)act->next, "NULL") ? '&' : ' ',
				act->next,
				act->compatDosPointer
			);
		}
	}
}

void OutputDOS16ActionCompat(FILE *outfile)
{
	size_t index = 0;
	CK_VAR_Variable *currentVar = NULL;
	fprintf(outfile, "typedef struct CK_ActionCompatPtr {\n");
	fprintf(outfile, "\tuint16_t dosPtr;\n");
	fprintf(outfile, "\tCK_action *realPtr;\n");
	fprintf(outfile, "} CK_ActionCompatPtr;\n");
	fprintf(outfile, "CK_ActionCompatPtr CompatPtrs[] = {\n");
	while ((currentVar = (CK_VAR_Variable *)STR_GetNextEntry(ck_varTable, &index)))
	{
		if (currentVar->type == VAR_Action)
		{
			CK_action *act = (CK_action *)currentVar->value;
			fprintf(outfile, "\t{0x%04X, &%s},\n", act->compatDosPointer, ck_varTable->arr[index-1].str);
		}
	}
	fprintf(outfile, "\t{0, NULL}\n");
	fprintf(outfile, "}\n\n");
}

int main(int argc, char **argv)
{
	MM_Startup();
	CK_VAR_Startup();

	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "--in"))
		{
			CK_VAR_LoadVars(argv[++i]);
		}
		else if (!strcmp(argv[i], "--header"))
		{
			FILE *f = fopen(argv[++i], "w");
			OutputHeaderVars(f);
			fclose(f);
		}
		else if (!strcmp(argv[i], "--dos16-actions"))
		{
			FILE *f = fopen(argv[++i], "w");
			OutputActionsDOS16(f);
			OutputDOS16ActionCompat(f);
			fclose(f);
		}
		else if (!strcmp(argv[i], "--actions"))
		{
			FILE *f = fopen(argv[++i], "w");
			OutputActionsOmnispeak(f);
			fclose(f);
		}
		else if (!strcmp(argv[i], "--static-strings"))
		{
			staticStrings = true;
		}
		else if (!strcmp(argv[i], "--prefix"))
		{
			prefix = argv[++i];
		}
		else
		{
			printf("%s: Convert Omnispeak variables to C headers\n\n", argv[0]);
			printf("Usage: %s --in EPISODE.CKx --header episode.h --actions episode.c\n");
			printf("\tOptions are processed in order, so --in must come before --header, --actions.\n");
			printf("\tActions compatible with 16-bit DOS versions can be made with --dos16-actions.\n");
			return -1;
		}
	}
}

