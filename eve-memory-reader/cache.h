#pragma once
#include "hashtable.h"
#include "python.h"

extern HashTable* dict_entry_cache;
extern HashTable* python_type_name_cache;
extern HashTable* python_string_value_cache;

void* dict_entry_cache_copy_fn(void*, size_t);
void dict_entry_cache_free_fn(void*);
void initialize_cache();
