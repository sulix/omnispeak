#ifndef ID_STR_H
#define ID_STR_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
/* String manager, allows objects to be indexed by strings */

typedef struct STR_Entry
{
	const char *str;
	void *ptr;
} STR_Entry;


typedef struct STR_Table
{
	size_t size;
	STR_Entry arr[];
} STR_Table;


void STR_AllocTable(STR_Table **tabl, size_t size);
void* STR_LookupEntry(STR_Table *tabl, const char* str);
bool STR_AddEntry(STR_Table *tabl, const char *str, void *value);


const char *STR_Pool(const char *str);
void STR_UnPool(const char *pooled_str);

#endif //ID_STR_H

