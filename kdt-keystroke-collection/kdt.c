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

// Session structs are put on the stack, so they don't need to be freed, 
// but their members do need to be freed.
void cleanup(struct session *sessions, size_t sessions_length) {
	struct session *current_session; 
	for(size_t i = 0; i < sessions_length; i++) {
		current_session = &sessions[i];

		// (1) Free keystrokes
		if(current_session->keystrokes != NULL)
			free(current_session->keystrokes);

		// (2) Free time deltas
		if(current_session->time_deltas != NULL) 
			free(current_session->time_deltas);

		// (3) Free dwell times 
		if(current_session->dwell_times != NULL) 
			free(current_session->dwell_times);
		
		// (4) Free flight times
		if(current_session->flight_times != NULL) 
			free(current_session->flight_times);
	}
}


int main(int argc, char **argv) {
	// Provide usage instructions (e.g. --help) if no arguments are provided
	if(argc < 2) {
		display_help_text();
		exit(EXIT_FAILURE);
	}
	
	// Parse command line arguments
	enum kdt_error error_code = KDT_NO_ERROR;

	struct user_info *user_info = malloc(sizeof(struct user_info));
	if(user_info == NULL) {
		fprintf(stderr, "Failed to allocate memory for user_info.\n");
		exit(EXIT_FAILURE);
	}
	user_info->typing_duration = 0;


	short number_of_tests = 0;
	char output_file_path[64];
	FILE *output_file_fh = NULL;
	char device_file_path[64];
	byte mode = MODE_FREE_TEXT;

	error_code = parse_command_line_arguments(user_info->user, user_info->email, user_info->major, &mode, &number_of_tests, &user_info->typing_duration, device_file_path, output_file_path, output_file_fh, argc, argv);
	switch(error_code) {
		case KDT_NO_ERROR:
			break;
		case KDT_HELP_REQUEST:
			display_help_text();
			exit(EXIT_FAILURE);
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
	display_environment_details(user_info->user, user_info->email, user_info->major, user_info->typing_duration, number_of_tests, output_file_path, device_file_path, mode);

	// Initialize shared data for timer fucntion
	struct timer_state timer = {
		.flag = true,
		.seconds = user_info->typing_duration
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
	// make sure members are initialized. This can cause problems later otherwise
	for(byte i = 0; i < number_of_tests; i++) {
		sessions[i].keystrokes = NULL;
		sessions[i].keystrokes_length = 0;

		sessions[i].time_deltas = NULL;
		sessions[i].time_deltas_length = 0;

		sessions[i].dwell_times = NULL;
		sessions[i].dwell_times_length = 0;

		sessions[i].flight_times = NULL;
		sessions[i].flight_times_length = 0;
	}
	byte sessions_length = 0;
	unsigned long *time_deltas;
	unsigned long *dwell_times;
    	unsigned long *flight_times;

	// Variables to track Shift state and Caps toggle
	int shift_pressed = 0;
	int caps_lock = 0;

	pthread_t timer_thread;
	for(int session_number = 0; session_number < number_of_tests; session_number++) {
		// Prompt
		printf("kdt$ "); 
		fflush(stdout);

		// Enter non-canonical mode without echoing to collect raw data
		disable_buffering_and_echoing();

		// Set timer appropriately
		timer.flag = true;

		// Create thread for timer function
		if(pthread_create(&timer_thread, NULL, (void*)timer_function, &timer) != 0) {
			fprintf(stderr, "Error creating timer thread.\n");
			cleanup(sessions, number_of_tests);
			exit(EXIT_FAILURE);
		}
	
		// Open event file for reading
		struct input_event ev;
		int fd = open(device_file_path, O_RDONLY);
		if (fd == -1) {
		    perror("Error opening device");
		    cleanup(sessions, number_of_tests);
		    exit(EXIT_FAILURE);
		}
	
		shift_pressed = 0;
		caps_lock = 0;
		
		// **Active keys map**: Track keys that are pressed but not yet released
		struct keystroke active_keys[KEY_MAX + 1];  // Store the active keys and their press times
		int active_keys_count = 0;

		// Actually collect the raw data
		while(timer.flag == true) {
			// Read from event file. If there is nothing *new* to read, don't do anything (continue)
			if (read(fd, &ev, sizeof(struct input_event)) <= 0) 
				continue;
			
			// New event was a key press or a key release
			if (ev.type == EV_KEY) {
				// Handle Shift Modifiers (Left Shift, Right Shift)
				if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
					// Track if the shift key is being held or released
					shift_pressed = ev.value;
					continue;
				}
				// Handle Capslock toggle (Pressing the capslock key)
				else if(ev.code == KEY_CAPSLOCK && ev.value == 1) {
					// Toggle stored flag
					caps_lock = !caps_lock;
					continue;
				}

				// Get ASCII code based on modifers (shift and capslock)
				int ascii_character = keycode_to_ascii(ev.code, shift_pressed, caps_lock);

				// Case 1: Key Pressed
				if (ev.value == 1) {
					clock_gettime(CLOCK_MONOTONIC, &(active_keys[ev.code].press_time));

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
				// Case 2: Key Released AND it is in active keys with a already set character
				else if (ev.value == 0 && (int) active_keys[ev.code].c != 0) {
					clock_gettime(CLOCK_MONOTONIC, &(active_keys[ev.code].release_time));

					// Store the full keystroke in the keystrokes array
					keystrokes[keystrokes_length] = active_keys[ev.code];
					keystrokes_length++;
					
					// Clear active key after release
					active_keys[ev.code].c = 0; // Clear active key
					active_keys_count--;
				}
			} // end ev.type == EV_KEY
		} // end data collection loop
	    
		// Close the event file we are reading from
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
					free(sessions[i].keystrokes);
				
				if(sessions[session_number].time_deltas != NULL) 
					free(sessions[i].time_deltas);
				
				if(sessions[session_number].dwell_times != NULL)
					free(sessions[i].dwell_times);

				if(sessions[i].flight_times != NULL)
					free(sessions[i].flight_times);
			}	
		}
		printf("[DEBUG] Session %d's keystrokes buffer was allocated successfully!\n", session_number + 1);
		printf("[DEBUG] Attempting to copy %zu keystrokes into session %d's keystrokes buffer...\n", keystrokes_length, session_number + 1);



		// Copy temporary keystrokes buffer into the current session
		memcpy(sessions[session_number].keystrokes, keystrokes, sizeof(struct keystroke) * keystrokes_length);
		printf("[DEBUG] Memory copied!\n");
		sessions[session_number].keystrokes_length = keystrokes_length;
		


		// Store TIME DELTAS in current session (there are always N-1 time deltas where N is the number of keystrokes) 
		printf("[DEBUG]Attempting to construct time_delta_array for session %d and have it point to the time deltas found with get_time_deltas_in_milliseconds...\n", session_number + 1);
		
		error_code = set_session_statistic_data(&sessions[session_number], STATISTIC_TIME_DELTAS); 

		if(error_code != KDT_NO_ERROR) {
			printf("[DEBUG]Something went wrong when setting the time delta information for session #%d. KDT error code was %d.\n", session_number + 1, error_code);
		}
		printf("[DEBUG]Successfully found time deltas for session #%d and set the time delta buffer.\n", session_number + 1);



		// Store DWELL TIMES in current session 	
		printf("[DEBUG]Attempting to construct dwell times array for session %d and have it point to the dwell times found with get_time_deltas_in_milliseconds...\n", session_number + 1);

		error_code = set_session_statistic_data(&sessions[session_number], STATISTIC_DWELL_TIMES);

		if(error_code != KDT_NO_ERROR) {
			printf("[DEBUG]Something went wrong when setting the dwell time information for session #%d. KDT error code was %d.\n", session_number + 1, error_code);
		}
		printf("[DEBUG]Successfully found dwell times for session #%d and set the dwell times buffer.\n", session_number + 1);



		// Store FLIGHT TIMES in current session (there are always N-1 flight times where N is the number of keystrokes) 
		printf("[DEBUG]Attempting to construct flight times array for session %d and have it point to the flight times found with get_flight_times_in_milliseconds...\n", session_number + 1);

		error_code = set_session_statistic_data(&sessions[session_number], STATISTIC_FLIGHT_TIMES);

		if(error_code != KDT_NO_ERROR) {
			printf("[DEBUG]Something went wrong when setting the flight time information for session #%d. KDT error code was %d.\n", session_number + 1, error_code);
		}
		printf("[DEBUG]Successfully found flight times for session #%d and set the flight times buffer.\n", session_number + 1);


		
		// Display data for current session
		time_deltas = sessions[session_number].time_deltas;
		dwell_times = sessions[session_number].dwell_times;
		flight_times = sessions[session_number].flight_times;

		// (1) Print the time deltas
		printf("Time deltas:\n");
		printf("%ld", time_deltas[0]);
		for(size_t i = 1; i < sessions[session_number].time_deltas_length; i++) {
			printf(", %ld", time_deltas[i]); 
		} 
		printf("\n");

		// (2) Print the dwell times
		printf("\nDwell Times:\n");
		printf("%ld", dwell_times[0]);
		for(size_t i = 1; i < sessions[session_number].keystrokes_length; i++) 
			printf(", %ld", dwell_times[i]);

		printf("\n");

		// (3) Print the flight times
		printf("\nFlight Times:\n");
		printf("%ld", flight_times[0]);
		for(size_t i = 1; i < sessions[session_number].keystrokes_length - 1; i++) 
			printf(", %ld", flight_times[i]);

		printf("\n");


		// Reset buffer for next run. There is no need to clear out the keystrokes buffer, because
		// the session struct has its keystrokes member set using memcpy.
		keystrokes_length = 0;

		// Prompt user before continuing to next test
		printf("\n[SYSTEM] Press ENTER to take next test... ");
		c = 1;
		while(c != '\n') 
			c = fgetc(stdin);

	} // end of main for loop for sessions

	// Open output file for writing (binary mode)
    	output_file_fh = fopen(output_file_path, "wb");
	if (output_file_fh == NULL) {
       		fprintf(stderr, "Error opening file \"%s\" for writing.\n", output_file_path);

		printf("Performing cleanup...");
		cleanup(sessions, number_of_tests);
		printf("\tdone!\n");

        	exit(EXIT_FAILURE);
    	}

	// Save session data
	if (save_sessions(output_file_fh, user_info, sessions, number_of_tests) != 0) {
		fprintf(stderr, "Error saving session data to file.\n");
		fclose(output_file_fh);

		printf("Performing cleanup...");
		cleanup(sessions, number_of_tests);
		printf("\tdone!\n");

		exit(EXIT_FAILURE);
	}
    	fclose(output_file_fh);
    	printf("Session data successfully saved to %s\n", output_file_path);

	printf("Program terminated [OK].\n");
	return 0;
}
