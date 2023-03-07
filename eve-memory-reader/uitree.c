#include "uitree.h"

UITreeNodeDictEntryList* NewUITreeNodeDictEntryList()
{
	UITreeNodeDictEntryList* del = malloc(sizeof(UITreeNodeDictEntryList));
	InitUITreeNodeDictEntryList(del);
	return del;
}

UITreeNodeDictEntry* NewUITreeNodeDictEntry(char* key, PythonDictValueRepresentation* value)
{
	UITreeNodeDictEntry* e = malloc(sizeof(UITreeNodeDictEntry));
	if (e == NULL)
	{
		printf("error while allocating memory for UITreeNodeDictEntry\n");
		exit(1);
	}
	e->key = key;
	e->value = value;
	return e;
}

void InitUITreeNodeDictEntryList(UITreeNodeDictEntryList* del)
{
	SIZE_T defaultSize = 256;
	del->data = malloc(sizeof(UITreeNodeDictEntry*) * defaultSize);
	del->used = 0;
	del->size = defaultSize;
}

void InsertUITreeNodeDictEntry(UITreeNodeDictEntryList* del, UITreeNodeDictEntry* e)
{
	if (del->used == del->size)
	{
		del->size *= 2;
		del->data = realloc(del->data, del->size * sizeof(UITreeNodeDictEntry*));
		if (del->data == NULL)
		{
			printf("unable to realloc data array for UITreeNodeDictEntryList\n");
			exit(1);
		}
	}
	if (del->data == NULL)
	{
		printf("data array for UITreeNodeDictEntryList has become corrupted\n");
		exit(1);
	}
	del->data[del->used++] = e;
}

void FreeUITreeNodeDictEntry(UITreeNodeDictEntry* e)
{
	if (e == NULL)
		return;

	if (e->key != NULL)
	{
		free(e->key);
		e->key = NULL;
	}

	FreePythonDictValueRepresentation(e->value);
	free(e);
	e = NULL;
}

void FreeUITreeNodeDictEntryList(UITreeNodeDictEntryList* del)
{
	if (del == NULL)
		return;

	if (del->data != NULL)
	{
		UINT i;
		for (i = 0; i < del->used; ++i)
		{
			FreeUITreeNodeDictEntry(del->data[i]);
		}
		free(del->data);
		del->data = NULL;
	}

	free(del);
	del = NULL;
}

void FreeUITreeNode(UITreeNode* n)
{
	if (n == NULL)
		return;

	if (n->python_object_type_name != NULL)
	{
		free(n->python_object_type_name);
		n->python_object_type_name = NULL;
	}

	if (n->dict_entries_of_interest != NULL)
		FreeUITreeNodeDictEntryList(n->dict_entries_of_interest);

	if (n->children != NULL)
	{
		UINT i;
		for (i = 0; i < n->number_of_children; ++i)
			FreeUITreeNode(n->children[i]);
		free(n->children);
		n->children = NULL;
	}

	free(n);
	n = NULL;
}

char* PrintUITreeNodeDictEntryList(UITreeNodeDictEntryList* del)
{
	char* response = malloc(sizeof(char) * 5000);
	strcpy_s(response, 5000, "");
	if (del == NULL)
		return response;
	UINT i;
	for (i = 0; i < del->used; ++i)
	{
		if (del->data[i] == NULL || del->data[i]->key == NULL || del->data[i]->value == NULL)
			continue;

		// print the key
		sprintf_s(response, 5000, "%s\"%s\": ", response, del->data[i]->key);

		// print the value
		if (del->data[i]->value->is_string)
			sprintf_s(response, 5000, "%s\"%s\",", response, del->data[i]->value->string_value);
		else if (del->data[i]->value->is_unicode)
			sprintf_s(response, 5000, "%s\"%s\",", response, del->data[i]->value->unicode_value);
		else if (del->data[i]->value->is_int)
			sprintf_s(response, 5000, "%s%d,", response, del->data[i]->value->int_value);
		else if (del->data[i]->value->is_float)
			sprintf_s(response, 5000, "%s%f,", response, del->data[i]->value->float_value);
		else if (del->data[i]->value->is_bool)
			sprintf_s(response, 5000, "%s%s,", response, del->data[i]->value->bool_value ? "true" : "false");
		else
			sprintf_s(response, 5000, "%snull,", response);
	}
	if (strlen(response) == 0)
		return response;
	if (response[strlen(response) - 1] == ',')
		response[strlen(response) - 1] = '\0';
	else
		response[strlen(response)] = '\0';
	return response;
}

char* PrintUITreeNode(UITreeNode* n, int level)
{
	size_t size = 500000;
	char* response = malloc(sizeof(char) * size);
	if (n == NULL)
		return response;
	sprintf_s(response, size, "{");
	sprintf_s(response, size, "%s\"address\": %I64u,", response, n->address);
	sprintf_s(response, size, "%s\"type\": \"%s\",", response, n->python_object_type_name);
	sprintf_s(response, size, "%s\"attrs\": {", response);
	if (n->dict_entries_of_interest->used > 0)
	{
		char* dict_response = PrintUITreeNodeDictEntryList(n->dict_entries_of_interest);
		sprintf_s(response, size, "%s%s", response, dict_response);
		free(dict_response);
	}
	sprintf_s(response, size, "%s},", response);
	UINT i;
	sprintf_s(response, size, "%s\"children\": [", response);
	for (i = 0; i < n->number_of_children; ++i)
	{
		if (n->children[i] != NULL)
		{
			char* child_response = PrintUITreeNode(n->children[i], level + 1);
			sprintf_s(response, size, "%s%s", response, child_response);
			free(child_response);
			if (i < n->number_of_children - 1)
				sprintf_s(response, size, "%s,", response);
		}
	}
	sprintf_s(response, size, "%s]}", response);
	if (level == 0)
		sprintf_s(response, size, "%s\n", response);
	return response;
}

