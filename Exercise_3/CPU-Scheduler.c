#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAX_PROCESSES 1000
#define MAX_NAME_LENGTH 50
#define MAX_DESCRIPTION_LENGTH 100

// a flag to indicate if an alarm signal has been received
volatile sig_atomic_t sig_alarm_received = 0;
char output_buffer[4096]; // buffer for async-signal-safe output

// comprator for sorting processes
typedef int (*Compatator)(const void *, const void *);


typedef struct
{
    pid_t pid; // process ID
    int csvRowNumber;
    char name[MAX_NAME_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];

    // time in seconds
    int arrivalTime;
    int burstTime;
    int priority;
    int startTime;      // time when the process starts running
    int turnaroundTime; // time from arrival to completion
    int waitingTime;    // time spent waiting in the ready queue
    int completionTime; // time when the process completes execution
    int remainingBurstTime; // remaining burst time for the process (used in Round Robin)
} Process;

typedef struct
{
    Process processes[MAX_PROCESSES];
    int count;
} ProcessList;

// print functions

void print_header(const char *scheduler_name) {
    int len = snprintf(output_buffer, sizeof(output_buffer),
                       "══════════════════════════════════════════════\n"
                       ">> Scheduler Mode : %s\n"
                       ">> Engine Status  : Initialized\n"
                       "──────────────────────────────────────────────\n\n",
                       scheduler_name);
    write(STDOUT_FILENO, output_buffer, len);
}


void print_footer(double avg_waiting_time, int total_turnaround_time, const char *scheduler_name) {
    int len;
    len = snprintf(output_buffer, sizeof(output_buffer),
                   "\n──────────────────────────────────────────────\n"
                   ">> Engine Status  : Completed\n"
                   ">> Summary        :\n");
    write(STDOUT_FILENO, output_buffer, len);

    // Print specific summary based on the scheduler type
    if (strcmp(scheduler_name, "Round Robin") == 0) {
        len = snprintf(output_buffer, sizeof(output_buffer),
                       "   └─ Total Turnaround Time : %d time units\n\n", total_turnaround_time);
    } else {
        len = snprintf(output_buffer, sizeof(output_buffer),
                       "   └─ Average Waiting Time : %.2f time units\n", avg_waiting_time);
    }
    write(STDOUT_FILENO, output_buffer, len);

    len = snprintf(output_buffer, sizeof(output_buffer),
                   ">> End of Report\n"
                   "══════════════════════════════════════════════\n");
    // if not round robin, add a newline
    if (strcmp(scheduler_name, "Round Robin") != 0) {
        len += snprintf(output_buffer + len, sizeof(output_buffer) - len, "\n");
    }
    write(STDOUT_FILENO, output_buffer, len);
}


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
        // initialize other fields to default values
        process.csvRowNumber = processList->count;
        process.pid = -1;
        process.turnaroundTime = 0;
        process.waitingTime = 0;
        process.completionTime = -1;
        process.startTime = -1;
        process.remainingBurstTime = process.burstTime;
        processList->processes[processList->count] = process;
        processList->count++;
    }
    fclose(file);
}

// alarm setup

void alarm_handler(int signum) {
    sig_alarm_received = 1; // set the flag to indicate that SIGALRM has been received
}


void setup_alarm_handler() {
    struct sigaction sa;
    // Clear the sigaction structure
    memset(&sa, 0, sizeof(sa));
    // Assign the alarm_handler function to handle SIGALRM
    sa.sa_handler = alarm_handler;
    // SA_RESTART flag ensures that system calls interrupted by the signal are restarted
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction failed to set up SIGALRM handler");
        exit(EXIT_FAILURE);
    }
}

void block_alarm()
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    // Block SIGALRM.
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        perror("sigprocmask BLOCK failed");
        exit(EXIT_FAILURE);
    }
}

void unblock_alarm()
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    // Unblock SIGALRM.
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
        perror("sigprocmask UNBLOCK failed");
        exit(EXIT_FAILURE);
    }
}

// process management

void forkProcesses(ProcessList *processList)
{
    for (int i = 0; i < processList->count; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // child process
            signal(SIGALRM, SIG_IGN); // avoid interfering with parent's alarm
            signal(SIGCHLD, SIG_IGN); // the parent will handle reaping the child processes

            while (1)
            {
                pause(); // wait for signals from parent
            }
            exit(0); // exit when done, shouldnt happen because we use SIGKILL
        }
        else
        {
            // parent process
            processList->processes[i].pid = pid; // store the child's pid

            // stop the child process immediately after forking
            // so we can control when it starts and stops running
            if (kill(pid, SIGSTOP) == -1)
            {
                perror("Failed to stop child process");
                exit(1);
            }
        }

    }
}


void pass_time(int duration)
{
    if (duration <= 0)
        return;
    
    block_alarm();
    // reset alarm flag
    sig_alarm_received = 0;
    alarm(duration);

    sigset_t suspend_mask;
    sigemptyset(&suspend_mask);

    // temporarily unblock all signals while waiting for the alarm
    // eintr means that the wait was interrupted by a signal
    if (sigsuspend(&suspend_mask) == -1 && errno != EINTR) 
    {
        perror("sigsuspend failed");
        exit(EXIT_FAILURE);
    }

    alarm(0);
    unblock_alarm();
}

void handle_idle_time(int targetTime, int *currentTime)
{
    if (*currentTime < targetTime)
    {
        int idleDuration = targetTime - *currentTime;

        pass_time(idleDuration);
        // log idle time, using signal-safe function
        int len = snprintf(output_buffer, sizeof(output_buffer),
        "%d → %d: Idle.\n", *currentTime, *currentTime + idleDuration);
        write(STDOUT_FILENO, output_buffer, len);
        *currentTime += idleDuration;
    }
}

void execute_procces_in_timeframe(Process *p, int duration, int* current_time) {
    if (p->startTime == -1) {
        p->startTime = *current_time; // record start time if not set
    }

    // continue the child process
    if (kill(p->pid, SIGCONT) == -1) {
        perror("Failed to continue child process");
        exit(EXIT_FAILURE);
    }

    // pass time until alarm
    pass_time(duration);

    // stop the child process after execution
    if (kill(p->pid, SIGSTOP) == -1) {
        perror("Failed to stop child process after execution");
        exit(EXIT_FAILURE);
    }

    // log process execution
    int len = snprintf(output_buffer, sizeof(output_buffer),
                       "%d → %d: %s Running %s.\n",
                       *current_time, *current_time + duration,
                       p->name, p->description);
    write(STDOUT_FILENO, output_buffer, len);

    *current_time += duration;


    p->remainingBurstTime -= duration; // update remaining burst time

}

void finalize_process(Process *p, int *num_completed, int *current_time) {
    
    p->completionTime = *current_time;
    p->turnaroundTime = p->completionTime - p->arrivalTime;
    p->waitingTime = p->turnaroundTime - p->burstTime;

    // kill child
    if (kill(p->pid, SIGKILL) == -1 && errno != ESRCH) {
        perror("Failed to kill child process");
        exit(EXIT_FAILURE);
    }

    waitpid(p->pid, NULL, 0); // wait for the child process to terminate
    p->pid = -1;

    (*num_completed)++; // increment the number of completed processes
}

void cleanup_children(ProcessList *processList) {
    for (int i = 0; i < processList->count; i++) {
        Process *p = &processList->processes[i];
        if (p->pid > 0) {
            // kill the child process if it is still running
            kill(p->pid, SIGSTOP); // stop the process if it is running
            if (kill(p->pid, SIGKILL) == -1 && errno != ESRCH) {
                perror("Failed to kill child process during cleanup");
                exit(EXIT_FAILURE);
            }
            waitpid(p->pid, NULL, 0); // wait for the child process to terminate
            p->pid = -1; // reset pid to indicate it has been cleaned up
        }
    }
}

void reset_attributes(ProcessList *processList) {
    for (int i = 0; i < processList->count; i++) {
        Process *p = &processList->processes[i];
        p->pid = -1;
        p->remainingBurstTime = p->burstTime;
        p->startTime = -1;
        p->completionTime = -1;
        p->waitingTime = 0;
        p->turnaroundTime = 0;
    }
}

void cleanup_process_list(ProcessList *processList) {
    // cleanup children processes
    cleanup_children(processList);
    
    // reset attributes of all processes
    reset_attributes(processList);
}




// scheduler helper functions

int compareFCFS(const void *a, const void *b)
{
    const Process *p1 = (const Process *)a;
    const Process *p2 = (const Process *)b;
    if (p1->arrivalTime != p2->arrivalTime) {
        return p1->arrivalTime - p2->arrivalTime;
    }
    // maintain original order
    return p1->csvRowNumber - p2->csvRowNumber;
}

int compareSJF(const void *a, const void *b)
{
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;

    if (p1->burstTime != p2->burstTime) {
        return p1->burstTime - p2->burstTime; // shorter burst time first
    }
    if (p1->arrivalTime != p2->arrivalTime) {
        return p1->arrivalTime - p2->arrivalTime; // earlier arrival time first
    }
    return p1->csvRowNumber - p2->csvRowNumber; // maintain original order
}

int compareProcessesByPriority(const void *a, const void *b)
{
    const Process *p1 = (const Process *)a;
    const Process *p2 = (const Process *)b;

    if (p1->priority != p2->priority) {
        return p1->priority - p2->priority; // lower priority value first
    }
    if (p1->arrivalTime != p2->arrivalTime) {
        return p1->arrivalTime - p2->arrivalTime; // earlier arrival time first
    }
    return p1->csvRowNumber - p2->csvRowNumber; // maintain original order
}

int get_next_arrival(const ProcessList *processList, int currentTime) {
    int next_arrival = -1;
    for (int i = 0; i < processList->count; i++) {
        const Process *p = &processList->processes[i];
        if (p->completionTime == -1 && (next_arrival == -1 || p->arrivalTime < next_arrival)) {
            next_arrival = p->arrivalTime;
        }
    }
    return next_arrival; // no more arrivals
}

// scheduler algorithms

void run_compare_based_scheduler(ProcessList *process_list, Compatator comparator, const char *scheduler_name) {
    print_header(scheduler_name);
    // copy ProcessList to a local variable
    ProcessList localProcessList;
    localProcessList.count = process_list->count;
    for (int i = 0; i < process_list->count; i++) {
        localProcessList.processes[i] = process_list->processes[i];
    }

    // sort processes by arrival time
    qsort(localProcessList.processes, localProcessList.count, sizeof(Process), comparator);

    int currentTime = 0;
    int completed_processes = 0;

    while (completed_processes < localProcessList.count) {
        
        Process *next_process = NULL;
        // find the next process to execute
        for (int i = 0; i < localProcessList.count; i++) {
            Process *p = &localProcessList.processes[i];
            if (p->completionTime == -1 && p->arrivalTime <= currentTime) {
                next_process = p;
                break;
            }
        }

        if (next_process) {
            // simulate idle time
            handle_idle_time(next_process->arrivalTime, &currentTime);
            // calculate waiting time for this process
            if (next_process->startTime == -1) {
                next_process->startTime = currentTime;
            }
            next_process->waitingTime = next_process->startTime - next_process->arrivalTime;

            // execute process for duration of its burst time
            execute_procces_in_timeframe(next_process, next_process->burstTime, &currentTime);

            // finalize (set completion time, turnaround time, kill process, add to current time)
            finalize_process(next_process, &completed_processes, &currentTime);
        } else {
            // find if there are any processes that have not yet arrived
            int nextArrival = get_next_arrival(&localProcessList, currentTime);
            // if there are, go idle
            if (nextArrival != -1) {
                handle_idle_time(nextArrival, &currentTime);
            }
            else
            {
                // else, all processes are completed, break the loop
                break;
            }
        }
    }
    // calculate average waiting time
    double totalWaitingTime = 0;
    for (int i = 0; i < localProcessList.count; i++) {
        totalWaitingTime += localProcessList.processes[i].waitingTime;
    }
    double averageWaitingTime = totalWaitingTime / localProcessList.count;
    // print report footer
    print_footer(averageWaitingTime, 0, scheduler_name);
}

void run_scheduler_with_setup(ProcessList *process_list, Compatator comparator, const char *scheduler_name) {
    forkProcesses(process_list); // fork processes
    run_compare_based_scheduler(process_list, comparator, scheduler_name);
    cleanup_process_list(process_list); // cleanup processes
}

// Round Robin

void run_round_robin(ProcessList *process_list, int time_quantum) {
    print_header("Round Robin");

    ProcessList localProcessList;
    localProcessList.count = process_list->count;
    for (int i = 0; i < process_list->count; i++) {
        localProcessList.processes[i] = process_list->processes[i];
    }

    int current_time = 0;
    int completed_count = 0;

    // circular queue
    Process *queue[MAX_PROCESSES];
    int head = 0, tail = 0; // queue pointers

    while (completed_count < localProcessList.count) {
        for (int i = 0; i < localProcessList.count; i++) {
            Process *p = &localProcessList.processes[i];
            if (p->completionTime == -1 && p->arrivalTime <= current_time) {
                // check if the process is already in the queue
                int in_queue = 0;
                for (int j = head; j != tail; j = (j + 1) % MAX_PROCESSES) {
                    if (queue[j] == p) {
                        in_queue = 1;
                        break;
                    }
                }

                // if not in queue, add it
                if (!in_queue) {
                    queue[tail] = p;
                    tail = (tail + 1) % MAX_PROCESSES;
                }
            }
        }

        Process *current_process = NULL;
        if (head != tail) { // if the queue is not empty
            current_process = queue[head];
            head = (head + 1) % MAX_PROCESSES;
        }
        // if there are no processes in the queue, go idle
        if (!current_process) {
            int next_arrival = get_next_arrival(&localProcessList, current_time);
            if (next_arrival != -1) {
                handle_idle_time(next_arrival, &current_time);
            } else {
                // all processes are completed
                break;
            }
        } else {
            // run the minimum between time quantum and remaining burst time
            int run_duration;
            if (current_process->remainingBurstTime < time_quantum) {
                run_duration = current_process->remainingBurstTime;
            } else {
                run_duration = time_quantum;
            }

            execute_procces_in_timeframe(current_process, run_duration, &current_time);

            if(current_process->remainingBurstTime <= 0) {
                // process completed
                finalize_process(current_process, &completed_count, &current_time);
            } else {
                // process not completed, add it back to the end of the queue
                queue[tail] = current_process;
                tail = (tail + 1) % MAX_PROCESSES;
            }

            
        }
    }
    
    print_footer(0, current_time, "Round Robin");
}

void runCPUScheduler(char *processesCsvFilePath, int timeQuantum) {

    ProcessList processList = {0};
    readProcessesFromCsv(processesCsvFilePath, &processList);

    // setup the alarm handler
    setup_alarm_handler();

    // scheduling algorithms
    // fcfs
    run_scheduler_with_setup(&processList, compareFCFS, "FCFS");
    // sjf
    run_scheduler_with_setup(&processList, compareSJF, "SJF");
    // priority
    run_scheduler_with_setup(&processList, compareProcessesByPriority, "Priority");

    // round robin
    forkProcesses(&processList); // fork processes for round robin
    run_round_robin(&processList, timeQuantum);
    cleanup_process_list(&processList); // cleanup processes after round robin

}