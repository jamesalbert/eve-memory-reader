#include "python.h"

PyDictEntryList* NewPyDictEntryList()
{
	PyDictEntryList* pdel = malloc(sizeof(PyDictEntryList));
	InitPyDictEntryList(pdel);
	return pdel;
}

PyDictEntry* NewPyDictEntry(ULONGLONG hash, ULONGLONG key, ULONGLONG value)
{
	PyDictEntry* e = malloc(sizeof(PyDictEntry));
	if (e == NULL)
	{
		printf("error while allocating memory for PyDictEntry\n");
		exit(1);
	}
	e->hash = hash;
	e->key = key;
	e->value = value;
	return e;
}

void InitPyDictEntryList(PyDictEntryList* pdel)
{
	SIZE_T defaultSize = 256;
	pdel->data = malloc(sizeof(PyDictEntry*) * defaultSize);
	pdel->used = 0;
	pdel->size = defaultSize;
}

void InsertPyDictEntry(PyDictEntryList* pdel, PyDictEntry *e)
{
	if (pdel->used == pdel->size)
	{
		pdel->size *= 2;
		PyDictEntry* tmp = realloc(pdel->data, pdel->size * sizeof(PyDictEntry*));
		if (tmp == NULL)
		{
			printf("unable to realloc data array for PyDictEntryList\n");
			exit(1);
		}
		pdel->data = tmp;
	}
	if (pdel->data == NULL)
	{
		printf("data array for PyDictEntryList has become corrupted\n");
		exit(1);
	}
	pdel->data[pdel->used++] = e;
}

void FreePythonDictValueRepresentation(PythonDictValueRepresentation* p)
{
	if (p == NULL)
		return;
	if (p->python_object_type_name != NULL)
	{
		free(p->python_object_type_name);
		p->python_object_type_name = NULL;
	}
	if (p->is_string && p->string_value != NULL)
	{
		free(p->string_value);
		p->string_value = NULL;
	}
	if (p->is_unicode && p->unicode_value != NULL)
	{
		free(p->unicode_value);
		p->unicode_value = NULL;
	}
	free(p);
	p = NULL;
}

void FreePyDictEntry(PyDictEntry* e)
{
	if (e == NULL)
		return;
	free(e);
	e = NULL;
}

void FreePyDictEntryList(PyDictEntryList* pdel)
{
	if (pdel == NULL)
		return;

	if (pdel->data != NULL)
	{
		UINT i;
		for (i = 0; i < pdel->used; ++i)
		{
			FreePyDictEntry(pdel->data[i]);
		}
		free(pdel->data);
		pdel->data = NULL;
	}

	free(pdel);
	pdel = NULL;
}

void PrintPyDictEntryList(PyDictEntryList* pdel)
{
	printf("PyDictEntryList: length=%d, size=%d\n", pdel->used, pdel->size);
}

