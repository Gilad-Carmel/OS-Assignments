#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MAX_PROCESSES 1000
#define MAX_NAME_LENGTH 50
#define MAX_DESCRIPTION_LENGTH 100

typedef struct
{
    int csvRowNumber;
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];

    // time in seconds
    int arrivalTime;
    int burstTime;
    int priority;
} Process;

typedef struct
{
    Process processes[MAX_PROCESSES];
    int count;
} ProcessList;

// read processes from CSV file
void readProcessesFromCsv(const char *filePath, ProcessList *processList)
{
    FILE *file = fopen(filePath, "r");
    if (!file)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        // skip row that starts with '#'
        if (line[0] == '#')
            continue;
        Process process;
        sscanf(line, "%50[^,],%100[^,],%d,%d,%d",
               process.name,
               process.description,
               &process.arrivalTime,
               &process.burstTime,
               &process.priority);
        process.csvRowNumber = processList->count; // 1-based index
        processList->processes[processList->count] = process;
        processList->count++;
    }
    fclose(file);
}

void printProcessList(const ProcessList *processList)
{
    printf("Processes:\n");
    for (int i = 0; i < processList->count; i++)
    {
        const Process *p = &processList->processes[i];
        printf("Row %d: %s, %s, Arrival: %d, Burst: %d, Priority: %d\n",
               p->csvRowNumber, p->name, p->description,
               p->arrivalTime, p->burstTime, p->priority);
    }
}

void executeProcess

// Scheduler algorithms
void runFCFS(char *processesCsvFilePath) {}

void runSJF(char *processesCsvFilePath) {}

void runPriorityScheduling(char *processesCsvFilePath) {}

// Round Robin

void runCPUScheduler(char *processesCsvFilePath, int timeQuantum) {
    ProcessList processList = {0};
    readProcessesFromCsv(processesCsvFilePath, &processList);
    printProcessList(&processList);
}