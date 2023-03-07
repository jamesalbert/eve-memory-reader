#include "process.h"


void InitProcessSample(ProcessSample* ps)
{
	SIZE_T defaultSize = 256;
	ps->committed_regions = malloc(sizeof(CommittedRegion*) * defaultSize);
	ps->used = 0;
	ps->size = defaultSize;
}

void InsertCommittedRegion(ProcessSample* ps, CommittedRegion* cr)
{
	if (ps->used == ps->size)
	{
		ps->size *= 2;
		CommittedRegion** tmp = realloc(ps->committed_regions, ps->size * sizeof(CommittedRegion*));
		if (tmp == NULL)
		{
			printf("unable to realloc CommittedRegion array for ProcessSample\n");
			exit(1);
		}
		ps->committed_regions = tmp;
	}
	if (ps->committed_regions == NULL)
	{
		printf("CommittedRegion array for ProcessSample has become corrupted\n");
		exit(1);
	}
	ps->committed_regions[ps->used++] = cr;
}

void FreeProcessSample(ProcessSample* ps)
{
	if (ps == NULL)
		return;

	if (ps->committed_regions != NULL)
	{
		UINT i;
		for (i = 0; i < ps->used; ++i)
		{
			if (ps->committed_regions[i] == NULL || ps->committed_regions[i]->content == NULL)
				continue;
			else
			{
				free(ps->committed_regions[i]->content);
				ps->committed_regions[i]->content = NULL;
				free(ps->committed_regions[i]);
				ps->committed_regions[i] = NULL;
			}
		}

		free(ps->committed_regions);
		ps->committed_regions = NULL;
	}

	ps->used = ps->size = 0;
}

void PrintProcessSample(ProcessSample* ps)
{
	printf("ProcessSample: length=%d, size=%d\n", ps->used, ps->size);
}

void get_process_name_from_pid(DWORD pid, char** process_name_out)
{
	size_t i;
	TCHAR process_name[MAX_PATH] = TEXT("<unknown>");

	// Get a handle to the process.

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, pid);

	// Get the process name.

	if (NULL != hProcess)
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
			&cbNeeded))
		{
			GetModuleBaseName(hProcess, hMod, process_name,
				sizeof(process_name) / sizeof(TCHAR));
		}
	}
	else 
	{
		return;
	}

	// Release the handle to the process.
	CloseHandle(hProcess);
	wcstombs_s(&i, *process_name_out, MAX_PATH, process_name, wcslen(process_name) + 1);
}

DWORD get_pid(char * process_name)
{
	DWORD pid = -1;
	unsigned int i;
	DWORD processes[1024], cb_needed, num_processes;
	if (!EnumProcesses(processes, sizeof(processes), &cb_needed))
	{
		return 1;
	}

	num_processes = cb_needed / sizeof(DWORD);

	for (i = 0; i < num_processes; i++)
	{
		if (processes[i] != 0)
		{
			char* pid_process_name = malloc(sizeof(char) * MAX_PATH);
			get_process_name_from_pid(processes[i], &pid_process_name);
			if (pid_process_name != NULL && strcmp(pid_process_name, process_name) == 0)
			{
				pid = processes[i];
			}
			free(pid_process_name);
			if (pid != -1)
			{
				break;
			}
		}
	}

	return pid;
}

