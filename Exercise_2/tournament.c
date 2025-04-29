#define NUM_GLADIATORS 4


char* gladiator_names[NUM_GLADIATORS] = {"Maximus", "Lucius", "Commodus", "Spartacus"};
char* gladiator_files[NUM_GLADIATORS] = {"G1", "G2", "G3", "G4"};

// fork a child for each glidator
// use exec to launch a process for each gladiator (pass matching gladiator file name as an argument)

// wait for the child process to complete and determine the winner based
// on whose gladiator process terminated last

// fscanf for G{i}
