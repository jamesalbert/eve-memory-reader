#include "hashtable.h"

#include <time.h>


ULONGLONG HASHTABLE_MAX = 1000000;

HashTable* NewHashTable()
{
    HashTable* ht = malloc(sizeof(HashTable));
    ht->entries = malloc(sizeof(HashTableEntry*) * HASHTABLE_MAX);
    ht->used = 0;
    UINT i;
    for (i = 0; i < HASHTABLE_MAX; ++i)
        ht->entries[i] = NULL;
    return ht;
}

ULONGLONG hash_code(ULONGLONG key)
{
    return key % HASHTABLE_MAX;
}

void* HashTableFind(HashTable* ht, ULONGLONG key, ULONGLONG* index, void* (*copy_fn)(void*, size_t))
{
    ULONGLONG hash_index = hash_code(key);
    while (ht->entries[hash_index] != NULL)
    {
        if (ht->entries[hash_index]->key == key)
        {
            if ((unsigned long)time(NULL) - ht->entries[hash_index]->ttl > 5)
            {
                free(ht->entries[hash_index]->value);
				ht->entries[hash_index]->value = NULL;
                free(ht->entries[hash_index]);
                ht->entries[hash_index] = NULL;
                ht->used--;
                return NULL;
            }
            else
            {
                void* value = ht->entries[hash_index]->value;
                size_t size = ht->entries[hash_index]->size;
				*index = hash_index;
                if (copy_fn == NULL)
                {
					void* copy = malloc(size);
					memcpy_s(copy, size, value, size);
					return copy;
                }
                else
                {
					return copy_fn(ht->entries[hash_index]->value, ht->entries[hash_index]->size);
                }
            }
        }
        ++hash_index;
        hash_index %= HASHTABLE_MAX;
    }
    return NULL;
}

void HashTableInsert(HashTable* ht, ULONGLONG key, void* value, size_t size, void* (*copy_fn)(void*, size_t))
{
    HashTableEntry* entry = malloc(sizeof(HashTableEntry));
    entry->key = key;
    if (copy_fn == NULL)
    {
		entry->value = malloc(size);
		memcpy_s(entry->value, size, value, size);
    }
    else
    {
        entry->value = copy_fn(value, size);
    }
    entry->ttl = (unsigned long)time(NULL);
    entry->size = size;
    ULONGLONG hash_index = hash_code(key);
    while (ht->entries[hash_index] != NULL)
    {
        ++hash_index;
        hash_index %= HASHTABLE_MAX;
    }
    ht->used++;
    ht->entries[hash_index] = entry;
}

int HashTableFlushStale(HashTable* ht, void (*free_fn)(void*))
{
    if (ht == NULL || ht->entries == NULL)
        return 0;
    int flushed = 0;
    UINT i;
    for (i = 0; i < HASHTABLE_MAX; ++i)
    {
        if (ht->entries[i] == NULL)
            continue;
        if ((unsigned long)time(NULL) - ht->entries[i]->ttl > 5)
        {
            if (free_fn == NULL)
            {
				free(ht->entries[i]->value);
				ht->entries[i]->value = NULL;
            }
            else
            {
                free_fn(ht->entries[i]->value);
            }
			free(ht->entries[i]);
			ht->entries[i] = NULL;
			ht->used--;
            flushed++;
        }
    }
    return flushed;
}

int HashTableFlush(HashTable* ht, void (*free_fn)(void*))
{
    int flushed = 0;
    UINT i;
    for (i = 0; i < HASHTABLE_MAX; ++i)
    {
        if (ht->entries[i] == NULL)
            continue;
		if (free_fn == NULL)
		{
			free(ht->entries[i]->value);
			ht->entries[i]->value = NULL;
		}
		else
		{
			free_fn(ht->entries[i]->value);
		}
		free(ht->entries[i]);
		ht->entries[i] = NULL;
		ht->used--;
		flushed++;
    }
    return flushed;
}
