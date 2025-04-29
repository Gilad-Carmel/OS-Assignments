// each gladiator
// health
// attack power
// opponent queue

// read stats from file
// fight your opponents by order while health > 0:
//      each fight deduct helath based on opp attack
//      log actions and health into G{gladno}_log.txt
//      if dies => exit with code 0

// gladiator log file:
//  record fights
//  message when defeated 

//print here the pid as I mentioned
while (health > 0) {
    for (int i = 0; i < 3; i++) {
        int opponent_attack = get_opponent_attack(opponents[i]);
        fprintf(logFile, "Facing opponent %d... Taking %d damage\n", opponents[i], opponent_attack);
        health -= opponent_attack;
        if (health > 0) {
            fprintf(logFile, "Are you not entertained? Remaining health: %d\n", health);
        } else {
            fprintf(logFile, "The gladiator has fallen... Final health: %d\n", health);
            break;
        }
    }
}