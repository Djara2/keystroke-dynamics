#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <fcntl.h>
#include "libkdt.h"

int main(int argc, char **argv) {
	// Provide usage instructions (e.g. --help) if no arguments are provided
	if(argc < 2) {
		display_help_text();
		exit(EXIT_FAILURE);
	}
	
	// Parse command line arguments
	enum kdt_error error_code = KDT_NO_ERROR;
	char user[64];
	char email[64];
	char major[64];
	short typing_duration = 0;
	short number_of_tests = 0;
	char output_file_path[64];
	FILE *output_file_fh = NULL;
	char device_file_path[64];
	// FILE *device_file_fh = NULL; "I think this is replaced by int fd, created by cordus" - Dave
	byte mode = MODE_FREE_TEXT;

	error_code = parse_command_line_arguments(user, email, major, &mode, &number_of_tests, &typing_duration, device_file_path, output_file_path, output_file_fh, argc, argv);
	switch(error_code) {
		case KDT_NO_ERROR:
			break;
		case KDT_INVALID_ARGUMENT_VALUE:
			exit(EXIT_FAILURE);
			break;
		case KDT_INVALID_OUTPUT_FILE:
			exit(EXIT_FAILURE);
			break;
		case KDT_INSUFFICIENT_ARGUMENTS:
			fprintf(stderr, "Insufficient arguments were provided for the program to run. See the help text below (kdt --help)\n\n");
			display_help_text();
			exit(EXIT_FAILURE);
			break;
		default:
			fprintf(stderr, "Unhandled/unspecified error encountered while parsing command line arguments. Terminating program...\n");
			exit(EXIT_FAILURE);
			break;
	}

	//printf("The typing collection will last for %d seconds.\n", typing_duration);
	display_environment_details(user, email, major, typing_duration, number_of_tests, output_file_path, device_file_path, mode);

	// Initialize shared data for timer fucntion
	struct timer_state timer = {
		.flag = true,
		.seconds = typing_duration
	};
	
	size_t keystrokes_length = 0;
	size_t keystrokes_capacity = 2048;
	struct keystroke *keystrokes = malloc(sizeof(struct keystroke) * keystrokes_capacity);
	if(keystrokes == NULL) {
		fprintf(stderr, "Failed to allocate memory for initial 2048 keystrokes.\n");
		exit(EXIT_FAILURE);
	}

	unsigned char c;
	//unsigned char bytes_read = 0;

	struct session sessions[number_of_tests];
	byte sessions_length = 0;
	unsigned long *dwell_times;
    	unsigned long *flight_times;

	pthread_t timer_thread;
	for(int session_number = 0; session_number < number_of_tests; session_number++) {
		// Prompt
		printf("rawInputTool$ "); 
		fflush(stdout);

		// Enter non-canonical mode without echoing to collect raw data
		disable_buffering_and_echoing();

		// Set timer appropriately
		timer.flag = true;

		// Create thread for timer function
		if(pthread_create(&timer_thread, NULL, (void*)timer_function, &timer) != 0) {
			fprintf(stderr, "Error creating timer thread.\n");
			return 1;
		}

        struct input_event ev;
        int fd = open(device_file_path, O_RDONLY);
        if (fd == -1) {
            perror("Error opening device");
            return 1;
        }

        // Variable to track Shift state
        int shift_pressed = 0;

        // **Active keys map**: Track keys that are pressed but not yet released
        struct keystroke active_keys[KEY_MAX + 1];  // Store the active keys and their press times
        int active_keys_count = 0;

		// Actually collect the raw data
		while(timer.flag == true) {
            if (read(fd, &ev, sizeof(struct input_event)) > 0) {
                if (ev.type == EV_KEY) {
                    // Handle Shift Modifiers (Left Shift, Right Shift)
                    if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
                        shift_pressed = ev.value; // Track shift key (1=pressed, 0=released)
                        continue;
                    }

                    // Get ASCII code
                    int ascii_character = keycode_to_ascii(ev.code, shift_pressed);

                    // Key Pressed
                    if (ev.value == 1) {
                        clock_gettime(CLOCK_MONOTONIC, &(active_keys[ev.code].press_time));

                        //printf("Key Pressed: %c\n", (char) ascii_character);

                        // Handles backspace
                        if(ascii_character == '\b' && keystrokes_length > 0) {
                            active_keys[ev.code].c = 127;
                            active_keys_count++;

                            printf("\b \b");
                            fflush(stdout);

                        }
                        // Handles all other characters
                        else if (ascii_character && keystrokes_length < BUFFER_SIZE - 1) {
                            active_keys[ev.code].c = ascii_character;
                            active_keys_count++;
                            printf("%c", ascii_character);
                            fflush(stdout);
                        }

                    }
                    // Key Released
                    else if (ev.value == 0 && active_keys[ev.code].c != 0) {
                        clock_gettime(CLOCK_MONOTONIC, &(active_keys[ev.code].release_time));
                        //printf("Key Released: %c\n", (char) active_keys[ev.code].c);

                        // Store the full keystroke in the keystrokes array
                        keystrokes[keystrokes_length] = active_keys[ev.code];
                        keystrokes_length++;

                        // Clear active key after release
                        active_keys[ev.code].c = 0; // Clear active key
                        active_keys_count--;
                    }
                }
            }
    
        }

        close(fd);
        // Sort keystrokes based on press time to ensure correct order
        qsort(keystrokes, keystrokes_length, sizeof(struct keystroke), compare_keystrokes);
		
		// Restore canonical mode and echoing
		fflush(stdout);
		enable_buffering_and_echoing();

		// Print the numeric values of keys pressed for current session
		printf("\nNumeric codes entered:\n");
		printf("\n%d", (int) keystrokes[0].c);
		for(size_t i = 1; i < keystrokes_length ; i++)
			printf(", %d", (int) keystrokes[i].c);

		printf("\n");

		// Get time deltas for current session
		dwell_times = get_dwell_times_in_milliseconds(keystrokes, keystrokes_length);
		if(dwell_times == NULL) {
			fprintf(stderr, "Could not get the dwell times.\n"); 
		}

		// Print the dwell times
		printf("\nDwell Times:\n");
		printf("%ld", dwell_times[0]);
		for(size_t i = 1; i < keystrokes_length; i++) 
			printf(", %ld", dwell_times[i]);

		printf("\n");

        flight_times = get_flight_times_in_milliseconds(keystrokes, keystrokes_length);
        if(flight_times == NULL) {
            fprintf(stderr, "Could not get the flight times\n");
        }

        // Print the flight times
		printf("\nFlight Times:\n");
		printf("%ld", flight_times[0]);
		for(size_t i = 1; i < keystrokes_length - 1; i++) 
			printf(", %ld", flight_times[i]);

		printf("\n");
		
		// Join thread
		printf("[DEBUG] Attempting to join timer pthread...\n");
		pthread_join(timer_thread, NULL);
		printf("[DEBUG] Timer pthread joined!\n");

		// Move data into nearest, unused session struct
		printf("[DEBUG] Attempting to allocate memory for session %d's keystrokes buffer...\n", session_number + 1);
		sessions[session_number].keystrokes = malloc(sizeof(struct keystroke) * keystrokes_length);
		if(sessions[session_number].keystrokes == NULL) {
			fprintf(stderr, "Failed to allocate memory for session %d's keystrokes buffer.\n", session_number + 1);

			// Free other sessions to prevent memory leak
			for(int i = 0; i < session_number; i++) {
				if(sessions[session_number].keystrokes != NULL) 
					free(sessions[session_number].keystrokes);
				
				if(sessions[session_number].time_deltas != NULL) {
					free(sessions[session_number].time_deltas->values);
					free(sessions[session_number].time_deltas);
				}
			}	
		}
		printf("[DEBUG] Session %d's keystrokes buffer was allocated successfully!\n", session_number + 1);
		printf("[DEBUG] Attempting to copy %zu keystrokes into session %d's keystrokes buffer...\n", keystrokes_length, session_number + 1);
		memcpy(sessions[session_number].keystrokes, keystrokes, sizeof(struct keystroke) * keystrokes_length);
		printf("[DEBUG] Memory copied!\n");
		sessions[session_number].keystrokes_length = keystrokes_length;
		
		// Copy over time deltas (there are always N-1 time deltas where N is the number of keystrokes) 
		if(dwell_times != NULL) {
			printf("[DEBUG] Attempting to construct time_delta_array for session %d and have it point to the time deltas found with get_time_deltas_in_milliseconds...\n", session_number + 1);
			sessions[session_number].time_deltas = time_delta_array_create(dwell_times, keystrokes_length - 1, keystrokes_length - 1);
			printf("[DEBUG] Function call succeeded!\n");
			if(sessions[session_number].time_deltas == NULL)
				fprintf(stderr, "Failed to create new time delta array for session number %d.\n", session_number + 1);
		}	
		else
			sessions[session_number].time_deltas = NULL;

		// Reset buffer for next run. There is no need to clear out the keystrokes buffer, because
		// the session struct has its keystrokes member set using memcpy.
		keystrokes_length = 0;

		// Prompt user before continuing to next test
		printf("\n[SYSTEM] Press ENTER to take next test... ");
		c = 1;
		while(c != '\n') 
			c = fgetc(stdin);
	}

	printf("Program terminated.\n");
	return 0;
}
