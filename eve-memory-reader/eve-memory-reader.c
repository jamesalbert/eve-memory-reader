#include "eve-memory-reader.h"


#define PROCESS_NAME "exefile.exe"

ProcessSample* ps;
ULONGLONG primary_ui_root;
Addresses* ui_roots;
ULONGLONG number_of_nodes = 0;
ULONGLONG cache_last_flushed = 0;
char* ui_json = NULL;
HANDLE hProcess;

char* DICT_KEYS_OF_INTEREST[25] = {
	"_top", "_left", "_width", "_height", "_displayX", "_displayY",
	"_displayHeight", "_displayWidth",
	"_name", "_text", "_setText",
	"children",
	"texturePath", "_bgTexturePath",
	"_hint", "_display",

	//  HPGauges
	"lastShield", "lastArmor", "lastStructure",

	//  Found in "ShipHudSpriteGauge"
	"_lastValue",

	//  Found in "ModuleButton"
	"ramp_active",

	//  Found in the Transforms contained in "ShipModuleButtonRamps"
	"_rotation",

	//  Found under OverviewEntry in Sprite named "iconSprite"
	"_color",

	//  Found in "SE_TextlineCore"
	"_sr",

	//  Found in "_sr" Bunch
	"htmlstr"
};

ULONGLONG find_min(const ULONGLONG* arr, UINT length) {
	// returns the minimum value of array
	size_t i;
	ULONGLONG minimum = arr[0];
	for (i = 1; i < length; ++i) {
		if (minimum > arr[i]) {
			minimum = arr[i];
		}
	}
	return minimum;
}

ULONGLONG find_max(const ULONGLONG* arr, UINT length) {
	// returns the minimum value of array
	size_t i;
	ULONGLONG maximum = arr[0];
	for (i = 1; i < length; ++i) {
		if (maximum < arr[i]) {
			maximum = arr[i];
		}
	}
	return maximum;
}

void read_committed_memory_regions_from_pid(DWORD pid, BOOL with_content)
{
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	ULONGLONG lpAddress = 0x0;
	while (TRUE)
	{
		MEMORY_BASIC_INFORMATION64 lpBuffer;
		SIZE_T result = VirtualQueryEx(hProcess, lpAddress, &lpBuffer, sizeof(MEMORY_BASIC_INFORMATION64));

		if (lpAddress == lpBuffer.BaseAddress + lpBuffer.RegionSize)
		{
			break;
		}

		lpAddress = lpBuffer.BaseAddress + lpBuffer.RegionSize;

		if (lpBuffer.State != MEM_COMMIT)
		{
			continue;
		}

		DWORD protectionsFlagsToSkip = PAGE_GUARD | PAGE_NOACCESS;
		DWORD matchingFlagsToSkip = protectionsFlagsToSkip & lpBuffer.Protect;

		if (matchingFlagsToSkip != 0)
		{
			continue;
		}

		byte* regionContent = NULL;

		if (with_content)
		{
			ULONGLONG bytesRead = 0;
			byte* regionContentBuffer = malloc(sizeof(byte) * lpBuffer.RegionSize);
			if (!ReadProcessMemory(hProcess, lpBuffer.BaseAddress, regionContentBuffer, lpBuffer.RegionSize, &bytesRead))
			{
				if (regionContentBuffer != NULL)
				{
					free(regionContentBuffer);
					regionContentBuffer = NULL;
				}
				exit(1);
			}

			if (bytesRead != lpBuffer.RegionSize)
			{
				if (regionContentBuffer != NULL)
				{
					free(regionContentBuffer);
					regionContentBuffer = NULL;
				}
				// TODO: this is recoverable, just try again
				printf("Failed to ReadProcessMemory at 0x%llx, only read %lld bytes; expecting %lld bytes.", lpBuffer.BaseAddress, bytesRead, lpBuffer.RegionSize);
				exit(1);
			}

			regionContent = regionContentBuffer;
		}

		CommittedRegion* cr = malloc(sizeof(CommittedRegion));
		if (cr == NULL)
		{
			printf("error creating CommittedRegion");
			exit(1);
		}
		cr->address = lpBuffer.BaseAddress;
		cr->region_size = lpBuffer.RegionSize;
		cr->content = regionContent;
		InsertCommittedRegion(ps, cr);
	}
}

void get_process_sample_from_pid(DWORD pid)
{
	ps = malloc(sizeof(ProcessSample));
	InitProcessSample(ps);
	read_committed_memory_regions_from_pid(pid, TRUE);
}

byte* slice_byte_array(byte* original, ULONGLONG size, UINT start, UINT end)
{
	byte* sliced = malloc(sizeof(byte) * size);
	UINT bytes_sliced = 0;
	UINT i;
	for (i = 0; i < size; ++i)
		if (start <= i && i < end)
			sliced[bytes_sliced++] = original[i];
	return sliced;
}

ULONGLONG cast_byte_array_to_ulong(byte* content, ULONGLONG size)
{
	ULONGLONG result = 0;
	UINT i;
	for (i = 0; i < size; ++i)
		result |= ((ULONGLONG)content[i]) << (i * 8);
	return result;
}

ULONGLONG* cast_byte_array_to_ulong_array(byte* content, ULONGLONG size)
{
	ULONGLONG* result = malloc(sizeof(ULONGLONG) * (size/sizeof(ULONGLONG)));
	UINT ulongs_created = 0;
	UINT i;
	for (i = 0; i < size; i += 8)
	{
		result[ulongs_created++] = (ULONGLONG)(
			(ULONGLONG)content[i] |
			((ULONGLONG)content[i+1] << 8) |
			((ULONGLONG)content[i+2] << 16) |
			((ULONGLONG)content[i+3] << 24) |
			((ULONGLONG)content[i+4] << 32) |
			((ULONGLONG)content[i+5] << 40) |
			((ULONGLONG)content[i+6] << 48) |
			((ULONGLONG)content[i+7] << 56)
		);
	}
	return result;
}

byte* read_bytes(ULONGLONG address, ULONGLONG length, ULONGLONG* bytes_read)
{

	byte* regionContentBuffer = malloc(sizeof(byte) * length);
	if (!ReadProcessMemory(hProcess, address, regionContentBuffer, _msize(regionContentBuffer), bytes_read))
	{
		if (regionContentBuffer != NULL)
		{
			free(regionContentBuffer);
			regionContentBuffer = NULL;
		}
		return NULL;
	}

	if (regionContentBuffer == NULL)
		return NULL;

	if (bytes_read == 0)
	{
		free(regionContentBuffer);
		regionContentBuffer = NULL;
		return NULL;
	}

	if (ULLONG_MAX < bytes_read)
	{
		free(regionContentBuffer);
		regionContentBuffer = NULL;
		return NULL;
	}

	return regionContentBuffer;
}

char* read_string_from_address(ULONGLONG address)
{
	ULONGLONG bytes_read = 0;
	byte* as_memory = read_bytes(address, 0x100, &bytes_read);

	if (as_memory == NULL)
		return NULL;

	UINT length = 0;
	UINT i = 0;
	do
		if (as_memory[++length] == 0)
			break;
	while (i++ < 0x100);
	byte* null_terminated = slice_byte_array(as_memory, 0x100, 0, length);
	char* response = malloc(sizeof(char) * length + 1);
	memcpy_s(response, length, as_memory, length);
	response[length] = '\0';
	free(null_terminated);
	free(as_memory);
	null_terminated = NULL;
	as_memory = NULL;
	return response;
}

ULONGLONG* read_memory_region_content_as_ulong_array(CommittedRegion* cr)
{
	ULONGLONG bytes_read = 0;
	byte* as_byte_array = read_bytes(cr->address, cr->region_size, &bytes_read);

	if (as_byte_array == NULL)
		return NULL;

	ULONGLONG* response = cast_byte_array_to_ulong_array(as_byte_array, cr->region_size);
	free(as_byte_array);
	as_byte_array = NULL;
	return response;
}

Addresses* find_python_type_object_type_candidates_within_region(CommittedRegion* cr)
{
	Addresses* candidates = malloc(sizeof(Addresses));
	InitAddresses(candidates);
	ULONGLONG* memory_region_ulong = read_memory_region_content_as_ulong_array(cr);
	if (memory_region_ulong == NULL)
		return;

	UINT candidate_address_index;
	for (candidate_address_index = 0; candidate_address_index < (cr->region_size / 8) - 4; ++candidate_address_index)
	{
		ULONGLONG candidate_address_in_process = cr->address + (ULONGLONG)candidate_address_index * 8;
		ULONGLONG candidate_ob_type = memory_region_ulong[candidate_address_index + 1];

		if (candidate_ob_type != candidate_address_in_process)
			continue;

		char* candidate_tp_name = read_string_from_address(memory_region_ulong[candidate_address_index + 3]);

		// check if candidate_tp_name == "type"
		if (candidate_tp_name == NULL)
			continue;
		if (strcmp(candidate_tp_name, "type") != 0)
		{
			free(candidate_tp_name);
			candidate_tp_name = NULL;
			continue;
		}

		InsertAddress(candidates, candidate_address_in_process);
		if (candidates->used > 0)
		{
			free(candidate_tp_name);
			candidate_tp_name = NULL;
			break;
		}
	}
	free(memory_region_ulong);
	memory_region_ulong = NULL;
	return candidates;
}

Addresses* find_python_type_object_type_candidates()
{
	Addresses* candidates = malloc(sizeof(Addresses));
	InitAddresses(candidates);
	unsigned int i;
	for (i = 0; i < ps->used; ++i)
	{
		InsertAddresses(candidates, find_python_type_object_type_candidates_within_region(ps->committed_regions[i]));
		if (candidates->used > 0)
			break;
	}
	return candidates;
}

Addresses* find_python_type_objects_candidates_within_region(CommittedRegion* cr, Addresses* previous_candidates, ULONGLONG min_candidate, ULONGLONG max_candidate)
{
	Addresses* candidates = malloc(sizeof(Addresses));
	InitAddresses(candidates);
	ULONGLONG* memory_region_ulong = read_memory_region_content_as_ulong_array(cr);
	if (memory_region_ulong == NULL)
		return;

	UINT candidate_address_index;
	for (candidate_address_index = 0; candidate_address_index < (cr->region_size / 8) - 4; ++candidate_address_index)
	{
		ULONGLONG candidate_address_in_process = cr->address + (ULONGLONG)candidate_address_index * 8;
		ULONGLONG candidate_ob_type = memory_region_ulong[candidate_address_index + 1];

		if (candidate_ob_type < min_candidate || max_candidate < candidate_ob_type)
		{
			continue;
		}

		if (!AddressesContains(previous_candidates, candidate_ob_type))
		{
			continue;
		}

		char* candidate_tp_name = read_string_from_address(memory_region_ulong[candidate_address_index + 3]);


		if (candidate_tp_name == NULL)
			continue;
		if (strcmp(candidate_tp_name, "UIRoot") != 0)
		{
			free(candidate_tp_name);
			candidate_tp_name = NULL;
			continue;
		}

		InsertAddress(candidates, candidate_address_in_process);
		if (candidates->used > 0)
		{
			free(candidate_tp_name);
			candidate_tp_name = NULL;
			break;
		}
	}
	free(memory_region_ulong);
	memory_region_ulong = NULL;
	return candidates;
}

Addresses* find_python_type_objects_candidates(Addresses* previous_candidates)
{
	if (previous_candidates->used < 1)
		return NULL;

	Addresses* candidates = malloc(sizeof(Addresses));
	InitAddresses(candidates);
	ULONGLONG min_candidate = find_min(previous_candidates->data, previous_candidates->used);
	ULONGLONG max_candidate = find_max(previous_candidates->data, previous_candidates->used);
	unsigned int i;
	for (i = 0; i < ps->used; ++i)
	{
		InsertAddresses(candidates, find_python_type_objects_candidates_within_region(ps->committed_regions[i], previous_candidates, min_candidate, max_candidate));
		if (candidates->used > 0)
			break;
	}
	return candidates;
}

Addresses* find_python_type_candidates_within_region(CommittedRegion* cr, Addresses* previous_candidates, ULONGLONG min_candidate, ULONGLONG max_candidate)
{
	Addresses* candidates = malloc(sizeof(Addresses));
	InitAddresses(candidates);
	ULONGLONG* memory_region_ulong = read_memory_region_content_as_ulong_array(cr);
	if (memory_region_ulong == NULL)
		return candidates;

	UINT candidate_address_index;
	for (candidate_address_index = 0; candidate_address_index < (cr->region_size / 8) - 4; ++candidate_address_index)
	{
		ULONGLONG candidate_address_in_process = cr->address + (ULONGLONG)candidate_address_index * 8;
		ULONGLONG candidate_ob_type = memory_region_ulong[candidate_address_index + 1];

		if (candidate_ob_type < min_candidate || max_candidate < candidate_ob_type)
		{
			continue;
		}

		if (!AddressesContains(previous_candidates, candidate_ob_type))
		{
			continue;
		}

		InsertAddress(candidates, candidate_address_in_process);
	}
	free(memory_region_ulong);
	memory_region_ulong = NULL;
	return candidates;
}

Addresses* find_python_type_candidates(Addresses* previous_candidates)
{
	if (previous_candidates->used < 1)
		return NULL;

	Addresses* candidates = malloc(sizeof(Addresses));
	InitAddresses(candidates);
	ULONGLONG min_candidate = find_min(previous_candidates->data, previous_candidates->used);
	ULONGLONG max_candidate = find_max(previous_candidates->data, previous_candidates->used);
	unsigned int i;
	for (i = 0; i < ps->used; ++i)
	{
		InsertAddresses(candidates, find_python_type_candidates_within_region(ps->committed_regions[i], previous_candidates, min_candidate, max_candidate));
	}
	return candidates;
}

Addresses* find_ui_roots()
{
	Addresses* ptot_candidates = find_python_type_object_type_candidates();
	Addresses* pto_candidates = find_python_type_objects_candidates(ptot_candidates);
	Addresses* pt_candidates = find_python_type_candidates(pto_candidates);
	return pt_candidates;
}

Addresses* get_ui_roots_from_process_sample_file()
{
	return find_ui_roots();
}

BOOL bytes_array_contains(byte* bytes, ULONGLONG size, ULONGLONG compareVal)
{
	if (bytes == NULL) return FALSE;
	UINT i;
	ULONGLONG x = bytes[size - 1];
	if (x == compareVal)
		return TRUE;
	bytes[size - 1] = compareVal;
	for (i = 0; bytes[i] != compareVal; i++);
	bytes[size - 1] = x;
	return i != bytes - 1;
}

BOOL string_array_contains(char* strings[], char* compareVal)
{
	UINT i = 0;
	while (strings[i]) {
		if (strcmp(strings[i], compareVal) == 0) {
			return TRUE;
		}
		i++;
	}
	return FALSE;
}

char* get_python_type_name_from_python_type_object_address(ULONGLONG address)
{
	ULONGLONG bytes_read = 0;
	byte* type_object_memory = read_bytes(address, 0x20, &bytes_read);

	if (type_object_memory == NULL)
		return NULL;

	if (bytes_read != 0x20)
	{
		free(type_object_memory);
		type_object_memory = NULL;
		return NULL;
	}

	byte* sliced = slice_byte_array(type_object_memory, 0x20, 0x18, 0x20);
	ULONGLONG tp_name = cast_byte_array_to_ulong(sliced, 0x20 - 0x18);
	free(sliced);
	sliced = NULL;

	bytes_read = 0;
	byte* name_bytes = read_bytes(tp_name, 100, &bytes_read);
	if (!bytes_array_contains(name_bytes, bytes_read, 0))
		return NULL;
	
	UINT length = 0;
	UINT i = 0;
	do
		if (name_bytes[++length] == 0)
			break;
	while (i++ < bytes_read);
	byte* null_terminated = slice_byte_array(name_bytes, bytes_read, 0, length);
	char* response = malloc(sizeof(char) * length + 1);
	memcpy_s(response, length, null_terminated, length);
	response[length] = '\0';
	free(name_bytes);
	free(type_object_memory);
	free(null_terminated);
	name_bytes = NULL;
	type_object_memory = NULL;
	null_terminated = NULL;
	return response;
}

char* get_python_type_name_from_python_object_address(ULONGLONG address)
{
	ULONGLONG hash_index = 0;
	char* response = HashTableFind(python_type_name_cache, address, &hash_index, NULL);
	if (response != NULL)
		return response;
	ULONGLONG bytes_read = 0;
	byte* type_object_memory = read_bytes(address, 0x10, &bytes_read);

	if (bytes_read != 0x10)
	{
		free(response);
		free(type_object_memory);
		response = NULL;
		type_object_memory = NULL;
		return NULL;
	}
	
	byte* sliced = slice_byte_array(type_object_memory, 0x10, 8, 0x10);
	ULONGLONG sliced_addr = cast_byte_array_to_ulong(sliced, 0x10 - 8);
	free(response);
	response = NULL;
	response = get_python_type_name_from_python_type_object_address(sliced_addr);
	free(sliced);
	free(type_object_memory);
	sliced = NULL;
	type_object_memory = NULL;
	if (response == NULL)
		return NULL;
	HashTableInsert(python_type_name_cache, address, response, _msize(response), NULL);
	return response;
}

PyDictEntryList* read_active_dictionary_entries_from_address(ULONGLONG address)
{
	ULONGLONG bytes_read = 0;
	byte* dict_memory = read_bytes(address, 0x30, &bytes_read);

	if (bytes_read != 0x30)
	{
		free(dict_memory);
		dict_memory = NULL;
		return NULL;
	}

	ULONGLONG* ulong_dict_memory = cast_byte_array_to_ulong_array(dict_memory, bytes_read);

	ULONGLONG ma_fill = ulong_dict_memory[2];
	ULONGLONG ma_used = ulong_dict_memory[3];
	ULONGLONG ma_mask = ulong_dict_memory[4];
	ULONGLONG ma_table = ulong_dict_memory[5];

	ULONGLONG number_of_slots = ma_mask + 1;

	if (number_of_slots < 0 || 10000 < number_of_slots)
	{
		free(dict_memory);
		free(ulong_dict_memory);
		dict_memory = NULL;
		ulong_dict_memory = NULL;
		return NULL;
	}
	
	ULONGLONG slots_memory_size = number_of_slots * 8 * 3;
	bytes_read = 0;
	byte* slots_memory = read_bytes(ma_table, slots_memory_size, &bytes_read);
	if (bytes_read != slots_memory_size)
	{
		free(dict_memory);
		free(ulong_dict_memory);
		free(slots_memory);
		dict_memory = NULL;
		ulong_dict_memory = NULL;
		slots_memory = NULL;
		return NULL;
	}
	
	ULONGLONG* ulong_slots_memory = cast_byte_array_to_ulong_array(slots_memory, bytes_read);
	PyDictEntryList* entries = NewPyDictEntryList();

	UINT slot_index;
	for (slot_index = 0; slot_index < number_of_slots; ++slot_index)
	{
		ULONGLONG hash = ulong_slots_memory[slot_index * 3];
		ULONGLONG key = ulong_slots_memory[slot_index * 3 + 1];
		ULONGLONG value = ulong_slots_memory[slot_index * 3 + 2];

		if (key == 0 || value == 0)
		{
			continue;
		}

		InsertPyDictEntry(entries, NewPyDictEntry(hash, key, value));
	}

	free(dict_memory);
	free(ulong_dict_memory);
	free(ulong_slots_memory);
	free(slots_memory);
	dict_memory = NULL;
	ulong_dict_memory = NULL;
	ulong_slots_memory = NULL;
	slots_memory = NULL;

	return entries;
}

char* read_python_string_value(ULONGLONG address, ULONGLONG max_length)
{
	ULONGLONG hash_index = 0;
	char* response = HashTableFind(python_string_value_cache, address, &hash_index, NULL);
	if (response != NULL)
		return response;
	ULONGLONG bytes_read = 0;
	byte* string_object_memory = read_bytes(address, 0x20, &bytes_read);

	if (bytes_read != 0x20)
	{
		free(string_object_memory);
		string_object_memory = NULL;
		return NULL;
	}

	byte* sliced = slice_byte_array(string_object_memory, 0x20, 0x10, 0x10 + 8);
	ULONGLONG string_object_size = cast_byte_array_to_ulong(sliced, 8);
	free(sliced);
	sliced = NULL;
	free(string_object_memory);
	string_object_memory = NULL;

	if (0 < max_length && max_length < string_object_size || ULLONG_MAX < string_object_size)
	{
		return NULL;
	}
	
	bytes_read = 0;
	byte* string_bytes = read_bytes(address + 8 * 4, string_object_size, &bytes_read);

	if (bytes_read != string_object_size)
	{
		free(string_bytes);
		string_bytes = NULL;
		return NULL;
	}

	response = malloc(sizeof(char) * bytes_read + 1);
	memcpy_s(response, bytes_read, string_bytes, bytes_read);
	response[bytes_read] = '\0';
	free(string_bytes);
	string_bytes = NULL;
	HashTableInsert(python_string_value_cache, address, response, _msize(response), NULL);
	return response;
}

void read_python_type_str(ULONGLONG address, PythonDictValueRepresentation* repr)
{
	repr->is_string = TRUE;
	repr->string_value = read_python_string_value(address, 0x1000);
}

char sanitize(byte utf)
{
	if (utf == '"')
		return ' ';
	else if (utf == '\'')
		return ' ';
	else if (utf == '\n')
		return ' ';
	else if (utf == '\r')
		return ' ';
	else if (utf < 0x21)
		return ' ';
	else
		return (char)utf;
}

int utf8_encode(char* out, byte utf)
{
	if (utf <= 0x7F) {
		// Plain ASCII
		out[0] = sanitize(utf);
		out[1] = '\0';
		return 1;
	}
	else {
		// error - use replacement character
		out[0] = ' ';
		out[1] = '\0';
		return 1;
	}
}

void read_python_type_unicode(ULONGLONG address, PythonDictValueRepresentation* repr)
{
	ULONGLONG bytes_read = 0;
	byte* python_object_memory = read_bytes(address, 0x20, &bytes_read);

	if (bytes_read != 0x20)
	{
		free(python_object_memory);
		python_object_memory = NULL;
		return;
	}

	byte* sliced = slice_byte_array(python_object_memory, bytes_read, 0x10, 0x10 + 8);
	ULONGLONG unicode_string_length = cast_byte_array_to_ulong(sliced, 8);
	free(sliced);
	sliced = NULL;

	if (unicode_string_length > 0x1000)
	{
		free(python_object_memory);
		python_object_memory = NULL;
		return;
	}

	ULONGLONG string_bytes_count = unicode_string_length * 2;
	sliced = slice_byte_array(python_object_memory, bytes_read, 0x18, bytes_read);
	ULONGLONG string_address = cast_byte_array_to_ulong(sliced, bytes_read - 0x18);
	free(sliced);
	sliced = NULL;
	bytes_read = 0;
	byte* string_bytes = read_bytes(string_address, string_bytes_count, &bytes_read);

	if (bytes_read != string_bytes_count)
	{
		free(string_bytes);
		free(python_object_memory);
		string_bytes = NULL;
		python_object_memory = NULL;
		return;
	}

	char* response = malloc(sizeof(char) * bytes_read / 2 + 1);
	memset(response, 0, bytes_read / 2 + 1);
	UINT i = 0;
	for (i = 0; i < bytes_read; i += 2)
	{
		char* utf8_decoded = malloc(sizeof(char) * 5);
		memset(utf8_decoded, 0, 5);
		size_t dstlen = strlen(response);
		size_t srclen = utf8_encode(utf8_decoded, string_bytes[i]);
		memcpy(response + dstlen, utf8_decoded, srclen);
		free(utf8_decoded);
		utf8_decoded = NULL;
	}
	response[bytes_read/2] = '\0';
	repr->is_unicode = TRUE;
	repr->unicode_value = response;
	free(string_bytes);
	free(python_object_memory);
	string_bytes = NULL;
	python_object_memory = NULL;
	//printf("repr->unicode_value = '%s'\n", repr->unicode_value);
}

void read_python_type_int(ULONGLONG address, PythonDictValueRepresentation* repr)
{
	ULONGLONG bytes_read = 0;
	byte* int_object_memory = read_bytes(address, 0x18, &bytes_read);

	if (int_object_memory == NULL)
		return;

	if (bytes_read != 0x18)
	{
		free(int_object_memory);
		int_object_memory = NULL;
		return;
	}

	byte* sliced = slice_byte_array(int_object_memory, bytes_read, 0x10, bytes_read);
	ULONGLONG value = cast_byte_array_to_ulong(sliced, bytes_read - 0x10);
	free(sliced);
	sliced = NULL;

	repr->is_int = TRUE;
	repr->int_value = value;
	free(int_object_memory);
	int_object_memory = NULL;
}

void read_python_type_float(ULONGLONG address, PythonDictValueRepresentation* repr)
{
}

void read_python_type_bool(ULONGLONG address, PythonDictValueRepresentation* repr)
{
	ULONGLONG bytes_read = 0;
	byte* bool_object_memory = read_bytes(address, 0x18, &bytes_read);

	if (bytes_read != 0x18) {
		return;
	}
	repr->is_bool = TRUE;
	byte* sliced = slice_byte_array(bool_object_memory, bytes_read, 0x10, 0x10 + 8);
	ULONGLONG response = cast_byte_array_to_ulong(sliced, 8);
	free(sliced);
	sliced = NULL;
	repr->bool_value = response != 0;
	free(bool_object_memory);
	bool_object_memory = NULL;
}

PyDictEntryList* get_dict_entries_with_string_keys(ULONGLONG address)
{
	PyDictEntryList* dict_entries = read_active_dictionary_entries_from_address(address);

	if (dict_entries == NULL)
		return NULL;

	UINT i;
	for (i = 0; i < dict_entries->used; ++i)
	{
		dict_entries->data[i]->key_resolved = read_python_string_value(dict_entries->data[i]->key, 4000);
	}

	return dict_entries;
}

void read_python_type_bunch(ULONGLONG address, PythonDictValueRepresentation* repr)
{
}

PythonDictValueRepresentation* get_dict_entry_value_representation(ULONGLONG address)
{
	char* python_type_name = get_python_type_name_from_python_object_address(address);
	if (python_type_name == NULL)
		return NULL;

	ULONGLONG hash_index = 0;
	PythonDictValueRepresentation* repr = HashTableFind(dict_entry_cache, address, &hash_index, dict_entry_cache_copy_fn);
	if (repr != NULL)
	{
		free(python_type_name);
		return repr;
	}
	repr = malloc(sizeof(PythonDictValueRepresentation));
	repr->address = address;
	repr->python_object_type_name = python_type_name;
	repr->is_string = FALSE;
	repr->is_unicode = FALSE;
	repr->is_int = FALSE;
	repr->is_float = FALSE;
	repr->is_bool = FALSE;
	repr->is_pycolor = FALSE;
	repr->is_bunch = FALSE;
	repr->string_value = NULL;
	repr->unicode_value = NULL;
	repr->int_value = 0;
	repr->bool_value = FALSE;
	repr->float_value = 0.0;

	if (strcmp(python_type_name, "str") == 0)
		read_python_type_str(address, repr);
	else if (strcmp(python_type_name, "unicode") == 0)
		read_python_type_unicode(address, repr);
	else if (strcmp(python_type_name, "int") == 0)
		read_python_type_int(address, repr);
	else if (strcmp(python_type_name, "bool") == 0)
		read_python_type_bool(address, repr);
	else if (strcmp(python_type_name, "float") == 0)
		read_python_type_float(address, repr);
	else if (strcmp(python_type_name, "Bunch") == 0)
		read_python_type_bunch(address, repr);

	HashTableInsert(dict_entry_cache, address, repr, _msize(repr), dict_entry_cache_copy_fn);
	return repr;
}

UITreeNode** read_children(UITreeNodeDictEntryList* dict_entries_of_interest, int max_depth, ULONGLONG* number_of_children)
{
	if (max_depth < 1)
		return NULL;

	UITreeNode** nodes = NULL;
	UINT i;
	for (i = 0; i < dict_entries_of_interest->used; ++i)
	{
		if (strcmp(dict_entries_of_interest->data[i]->key, "children") != 0)
			continue;

		ULONGLONG children_entry_object_address = dict_entries_of_interest->data[i]->value->address;
		ULONGLONG bytes_read = 0;
		byte* children_list_memory = read_bytes(children_entry_object_address, 0x18, &bytes_read);
		if (bytes_read != 0x18)
		{
			free(children_list_memory);
			children_list_memory = NULL;
			return NULL;
		}

		byte* sliced = slice_byte_array(children_list_memory, bytes_read, 0x10, 0x10 + 8);
		ULONGLONG children_dict_address = cast_byte_array_to_ulong(sliced, 8);
		free(sliced);
		sliced = NULL;
		PyDictEntryList* children_dict_entries = read_active_dictionary_entries_from_address(children_dict_address);

		if (children_dict_entries == NULL)
		{
			free(children_list_memory);
			children_list_memory = NULL;
			return NULL;
		}

		PyDictEntry* children_entry = NULL;
		UINT i;
		for (i = 0; i < children_dict_entries->used; ++i)
		{
			char* python_type_name = get_python_type_name_from_python_object_address(children_dict_entries->data[i]->key);
			if (strcmp(python_type_name, "str") != 0)
			{
				free(python_type_name);
				python_type_name = NULL;
				continue;
			}
			char* key_string = read_python_string_value(children_dict_entries->data[i]->key, 4000);
			if (strcmp(key_string, "_childrenObjects") == 0)
			{
				children_entry = children_dict_entries->data[i];
				free(python_type_name);
				free(key_string);
				python_type_name = NULL;
				key_string = NULL;
				break;
			}
		}

		if (children_entry == NULL)
		{
			free(children_list_memory);
			FreePyDictEntryList(children_dict_entries);
			children_list_memory = NULL;
			return NULL;
		}

		bytes_read = 0;
		byte* python_list_object_memory = read_bytes(children_entry->value, 0x20, &bytes_read);

		if (python_list_object_memory == NULL)
		{
			free(children_list_memory);
			FreePyDictEntryList(children_dict_entries);
			children_list_memory = NULL;
			return NULL;
		}

		if (bytes_read != 0x20)
		{
			free(python_list_object_memory);
			free(children_list_memory);
			FreePyDictEntryList(children_dict_entries);
			python_list_object_memory = NULL;
			children_list_memory = NULL;
			return NULL;
		}

		sliced = slice_byte_array(python_list_object_memory, bytes_read, 0x10, 0x10 + 8);
		if (sliced == NULL)
		{
			free(python_list_object_memory);
			free(children_list_memory);
			FreePyDictEntryList(children_dict_entries);
			python_list_object_memory = NULL;
			children_list_memory = NULL;
			return NULL;
		}

		ULONGLONG list_ob_size = cast_byte_array_to_ulong(sliced, 8);
		free(sliced);
		sliced = NULL;

		if (list_ob_size > 4000)
		{
			free(python_list_object_memory);
			free(children_list_memory);
			FreePyDictEntryList(children_dict_entries);
			python_list_object_memory = NULL;
			children_list_memory = NULL;
			return NULL;
		}
		
		ULONGLONG list_entries_size = list_ob_size * 8;
		sliced = slice_byte_array(python_list_object_memory, bytes_read, 0x18, 0x18 + 8);
		ULONGLONG list_ob_item = cast_byte_array_to_ulong(sliced, 8);
		free(sliced);
		sliced = NULL;
		bytes_read = 0;
		byte* list_entry_memory = read_bytes(list_ob_item, list_entries_size, &bytes_read);

		if (bytes_read != list_entries_size || list_entries_size == 0)
		{
			free(python_list_object_memory);
			free(children_list_memory);
			free(list_entry_memory);
			FreePyDictEntryList(children_dict_entries);
			python_list_object_memory = NULL;
			children_list_memory = NULL;
			list_entry_memory = NULL;
			children_dict_entries = NULL;
			return NULL;
		}

		ULONGLONG* list_entries = cast_byte_array_to_ulong_array(list_entry_memory, bytes_read);

		nodes = malloc(sizeof(UITreeNode*) * bytes_read / 8);
		for (i = 0; i < bytes_read / 8; ++i)
		{
			UITreeNode* tmp = read_ui_tree_from_address(list_entries[i], max_depth - 1);
			if (tmp != NULL)
				nodes[(*number_of_children)++] = tmp;
		}
		free(python_list_object_memory);
		free(children_list_memory);
		FreePyDictEntryList(children_dict_entries); // THIS MIGHT NEED TO GO AWAY
		free(list_entries);
		free(list_entry_memory);
		python_list_object_memory = NULL;
		children_list_memory = NULL;
		list_entries = NULL;
		list_entry_memory = NULL;
		break; // only operate on first child
	}
	return nodes;
}

UITreeNode* read_ui_tree_from_address(ULONGLONG address, int max_depth)
{
	ULONGLONG bytes_read = 0;
	byte* ui_node_object_memory = read_bytes(address, 0x30, &bytes_read);

	if (ui_node_object_memory == NULL)
		return NULL;

	if (bytes_read != 0x30)
	{
		free(ui_node_object_memory);
		ui_node_object_memory = NULL;
		return NULL;
	}

	char* python_object_type_name = get_python_type_name_from_python_object_address(address);
	if (python_object_type_name == NULL)
	{
		free(ui_node_object_memory);
		ui_node_object_memory = NULL;
		return NULL;
	}
	
	if (python_object_type_name[0] == '\0')
	{
		free(python_object_type_name);
		free(ui_node_object_memory);
		python_object_type_name = NULL;
		ui_node_object_memory = NULL;
		return NULL;
	}

	byte* sliced = slice_byte_array(ui_node_object_memory, bytes_read, 0x10, 0x10 + 8);
	ULONGLONG dict_address = cast_byte_array_to_ulong(sliced, 8);
	free(sliced);
	sliced = NULL;
	PyDictEntryList* dict_entries = read_active_dictionary_entries_from_address(dict_address);

	if (dict_entries == NULL)
	{
		free(ui_node_object_memory);
		FreePyDictEntryList(dict_entries);
		free(python_object_type_name);
		ui_node_object_memory = NULL;
		dict_entries = NULL;
		python_object_type_name = NULL;
		return NULL;
	}

	UITreeNodeDictEntryList* dict_entries_of_interest = NewUITreeNodeDictEntryList();

	BOOL display = TRUE;
	BOOL display_checked = FALSE;

	UINT i;
	for (i = 0; i < dict_entries->used; ++i)
	{
		char* key_object_type_name = get_python_type_name_from_python_object_address(dict_entries->data[i]->key);

		if (strcmp(key_object_type_name, "str") != 0)
		{
			free(key_object_type_name);
			key_object_type_name = NULL;
			continue;
		}
		free(key_object_type_name);
		key_object_type_name = NULL;

		char* key_string = read_python_string_value(dict_entries->data[i]->key, 4000);

		if (!string_array_contains(DICT_KEYS_OF_INTEREST, key_string))
		{
			free(key_string);
			key_string = NULL;
			continue;
		}
		
		PythonDictValueRepresentation* repr = get_dict_entry_value_representation(dict_entries->data[i]->value);
		if (repr == NULL)
		{
			free(key_string);
			key_string = NULL;
			continue;
		}
		if (strcmp(key_string, "_display") == 0 && repr->is_bool && !display_checked)
		{
			display = repr->bool_value;
			display_checked = TRUE;
		}
		InsertUITreeNodeDictEntry(dict_entries_of_interest, NewUITreeNodeDictEntry(key_string, repr));
	}

	if (!display)
	{
		free(ui_node_object_memory);
		FreePyDictEntryList(dict_entries);
		FreeUITreeNodeDictEntryList(dict_entries_of_interest);
		free(python_object_type_name);
		ui_node_object_memory = NULL;
		dict_entries = NULL;
		dict_entries_of_interest = NULL;
		python_object_type_name = NULL;
		return NULL;
	}

	ULONGLONG number_of_children = 0;
	UITreeNode** children = read_children(dict_entries_of_interest, max_depth, &number_of_children);
	UITreeNode* node = malloc(sizeof(UITreeNode));
	node->address = address;
	node->dict_entries_of_interest = dict_entries_of_interest;
	node->python_object_type_name = python_object_type_name;
	node->children = children;
	node->number_of_children = number_of_children;

	free(ui_node_object_memory);
	FreePyDictEntryList(dict_entries);
	ui_node_object_memory = NULL;
	++number_of_nodes;
	return node;
}


__declspec(dllexport) void free_ui_json()
{
	free(ui_json);
	ui_json = NULL;
}

__declspec(dllexport) char* get_ui_json()
{
	return ui_json;
}

__declspec(dllexport) void read_ui_trees()
{
	if ((unsigned long)time(NULL) - cache_last_flushed > 30)
	{
		printf("flushing cache...\n");
		int total_flushed = 0;
		total_flushed += HashTableFlushStale(dict_entry_cache, dict_entry_cache_free_fn);
		total_flushed += HashTableFlushStale(python_string_value_cache, NULL);
		total_flushed += HashTableFlushStale(python_type_name_cache, NULL);
		printf("cache flushed %d stale items!\n", total_flushed);
		cache_last_flushed = (unsigned long)time(NULL);
	}
	UINT i;
	for (i = 0; i < ui_roots->used; ++i)
	{
		if (primary_ui_root != 0 && primary_ui_root != ui_roots->data[i])
			continue;
		number_of_nodes = 0;
		UITreeNode* ui_tree = read_ui_tree_from_address(ui_roots->data[i], 99);
		if (ui_tree != NULL && ui_tree->number_of_children > 5)
		{
			primary_ui_root = ui_roots->data[i];
			//printf("found %I64u nodes!\n", number_of_nodes);
			char* json_str = PrintUITreeNode(ui_tree, 0);
			size_t json_str_len = strlen(json_str);
			ui_json = malloc(json_str_len + 1);
			memcpy_s(ui_json, json_str_len, json_str, json_str_len);
			ui_json[json_str_len] = '\0';
			free(json_str);
			//free(response);
			//response = NULL;
			FreeUITreeNode(ui_tree);
		}
		else FreeUITreeNode(ui_tree);
	}
}


void get_memory_and_root_addresses(DWORD pid)
{
	printf("reading committed memory...\n");
	ULONGLONG start_time = (unsigned long)time(NULL);
	get_process_sample_from_pid(pid);
	ULONGLONG ps_end_time = (unsigned long)time(NULL);
	ULONGLONG ps_dur = ps_end_time - start_time;
	printf("Finished processing memory in %I64u seconds!\n", ps_dur);

	printf("finding UIRoot candidates...\n");
	ui_roots = get_ui_roots_from_process_sample_file();
	ULONGLONG ui_roots_end_time = (unsigned long)time(NULL);
	ULONGLONG ui_roots_dur = ui_roots_end_time - ps_end_time;
	printf("Finished finding %I64u UIRoot candidates in %I64u seconds!\n", ui_roots->used, ui_roots_dur);
}


__declspec(dllexport) int initialize()
{
	primary_ui_root = 0;
	DWORD pid = get_pid(PROCESS_NAME);
	if (pid == -1)
	{
		printf("Process not found\n");
		return 1;
	}

	initialize_cache();
	cache_last_flushed = (unsigned long)time(NULL);
	printf("found %s process (pid: %d)\n", PROCESS_NAME, pid);
	get_memory_and_root_addresses(pid);
	return 0;
}

__declspec(dllexport) void free_string(char* str)
{
	if (str != NULL)
	{
		free(str);
		str = NULL;
	}
}

__declspec(dllexport) void cleanup()
{
	FreeProcessSample(ps);
	FreeAddresses(ui_roots);
}

int main()
{
	return 0;
}
