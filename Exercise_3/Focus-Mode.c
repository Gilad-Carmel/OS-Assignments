#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#define EMAIL_SIGNAL SIGUSR1
#define DELIVERY_SIGNAL SIGUSR2
#define DOORBELL_SIGNAL SIGPIPE

#define EMAIL_OUTCOME 

// handlers

void handle_signal(int signum) {
    if (signum == EMAIL_SIGNAL) {
    } else if (signum == REMINDER_SIGNAL) {
    } else if (signum == DOORBELL_SIGNAL) {
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
    printf("        Checking pending distractions...\n");
    printf("──────────────────────────────────────────────\n");
}
void printBackToFocusMode() {
    printf("──────────────────────────────────────────────\n");
    printf("             Back to Focus Mode.\n");
    printf("══════════════════════════════════════════════\n");
}

void printDistractionMenu() {
    printf("Simulate a distraction:\n");
    printf("  1 = Email notification\n");
    printf("  2 = Reminder to pick up delivery\n");
    printf("  3 = Doorbell Ringing\n");
    printf("  q = Quit\n");
    printf(">> \n\n");
} 

// generic send signal function
void sendSignal(int sig) {
    if (kill(getpid(), sig) == -1) {
        perror("Failed to send signal");
    }
}


void sendEmailSignal() {
    printf("Email notification sent.\n");
    sendSignal(EMAIL_SIGNAL);
}
void sendDeliverySignal() {
    printf("Reminder to pick up delivery sent.\n");
    sendSignal(DELIVERY_SIGNAL);
}
void sendDoorbellSignal() {
    printf("Doorbell ringing signal sent.\n");
    sendSignal(DOORBELL_SIGNAL);
}

void distractionMenu(int duration) {
    char choice;
    for (int i = 0; i < duration; i++) {
        printDistractionMenu();
        scanf(" %c", &choice);
        switch (choice) {
            case '1':
                sendEmailSignal();
                break;
            case '2':
                sendDeliverySignal();
                break;
            case '3':
                sendDoorbellSignal();
                break;
            case 'q':
                printf("Exiting distraction menu.\n");
                return;
            default:
                printf("Invalid choice. Please try again.\n");
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

void exitFocusMode() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, EMAIL_SIGNAL);
    sigaddset(&sigset, DELIVERY_SIGNAL);
    sigaddset(&sigset, DOORBELL_SIGNAL);

    if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) != 0) {
        perror("Failed to unblock signals");
    }
}

void check_pending(int sig, char *signame) {
    sigset_t sigset;

    if (sigpending(&sigset) != 0) {
        perror("sigpending() error\n");
    } else if (sigismember(&sigset, sig)) {
        printf("a %s signal is pending\n", signame);
    } else {
        printf("no %s signals are pending\n", signame);
    }
}

void pendingPhase() {
    printPendingDistractions();
    check_pending(EMAIL_SIGNAL, "email");
    check_pending(DELIVERY_SIGNAL, "delivery");
    check_pending(DOORBELL_SIGNAL, "doorbell");
}


void runFocusMode(int numOfRounds, int duration) {
    
    printf("Entering Focus Mode. All distractions are blocked.\n");
    for (int i = 0; i < numOfRounds; i++) {
        startFocusMode();
        printRoundStart(i + 1);
        distractionMenu(duration);
        exitFocusMode();
        pendingPhase();
        // sleep(duration);
        printBackToFocusMode();
    }
    printf("Focus Mode completed.\n");
}