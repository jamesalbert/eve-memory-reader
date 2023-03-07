#pragma once
#include <stdlib.h>
#include <windows.h>
#include <winnt.h>
#include <malloc.h>


typedef struct
{
	ULONGLONG key;
	void* value;
	ULONGLONG ttl;
	size_t size;
} HashTableEntry;

typedef struct
{
	HashTableEntry** entries;
	ULONGLONG used;
} HashTable;

extern ULONGLONG HASHTABLE_MAX;

HashTable* NewHashTable();
ULONGLONG hash_code(ULONGLONG);
void* HashTableFind(HashTable*, ULONGLONG, ULONGLONG*, void* (*copy_fn)(void*, size_t));
void HashTableInsert(HashTable*, ULONGLONG, void*, size_t, void* (*copy_fn)(void*, size_t));
int HashTableFlushStale(HashTable*, void (*free_fn)(void*));
int HashTableFlush(HashTable*, void (*free_fn)(void*));
