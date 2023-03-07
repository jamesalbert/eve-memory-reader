#include "cache.h"

HashTable* dict_entry_cache;
HashTable* python_type_name_cache;
HashTable* python_string_value_cache;

void initialize_cache()
{
	dict_entry_cache = NewHashTable();
	python_type_name_cache = NewHashTable();
	python_string_value_cache = NewHashTable();
}

void* dict_entry_cache_copy_fn(void* vvalue, size_t size)
{
	PythonDictValueRepresentation* value = (PythonDictValueRepresentation*)vvalue;
	PythonDictValueRepresentation* repr = malloc(size);
	repr->address = value->address;
	if (value->python_object_type_name != NULL)
	{
		const size_t length = strlen(value->python_object_type_name);
		repr->python_object_type_name = malloc(length + 1);
		memcpy_s(repr->python_object_type_name, length + 1, value->python_object_type_name, length + 1);
	}
	else
	{
		repr->python_object_type_name = NULL;
	}
	repr->is_string = value->is_string;
	repr->is_unicode = value->is_unicode;
	repr->is_int = value->is_int;
	repr->is_float = value->is_float;
	repr->is_bool = value->is_bool;
	repr->is_pycolor = value->is_pycolor;
	repr->is_bunch = value->is_bunch;
	if (value->is_string && value->string_value != NULL)
	{
		const size_t length = strlen(value->string_value);
		repr->string_value = malloc(length + 1);
		memcpy_s(repr->string_value, length + 1, value->string_value, length + 1);
	}
	else
	{
		repr->string_value = NULL;
	}
	if (value->is_unicode && value->unicode_value != NULL)
	{
		const size_t length = strlen(value->unicode_value);
		repr->unicode_value = malloc(length + 1);
		memcpy_s(repr->unicode_value, length + 1, value->unicode_value, length + 1);
	}
	else
	{
		repr->unicode_value = NULL;
	}
	repr->int_value = value->int_value;
	repr->bool_value = value->bool_value;
	repr->float_value = value->float_value;
	return repr;
}

void dict_entry_cache_free_fn(void* vvalue)
{
	PythonDictValueRepresentation* value = (PythonDictValueRepresentation*)vvalue;
	if (value->python_object_type_name != NULL)
	{
		free(value->python_object_type_name);
		value->python_object_type_name = NULL;
	}
	if (value->string_value != NULL)
	{
		free(value->string_value);
		value->string_value = NULL;
	}
	if (value->unicode_value != NULL)
	{
		free(value->unicode_value);
		value->unicode_value = NULL;
	}
}
