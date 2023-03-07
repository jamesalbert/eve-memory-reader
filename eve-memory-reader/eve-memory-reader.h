#pragma once
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <winnt.h>
#include <string.h>
#include <time.h>
#include "memory.h"
#include "process.h"
#include "python.h"
#include "uitree.h"
#include "hashtable.h"
#include "cache.h"


extern ProcessSample* ps;
extern ULONGLONG primary_ui_root;
extern Addresses* ui_roots;
extern HANDLE hProcess;
extern ULONGLONG cache_last_flushed;
extern char* ui_json;

__declspec(dllexport) void read_ui_trees();
__declspec(dllexport) char* get_ui_json();
__declspec(dllexport) void free_ui_json();
__declspec(dllexport) int initialize();
__declspec(dllexport) void free_string(char*);
__declspec(dllexport) void cleanup();
UITreeNode* read_ui_tree_from_address(ULONGLONG, int);
PythonDictValueRepresentation* get_dict_entry_value_representation(ULONGLONG);
