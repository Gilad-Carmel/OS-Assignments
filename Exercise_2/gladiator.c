// each gladiator
// health
// attack power
// opponent queue

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NUM_OPPONENTS 3

typedef struct
{
    int health;
    int attack_power;
    int opponents[NUM_OPPONENTS];
} Gladiator;

// read stats from file
void get_gladiator_filename(const char* glad_name, char* filename) {
    sprintf(filename, "%s.txt", glad_name);
}

void get_gladiator_log_filename(const char* glad_name, char* filename) {
    sprintf(filename, "%s_log.txt", glad_name);
}

// Get gladiator by base name (e.g., "G1")
Gladiator get_gladiator_by_name(const char* glad_name)
{
    Gladiator gladiator;
    char filename[64];
    get_gladiator_filename(glad_name, filename);

    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        gladiator.health = 0;
        gladiator.attack_power = 0;
        return gladiator;
    }
    fscanf(file, "%d, %d, %d, %d, %d", &gladiator.health, &gladiator.attack_power,
           &gladiator.opponents[0], &gladiator.opponents[1], &gladiator.opponents[2]);
    fclose(file);
    return gladiator;
}

int get_opponent_attack(int opp_no) {
    char opp_name[10];
    sprintf(opp_name, "G%d", opp_no);
    Gladiator opp = get_gladiator_by_name(opp_name);
    return opp.attack_power;
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <gladiator_name> (e.g., G1)\n", argv[0]);
        return 1;
    }
    
    const char* gladiator_name = argv[1];
    
    Gladiator my_glad = get_gladiator_by_name(gladiator_name);
    
    // open log file
    char log_filename[64];
    get_gladiator_log_filename(gladiator_name, log_filename);
    FILE *logFile = fopen(log_filename, "w");
    if (logFile == NULL)
    {
        perror("Error opening log file");
        return 1;
    }
    
    int health = my_glad.health;
    // print here the pid as I mentioned
    int pid = getpid();
    // write pid to log file
    fprintf(logFile, "Gladiator process started. %d:\n", pid);
    
    while (health > 0) {
        for (int i = 0; i < 3; i++) {
            int opponent_attack = get_opponent_attack(my_glad.opponents[i]);
            fprintf(logFile, "Facing opponent %d... Taking %d damage\n", my_glad.opponents[i], opponent_attack);
            health -= opponent_attack;
            if (health > 0) {
                fprintf(logFile, "Are you not entertained? Remaining health: %d\n", health);
            } else {
                fprintf(logFile, "The gladiator has fallen... Final health: %d\n", health);
                break;
            }
        }
    }
    
    fclose(logFile);
    return 0;
}