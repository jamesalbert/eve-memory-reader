#pragma once
#include <stdlib.h>
#include <windows.h>
#include <winnt.h>
#include "types.h"


typedef struct
{
	ULONGLONG* data;
	SIZE_T used;
	SIZE_T size;
} Addresses;

void InitAddresses(Addresses*);
void InsertAddress(Addresses*, ULONGLONG);
void InsertAddresses(Addresses*, Addresses*);
BOOL AddressesContains(Addresses*, ULONGLONG);
void FreeAddresses(Addresses*);
void PrintAddresses(Addresses*);

