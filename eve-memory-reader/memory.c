#include "memory.h"

void InitAddresses(Addresses* addrs)
{
	SIZE_T defaultSize = 256;
	addrs->data = malloc(sizeof(ULONGLONG) * defaultSize);
	addrs->used = 0;
	addrs->size = defaultSize;
}

void InsertAddress(Addresses* addrs, ULONGLONG addr)
{
	if (addrs->used == addrs->size)
	{
		addrs->size *= 2;
		ULONGLONG* tmp = realloc(addrs->data, addrs->size * sizeof(ULONGLONG));
		if (tmp == NULL)
		{
			printf("unable to realloc data array for Addresses\n");
			exit(1);
		}
		addrs->data = tmp;
	}
	if (addrs->data == NULL)
	{
		printf("data array for Addresses has become corrupted\n");
		exit(1);
	}
	addrs->data[addrs->used++] = addr;
}

BOOL AddressesContains(Addresses* addrs, ULONGLONG compareVal)
{
	UINT i;
	ULONGLONG x = addrs->data[addrs->used - 1];
	if (x == compareVal)
		return TRUE;
	addrs->data[addrs->used - 1] = compareVal;
	for (i = 0; addrs->data[i] != compareVal; i++);
	addrs->data[addrs->used - 1] = x;
	return i != addrs->used - 1;
}


void InsertAddresses(Addresses* to, Addresses* from)
{
	if (to == NULL || from == NULL)
		return;
	UINT i;
	for (i = 0; i < from->used; ++i)
	{
		InsertAddress(to, from->data[i]);
	}
	FreeAddresses(from);
}

void FreeAddresses(Addresses* addrs)
{
	if (addrs == NULL)
		return;

	if (addrs->data != NULL)
	{
		free(addrs->data);
		addrs->data = NULL;
	}

	addrs->used = addrs->size = 0;
	free(addrs);
	addrs = NULL;
}

void PrintAddresses(Addresses* addrs)
{
	printf("Addresses: length=%d, size=%d\n", addrs->used, addrs->size);
}

