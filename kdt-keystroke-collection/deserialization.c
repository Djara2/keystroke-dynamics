#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libkdt.h"


/*
 * Function to deserialize the data and verify it was stored correctly
 * Takes in a file pointer to file to read from, 
 * a pointer to a pointer to hold the array of sessions,
 * and a pointer to the variable that store the number of sessions
 */
int load_sessions(FILE *file, struct user_info **user_info, struct session **sessions, size_t *session_count) {
     // Make sure file pointer is valid
    if (!file) {
        fprintf(stderr, "Invalid file pointer for loading sessions.\n");
        return -1;
    }

    // Allocate memory for user_info
    *user_info = malloc(sizeof(struct user_info));
    if (!(*user_info)) {
        return -1;
    }

    // Read user_info fields from file (64 bytes each for user, email, and major, and 2 bytes for typing_duration)
    fread((*user_info)->user, sizeof(char), 64, file);
    fread((*user_info)->email, sizeof(char), 64, file);
    fread((*user_info)->major, sizeof(char), 64, file);
    fread(&(*user_info)->typing_duration, sizeof(short), 1, file);

    // Read number of sessions (8 bytes)
    fread(session_count, sizeof(size_t), 1, file);
    *sessions = malloc(sizeof(struct session) * (*session_count));
    if (!(*sessions)) {
        return -1;
    }

    // Loop through the number of sessions (determined from reading the session_count bytes)
    for (size_t i = 0; i < *session_count; i++) {
        // Read number of keystrokes (8 bytes)
        fread(&(*sessions)[i].keystrokes_length, sizeof(size_t), 1, file);

        // Allocate memory for keystrokes
        (*sessions)[i].keystrokes = malloc(sizeof(struct keystroke) * (*sessions)[i].keystrokes_length);

        // Read keystrokes (33 Bytes total)
        for (size_t j = 0; j < (*sessions)[i].keystrokes_length; j++) {
            fread(&(*sessions)[i].keystrokes[j].c, sizeof(char), 1, file);  // Keystroke key (1 byte)
            fread(&(*sessions)[i].keystrokes[j].press_time.tv_sec, sizeof(long), 1, file);  // Press timestamp (8 bytes)
            fread(&(*sessions)[i].keystrokes[j].press_time.tv_nsec, sizeof(long), 1, file);   // Nanoseconds (8 bytes)
            fread(&(*sessions)[i].keystrokes[j].release_time.tv_sec, sizeof(long), 1, file);  // Release timestamp (8 bytes)
            fread(&(*sessions)[i].keystrokes[j].release_time.tv_nsec, sizeof(long), 1, file);   // Nanoseconds (8 bytes)
        }

        // Read time deltas length (8 Bytes)
        fread(&(*sessions)[i].time_deltas_length, sizeof(size_t), 1, file);
        // Read the time deltas using the length
        if ((*sessions)[i].time_deltas_length > 0) {
            (*sessions)[i].time_deltas = malloc(sizeof(unsigned long) * (*sessions)[i].time_deltas_length);
            fread((*sessions)[i].time_deltas, sizeof(unsigned long), (*sessions)[i].time_deltas_length, file);
        } else {
            (*sessions)[i].time_deltas = NULL;
        }

        // Read dwell times length
        fread(&(*sessions)[i].dwell_times_length, sizeof(size_t), 1, file);
        // Read the dwell times using the length
        if ((*sessions)[i].dwell_times_length > 0) {
            (*sessions)[i].dwell_times = malloc(sizeof(unsigned long) * (*sessions)[i].dwell_times_length);
            fread((*sessions)[i].dwell_times, sizeof(unsigned long), (*sessions)[i].dwell_times_length, file);
        } else {
            (*sessions)[i].dwell_times = NULL;
        }

        // Read flight times length
        fread(&(*sessions)[i].flight_times_length, sizeof(size_t), 1, file);
        // Read the dwell using the length
        if ((*sessions)[i].flight_times_length > 0) {
            (*sessions)[i].flight_times = malloc(sizeof(unsigned long) * (*sessions)[i].flight_times_length);
            fread((*sessions)[i].flight_times, sizeof(unsigned long), (*sessions)[i].flight_times_length, file);
        } else {
            (*sessions)[i].flight_times = NULL;
        }
    }

    return 0;
}

void print_sessions(struct session *sessions, size_t session_count) {
    for (size_t i = 0; i < session_count; i++) {
        printf("Session %zu:\n", i + 1);
        printf("  Keystrokes Length: %zu\n", sessions[i].keystrokes_length);
        for (size_t j = 0; j < sessions[i].keystrokes_length; j++) {
            printf("    Keystroke %zu: %c | Press Time: %ld.%ld | Release Time: %ld.%ld\n",
                   j, sessions[i].keystrokes[j].c,
                   sessions[i].keystrokes[j].press_time.tv_sec, sessions[i].keystrokes[j].press_time.tv_nsec,
                   sessions[i].keystrokes[j].release_time.tv_sec, sessions[i].keystrokes[j].release_time.tv_nsec);
        }

        printf("  Time Deltas: ");
        for (size_t j = 0; j < sessions[i].time_deltas_length; j++) {
            printf("%lu ", sessions[i].time_deltas[j]);
        }
        printf("\n");

        printf("  Dwell Times: ");
        for (size_t j = 0; j < sessions[i].dwell_times_length; j++) {
            printf("%lu ", sessions[i].dwell_times[j]);
        }
        printf("\n");

        printf("  Flight Times: ");
        for (size_t j = 0; j < sessions[i].flight_times_length; j++) {
            printf("%lu ", sessions[i].flight_times[j]);
        }
        printf("\n\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <session_data_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    struct user_info *user_info = NULL;  // Allocate for user info
    struct session *sessions = NULL;
    size_t session_count = 0;

    // Load user info and sessions from file
    if (load_sessions(file, &user_info, &sessions, &session_count) != 0) {
        fprintf(stderr, "Failed to load sessions.\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    fclose(file);

    // Print user info and sessions
    printf("User Info:\n");
    printf("  User: %s\n", user_info->user);
    printf("  Email: %s\n", user_info->email);
    printf("  Major: %s\n", user_info->major);
    printf("  Typing Duration: %d\n\n", user_info->typing_duration);

    // Print the sessions
    print_sessions(sessions, session_count);

    // Free allocated memory
    free(user_info);
    for (size_t i = 0; i < session_count; i++) {
        free(sessions[i].keystrokes);
        free(sessions[i].time_deltas);
        free(sessions[i].dwell_times);
        free(sessions[i].flight_times);
    }
    free(sessions);

    return EXIT_SUCCESS;
}


