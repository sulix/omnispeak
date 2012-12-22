#include "id_mm.h"
#include "id_str.h"

#include <stdlib.h>
#include <stdio.h>

/* String manager, allows objects to be indexed by strings */

// Hash a string using an xor variant of the djb2 hash
static unsigned int STR_HashString(const char *str)
{
	int slen = strlen(str);
	unsigned int hash = 0;
	for (int i = 0; i < slen; ++i)
	{
		hash = ((hash << 5) + hash) ^ (unsigned int)(str[i]);
	}
	return hash;
}


// Allocate a table 'tabl' of size 'size'
void STR_AllocTable(STR_Table **tabl, size_t size)
{
	MM_GetPtr((mm_ptr_t*)(tabl), sizeof(STR_Table) + size*(sizeof(STR_Entry)));
	// Lock it in memory so that it doesn't get purged.
	MM_SetLock((mm_ptr_t*)(tabl), true);
	(*tabl)->size = size;
	for (size_t i = 0; i < size; ++i)
	{
		(*tabl)->arr[i].str = 0;
		(*tabl)->arr[i].ptr = 0;
	}
}

// Returns the pointer associated with 'str' in 'tabl'
void* STR_LookupEntry(STR_Table *tabl, const char* str)
{
	int hash = STR_HashString(str) % tabl->size;
	for (size_t i = hash; ; i = (i+1)%tabl->size)
	{
		if (tabl->arr[i].str == 0) break;
		if (strcmp(str, tabl->arr[i].str) == 0)
		{
			return (tabl->arr[i].ptr);
		}
	}
	return (void*)(0);
}

// Add an entry 'str' with pointer 'value' to 'tabl'. Returns 'true' on success
bool STR_AddEntry(STR_Table *tabl, const char *str, void *value)
{
	int hash = STR_HashString(str) % tabl->size;
	for (size_t i = hash; ; i = (i+1)%tabl->size)
	{
		if (tabl->arr[i].str == 0)
		{
			tabl->arr[i].str = str;
			tabl->arr[i].ptr = value;
			return true;
		}
	}
	return false;
}

/*
 * One day, this will be a glorious string pool, saving memory and generally being awesome.
 * For now, it's a crappy malloc wrapper.
 */
const char *STR_Pool(const char *str)
{
	return strdup(str);
}

void STR_UnPool(const char *pooled_str)
{
	free((void*)pooled_str);
}


