#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define EMAIL_SIGNAL SIGUSR1
#define DELIVERY_SIGNAL SIGUSR2
#define DOORBELL_SIGNAL SIGPIPE

void async_safe_write(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

// handlers

void handle_signal(int signum) {
    if (signum == EMAIL_SIGNAL) {
        async_safe_write("[Outcome:] The TA announced: Everyone get 100 on the exercise!\n");
    } else if (signum == DELIVERY_SIGNAL) {
        async_safe_write("[Outcome:] You picked it up just in time.\n");
    } else if (signum == DOORBELL_SIGNAL) {
        async_safe_write("[Outcome:] Food delivery is here.\n");
    }
}

void setupSignalHandlers() {
    struct sigaction sa;

    sa.sa_handler = handle_signal;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(EMAIL_SIGNAL, &sa, NULL);
    sigaction(DELIVERY_SIGNAL, &sa, NULL);
    sigaction(DOORBELL_SIGNAL, &sa, NULL);
}

void printRoundStart(int roundNumber) {
    printf("══════════════════════════════════════════════\n");
    printf("                Focus Round %d                \n", roundNumber);
    printf("──────────────────────────────────────────────\n");
}
void printPendingDistractions() {
    printf("──────────────────────────────────────────────\n");
    printf("        Checking pending distractions...      \n");
    printf("──────────────────────────────────────────────\n");
}
void printBackToFocusMode() {
    printf("──────────────────────────────────────────────\n");
    printf("             Back to Focus Mode.              \n");
    printf("══════════════════════════════════════════════\n");
}

void printDistractionMenu() {
    printf("\n");
    printf("Simulate a distraction:\n");
    printf("  1 = Email notification\n");
    printf("  2 = Reminder to pick up delivery\n");
    printf("  3 = Doorbell Ringing\n");
    printf("  q = Quit\n");
    printf(">> ");
} 

// generic send signal function
void sendSignal(int sig) {
    if (kill(getpid(), sig) == -1) {
        perror("Failed to send signal");
    }
}


void distractionMenu(int duration) {
    char choice;
    for (int i = 0; i < duration; i++) {
        printDistractionMenu();
        scanf(" %c", &choice);
        switch (choice) {
            case '1':
                sendSignal(EMAIL_SIGNAL);
                break;
            case '2':
                sendSignal(DELIVERY_SIGNAL);
                break;
            case '3':
                sendSignal(DOORBELL_SIGNAL);
                break;
            case 'q':
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
    
}

// start focus mode, block all signals
void startFocusMode() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, EMAIL_SIGNAL);
    sigaddset(&sigset, DELIVERY_SIGNAL);
    sigaddset(&sigset, DOORBELL_SIGNAL);

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) != 0) {
        perror("Failed to block signals");
    }
}

void unblock_signal(int signum) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, signum);

    if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) != 0) {
        perror("Failed to unblock signals");
    }
}

int check_member(int sig, sigset_t *sigset) {
    if (sigismember(sigset, sig)) {
        return 1; // Signal is blocked
    } else {
        return 0; // Signal is not blocked
    }

}

void pendingPhase() {
    printPendingDistractions();
    sigset_t sigset;
    int distraction_count = 0;
    if (sigpending(&sigset) != 0) {
        perror("sigpending() error\n");
    }
    if (check_member(EMAIL_SIGNAL, &sigset)) {
        printf(" - Email notification is waiting.\n");
        unblock_signal(EMAIL_SIGNAL);
        distraction_count++;
    }
    if (check_member(DELIVERY_SIGNAL, &sigset)) {
        printf(" - You have a reminder to pick up your delivery.\n");
        unblock_signal(DELIVERY_SIGNAL);
        distraction_count++;
    }
    if (check_member(DOORBELL_SIGNAL, &sigset)) {
        printf(" - The doorbell is ringing.\n");
        unblock_signal(DOORBELL_SIGNAL);
        distraction_count++;
    }

    if (distraction_count == 0) {
        printf("No distractions reached you this round.\n");
    }
    
}


void runFocusMode(int numOfRounds, int duration) {
    setupSignalHandlers();
    printf("Entering Focus Mode. All distractions are blocked.\n");
    for (int i = 0; i < numOfRounds; i++) {
        startFocusMode();
        printRoundStart(i + 1);
        distractionMenu(duration);
        pendingPhase();
        // sleep(duration);
        printBackToFocusMode();
    }
    printf("\nFocus Mode complete. All distractions are now unblocked.\n");
}