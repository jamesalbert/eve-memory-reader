#pragma once
#include <windows.h>
#include <winnt.h>
#include <psapi.h>
#include "types.h"


void get_process_name_from_pid(DWORD, char**);
DWORD get_pid(char*);

typedef struct
{
	ULONGLONG address;
	ULONGLONG region_size;
	byte* content;
} CommittedRegion;

typedef struct
{
	CommittedRegion** committed_regions;
	SIZE_T used;
	SIZE_T size;
} ProcessSample;

void InitProcessSample(ProcessSample*);
void InsertCommittedRegion(ProcessSample*, CommittedRegion*);
void FreeProcessSample(ProcessSample*);
void PrintProcessSample(ProcessSample*);

