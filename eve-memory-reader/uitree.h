#pragma once


#include <stdlib.h>
#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include "types.h"
#include "python.h"
#include "hashtable.h"
#include "cache.h"


typedef struct
{
	char* key;
	PythonDictValueRepresentation* value;
} UITreeNodeDictEntry;


typedef struct {
	UITreeNodeDictEntry** data;
	SIZE_T used;
	SIZE_T size;
} UITreeNodeDictEntryList;

typedef struct UITreeNodeBase
{
	ULONGLONG address;
	char* python_object_type_name;
	UITreeNodeDictEntryList* dict_entries_of_interest;
	//char* other_dict_entries[256];
	struct UITreeNodeBase** children;
	ULONGLONG number_of_children;
} UITreeNode;

UITreeNodeDictEntryList* NewUITreeNodeDictEntryList();
UITreeNodeDictEntry* NewUITreeNodeDictEntry(char*, PythonDictValueRepresentation*);
void InitUITreeNodeDictEntryList(UITreeNodeDictEntryList*);
void InsertUITreeNodeDictEntry(UITreeNodeDictEntryList*, UITreeNodeDictEntry*);
void FreeUITreeNodeDictEntry(UITreeNodeDictEntry*);
void FreeUITreeNodeDictEntryList(UITreeNodeDictEntryList*);
void FreeUITreeNode(UITreeNode*);
char* PrintUITreeNodeDictEntryList(UITreeNodeDictEntryList*);
void PrintUITreeNodeDictEntry(UITreeNodeDictEntry*, char*);
char* PrintUITreeNode(UITreeNode*, int);


