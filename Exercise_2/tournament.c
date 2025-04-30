#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_GLADIATORS 4


char* gladiator_names[NUM_GLADIATORS] = {"Maximus", "Lucius", "Commodus", "Spartacus"};
char* gladiator_files[NUM_GLADIATORS] = {"G1", "G2", "G3", "G4"};

// fork a child for each glidator
// use exec to launch a process for each gladiator (pass matching gladiator file name as an argument)

// wait for the child process to complete and determine the winner based
// on whose gladiator process terminated last


int main()
{
    int gladiator_pids[NUM_GLADIATORS];

    for (int i = 0; i < NUM_GLADIATORS; i++)
    {
        gladiator_pids[i] = fork();
        if (gladiator_pids[i] == 0)
        {
            // use execvp to launch a process for each gladiator
            char *args[] = {"./gladiator", gladiator_files[i], NULL};
            execvp(args[0], args);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    }

    // wait for all gladiators to finish
    int winner_index = -1;
    
    // Get gladiators in order of termination 
    for (int i = 0; i < NUM_GLADIATORS; i++) {
        int status;
        // only the last pid w
        pid_t terminated_pid = wait(&status);
        
        for (int j = 0; j < NUM_GLADIATORS; j++) {
            if (gladiator_pids[j] == terminated_pid) {
                winner_index = j;
                break;
            }
        }
    }
    printf("The gods have spoken, the winner of the tournament is %s!\n", gladiator_names[winner_index]);

    return 0;
}
