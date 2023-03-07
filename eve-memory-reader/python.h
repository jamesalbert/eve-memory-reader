#pragma once

#include <stdlib.h>
#include <windows.h>
#include <winnt.h>
#include "types.h"


typedef struct {
	ULONGLONG address;
	char* python_object_type_name;
	BOOL is_string;
	BOOL is_unicode;
	BOOL is_int;
	BOOL is_bool;
	BOOL is_float;
	BOOL is_pycolor;
	BOOL is_bunch;
	char* string_value;
	char* unicode_value;
	INT int_value;
	BOOL bool_value;
	FLOAT float_value;
} PythonDictValueRepresentation;

typedef struct {
	ULONGLONG hash;
	ULONGLONG key;
	char* key_resolved;
	ULONGLONG value;
} PyDictEntry;

typedef struct {
	PyDictEntry** data;
	SIZE_T used;
	SIZE_T size;
} PyDictEntryList;

PyDictEntryList* NewPyDictEntryList();
PyDictEntry* NewPyDictEntry(ULONGLONG, ULONGLONG, ULONGLONG);
void InitPyDictEntryList(PyDictEntryList*);
void InsertPyDictEntry(PyDictEntryList*, PyDictEntry*);
void FreePythonDictValueRepresentation(PythonDictValueRepresentation*);
void FreePyDictEntry(PyDictEntry*);
void FreePyDictEntryList(PyDictEntryList*);
void PrintPyDictEntryList(PyDictEntryList*);
void PrintPyDictEntry(PyDictEntry*);


