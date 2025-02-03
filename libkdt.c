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
#include <stdint.h>
#include "libkdt.h"

void disable_buffering_and_echoing() {
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	// disable canonical mode and echo
	t.c_lflag &= ~(ICANON | ECHO); 
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void enable_buffering_and_echoing() {
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	// Re-enable canonical mode and echo
	t.c_lflag |= (ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void timer_function(void* arg) {
	struct timer_state *data = (struct timer_state*) arg;

	sleep(data->seconds);

	// pthread_mutex_lock(&data->lock);
	data->flag = false;
}

// Statistic function #1: Get time deltas
unsigned long* get_time_deltas_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length) {
	if(keystrokes == NULL) return NULL;
	
	if(keystrokes_length < 2) {
		fprintf(stderr, "[get_time_deltas_in_milliseconds] Time deltas cannot be found with a keystrokes buffer that is less than 2 keystrokes long.\n");
		return NULL;
	}

	unsigned long *time_deltas = malloc(sizeof(unsigned long) * (keystrokes_length - 1) );
	if(time_deltas == NULL) {
		printf("[get_time_deltas_in_milliseconds] Error allocating memory for time deltas buffer.\n");
		return NULL;
	}
	
	// Actually get dwell times
	for(size_t i = 0; i < keystrokes_length - 1; i++) {
		unsigned long whole_seconds_difference_in_ms = 1000 * (keystrokes[i+1].press_time.tv_sec - keystrokes[i].press_time.tv_sec);
		unsigned long nanoseconds_difference_in_ms   = (keystrokes[i+1].press_time.tv_nsec - keystrokes[i].press_time.tv_nsec) / 1000000;
		time_deltas[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;	
	}

	return time_deltas;
}

// Statistics function #2: Dwell times (time between press and release of one key)
unsigned long* get_dwell_times_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length) {
	if(keystrokes == NULL) return NULL;

	if(keystrokes_length < 1) {
		fprintf(stderr, "[get_dwell_times_in_milliseconds] Dwell times cannot be found with a keystrokes buffer that is less than 1 keystrokes long.\n");
		return NULL;
	}
	
	unsigned long *dwell_times = malloc(sizeof(unsigned long) * (keystrokes_length));
	if(dwell_times == NULL) {
		printf("Error allocating memory for time deltas buffer.\n");
		return NULL;
	}

	// Actually get dwell times
	for(size_t i = 0; i < keystrokes_length; i++) {
		unsigned long whole_seconds_difference_in_ms = 1000 * (keystrokes[i].release_time.tv_sec - keystrokes[i].press_time.tv_sec);
		unsigned long nanoseconds_difference_in_ms   = (keystrokes[i].release_time.tv_nsec - keystrokes[i].press_time.tv_nsec) / 1000000;
		dwell_times[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;	
	}

	return dwell_times;
}

// Statistics function #3: Flight times (time between key-up of one key and key-down of another key)
unsigned long* get_flight_times_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length) {
	if(keystrokes == NULL) return NULL;

	if(keystrokes_length < 2) {
		fprintf(stderr, "[get_flight_times_in_milliseconds] Flight times cannot be found with a keystrokes buffer that is less than 2 keystrokes long.\n");
		return NULL;
	}

	unsigned long *flight_times = malloc(sizeof(unsigned long) * (keystrokes_length - 1));
	if(flight_times == NULL) {
		printf("Error allocating memory for flight times buffer.\n");
		return NULL;
	}
		
	// Actually find flight times
	for(size_t i = 0; i < keystrokes_length - 1; i++) {
		if(keystrokes[i + 1].press_time.tv_sec > keystrokes[i].release_time.tv_sec || keystrokes[i + 1].press_time.tv_nsec > keystrokes[i].release_time.tv_nsec) {
			unsigned long whole_seconds_difference_in_ms = 1000 * (keystrokes[i + 1].press_time.tv_sec - keystrokes[i].release_time.tv_sec);
			unsigned long nanoseconds_difference_in_ms   = (keystrokes[i + 1].press_time.tv_nsec - keystrokes[i].release_time.tv_nsec) / 1000000;
			flight_times[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;
		}
		else {
			unsigned long whole_seconds_difference_in_ms = 1000 * (keystrokes[i].release_time.tv_sec - keystrokes[i + 1].press_time.tv_sec);
			unsigned long nanoseconds_difference_in_ms   = (keystrokes[i].release_time.tv_nsec - keystrokes[i + 1].press_time.tv_nsec) / 1000000;
			flight_times[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;
		}
	}
	
	return flight_times;
}

// Efficiently set sessions with data. More concise than what we were doing before
enum kdt_error set_session_statistic_data( struct session *s, enum kdt_statistic statistic_code) {
	if(s == NULL) { 
		fprintf(stderr, "[set_session_statistic_data] Cannot use a session struct pointer that points to NULL.\n");
		return KDT_INVALID_ARGUMENT_VALUE;
	}
	
	// Determine what statistic we are calculating, then set
	unsigned long *statistic_array;
	size_t *statistic_array_length;
	unsigned long* (*statistics_function)(struct keystroke*, size_t);
	switch(statistic_code) {
		case STATISTIC_TIME_DELTAS:
			statistic_array_length = &(s->time_deltas_length);
			// Get statistics, then set statistics array and length member
			statistics_function = get_time_deltas_in_milliseconds;
			statistic_array = statistics_function(s->keystrokes, s->keystrokes_length);
			
			s->time_deltas = statistic_array;
			s->time_deltas_length = s->keystrokes_length - 1;	// times between keystrokes, so there are n-1 of these
			break;

		case STATISTIC_DWELL_TIMES:
			statistic_array_length = &(s->dwell_times_length);
			// Get statistics, then set statistics array and length member
			statistics_function = get_dwell_times_in_milliseconds;
			statistic_array = statistics_function(s->keystrokes, s->keystrokes_length);

			s->dwell_times = statistic_array;
			s->dwell_times_length = s->keystrokes_length;
			break;

		case STATISTIC_FLIGHT_TIMES:		
			statistic_array_length = &(s->flight_times_length);
			// Get statistics, then set statistics array and length member
			statistics_function = get_flight_times_in_milliseconds;
			statistic_array = statistics_function(s->keystrokes, s->keystrokes_length);
			
			s->flight_times = statistic_array;
			s->flight_times_length = s->keystrokes_length - 1;	// times between keystrokes, so there are n-1 of these
			break;

		default:
			fprintf(stderr, "[set_session_statistic_data] kdt_statistic code \"%d\" is invalid.\n", statistic_code);
			return KDT_INVALID_ARGUMENT_VALUE;
			break;
	}

	// If something went wrong, then our statistics buffer was set to NULL.
	if(statistic_array == NULL) {
		fprintf(stderr, "[set_session_statistic_data] The statistics function associated with kdt_statistic code \"%d\" returned NULL.\n", statistic_code);
		(*statistic_array_length) = 0;
		return KDT_NULL_ERROR;
	}
	
	return KDT_NO_ERROR;
}


void display_help_text() {
	FILE *help_fh = fopen("res/help.txt", "r");
	if(help_fh == NULL) {
		fprintf(stderr, "Failed to open res/help.txt. Was the file deleted or moved?\n");
		return;
	}
	size_t help_fh_contents_length = 0;
	size_t help_fh_contents_capacity = 512;
	char *help_fh_contents = malloc(sizeof(char) * help_fh_contents_capacity);
	char c = fgetc(help_fh);

	// get contents of help.txt
	while(c != EOF) {
		help_fh_contents[help_fh_contents_length] = c;
		help_fh_contents_length++;
		if(help_fh_contents_length >= help_fh_contents_capacity) {
			help_fh_contents_capacity *= 2;
			help_fh_contents = realloc(help_fh_contents, sizeof(char) * help_fh_contents_capacity);
			if(help_fh_contents == NULL) {
				fprintf(stderr, "Failed to allocate more memory for contents of help.txt.\n");
			}
		}

		c = fgetc(help_fh);
	}

	// Close file and display contents
	fclose(help_fh);
	printf("%s\n", help_fh_contents);
	free(help_fh_contents);
}

void display_environment_details(char user[], char email[], char major[], int duration, short number_of_samples, char output_file_path[], char device_file_path[], byte mode) {
	printf("Environment:\n\tUser: %s\n\tEmail: %s\n\tMajor: %s\n\tTyping duration: %d\n\tSamples to take: %hd\n\tOutput file: %s\n\tInput device file: %s\n\tMode: %d (%s)\n\n",

	user, 
	email, 
	major, 
	duration, 
	number_of_samples, 
	output_file_path, 
	device_file_path,
	mode,
	mode == 0 ? "Free" : "Fixed"
	);
}

enum kdt_error parse_command_line_arguments(char *user, char *email, char *major, byte *mode, short *number_of_tests, short *typing_duration, char *device_file_path, char *output_file_path, FILE *output_file_fh, int argc, char **argv) {	
	// Use "any" logic on this buffer. If any are false, then the program cannot run.
	bool fulfilled_arguments[REQUIRED_ARGUMENTS_COUNT];
	for(char i = 0; i < REQUIRED_ARGUMENTS_COUNT; i++) 
		fulfilled_arguments[i] = false;

	// Find all lengths ahead of time to make bounds checking easier
	uint16_t token_lengths[argc];
	for(byte i = 0; i < argc; i++) 
		token_lengths[i] = strlen(argv[i]);
	
	uint8_t            token_number = 1;
	uint16_t           token_index = 0;
	char               *current_token = argv[token_number];
	enum cli_sm_state  current_state = CLI_SM_READ_PARAM;
	enum kdt_error     error_code = KDT_NO_ERROR;
	void               *current_parameter;
	uint8_t            match_start_index = 1;
	enum kdt_parameter current_parameter_type = KDT_PARAM_NONE;
	while(token_number < argc) {
		current_token = argv[token_number];
		printf("[DEBUG] The current token is \"%s\".\n", current_token);
		switch(current_state) {
			case CLI_SM_READ_PARAM:
				// Parameter identifiers must be at least 2 chars long.  
				if(token_lengths[token_number] < 2) {
					current_state = CLI_SM_ERROR_PARAM_TOO_SHORT;
					current_parameter_type = KDT_PARAM_NONE;
					debug_state(current_token, current_parameter_type, current_state);
					break;
				}
				
				// Parameter identifiers must start with one hyphen or two at most
				if(current_token[0] != '-') {
					current_state = CLI_SM_ERROR_PARAM_MALFORMED;
					current_parameter_type = KDT_PARAM_NONE;
					debug_state(current_token, current_parameter_type, current_state);
					break;
				}

				// Allows for shared logic of short ID matching
				// and long ID autocompletion
				if(current_token[1] == '-')
					match_start_index = LONG_ID_MATCH_START;
				else
					match_start_index = SHORT_ID_MATCH_START;

				// Discern short identifier or long identifier
				switch(current_token[match_start_index]) { 
					// Help
					case 'h': 
						// No need to go to next token or adjust parameter
						// type, just display help text and exit program.
						current_state = CLI_SM_DISPLAY_HELP_TEXT;
						break;
					// Username
					case 'u':
						current_parameter_type = KDT_PARAM_USER;
						current_state = CLI_SM_READ_VALUE;

						token_number++;
						break;

					// Email
					case 'e':
						current_parameter_type = KDT_PARAM_EMAIL;
						current_state = CLI_SM_READ_VALUE;

						debug_state(current_token, current_parameter_type, current_state);

						token_number++;
						break;

					// Major
					case 'm':
						current_parameter_type = KDT_PARAM_MAJOR;
						current_state = CLI_SM_READ_VALUE;

						debug_state(current_token, current_parameter_type, current_state);

						token_number++;
						break;

					// Number of tests
					case 'n':
						current_parameter_type = KDT_PARAM_REPETITIONS;
						current_state = CLI_SM_READ_VALUE;

						debug_state(current_token, current_parameter_type, current_state);

						token_number++;
						break;

					// Duration
					case 'd':
						// -d alone maps to --duration
						if(match_start_index == SHORT_ID_MATCH_START) {
							current_parameter_type = KDT_PARAM_DURATION;
							current_state = CLI_SM_READ_VALUE;

							debug_state(current_token, current_parameter_type, current_state);

							token_number++;
							break;
						}

						// to be either "--duration" or "--device-file", the token will
						// have to be at least 10 characters long 
						if(token_lengths[token_number] < 10) {
							current_state = CLI_SM_ERROR_INVALID_PARAM;
							current_parameter_type = KDT_PARAM_NONE;

							debug_state(current_token, current_parameter_type, current_state);
							break;
						}

						// long identifier autocomplete
						switch(current_token[3]) {
							case 'u':
								current_parameter_type = KDT_PARAM_DURATION;
								token_number++;
								current_state = CLI_SM_READ_VALUE;

								debug_state(current_token, current_parameter_type, current_state);
								break;

							case 'e':
								current_parameter_type = KDT_PARAM_DEVICE_FILE;
								token_number++;
								current_state = CLI_SM_READ_VALUE;

								debug_state(current_token, current_parameter_type, current_state);
								break;
							default:
								current_state = CLI_SM_ERROR_INVALID_PARAM;
								current_parameter_type = KDT_PARAM_NONE;

								debug_state(current_token, current_parameter_type, current_state);
								break;
						}
						break;

					// Output
					case 'o':
						current_parameter_type = KDT_PARAM_OUTPUT_FILE;
						
						// Advance to next token, which we expect to be the value for
						// the parameter we just dealt with.
						token_number++;
						current_state = CLI_SM_READ_VALUE;


						debug_state(current_token, current_parameter_type, current_state);
						break;

					// Free text
					case 'f':
						// this one has ambiguity between short and long identifiers.
						// --f does not necessarily map to --free, it could also map 
						//     to --fixed
						if(match_start_index == SHORT_ID_MATCH_START) {
							(*mode) = MODE_FREE_TEXT;

							// since this parameter takes no value, we skip
							// to the next token and the mode remains in 
							current_parameter_type = KDT_PARAM_NONE;
							current_state = CLI_SM_READ_PARAM;
							token_number++;
								
							// SINCE THIS PARAMETER TAKES NO VALUE, THE RESPONSIBILITY 
							// OF UPDATING THE FULFILLED BUFFER NEEDS TO BE COMPLETED HERE
							fulfilled_arguments[REQUIRED_ARG_MODE] = true;

							debug_state(current_token, current_parameter_type, current_state);
							break;
						}
						// to be either "--free" or "--fixed", the token will have to be 
						// at least 6 characters long.
						if(token_lengths[token_number] < 6) {
							current_state = CLI_SM_ERROR_INVALID_PARAM;
							current_parameter_type = KDT_PARAM_NONE;

							debug_state(current_token, current_parameter_type, current_state);
							break;
						}

						// long identifier autocomplete
						switch(current_token[3]) {
							case 'r':
								// Set parameter
								(*mode) = MODE_FREE_TEXT;
								current_parameter_type = KDT_PARAM_NONE;

								// Next we expect a parameter ID
								current_state = CLI_SM_READ_PARAM;
								token_number++;

								debug_state(current_token, current_parameter_type, current_state);
								break;

							case 'i':
								(*mode) = MODE_FIXED_TEXT;
								current_parameter_type = KDT_PARAM_NONE;

								// This takes no value, so we expect to just read another
								// parameter after this.
								current_state = CLI_SM_READ_PARAM;
								token_number++; 


								debug_state(current_token, current_parameter_type, current_state);
								break;

							// User misspelled ID completely
							default:
								current_state = CLI_SM_ERROR_INVALID_PARAM;
								current_parameter_type = KDT_PARAM_NONE;

								debug_state(current_token, current_parameter_type, current_state);
								break;
						}
						break;

					// Fixed text
					case 'x':
						// Set "mode" variable as MODE_FIXED_TEXT (1)
						(*mode) = MODE_FIXED_TEXT;
						current_parameter_type = KDT_PARAM_NONE;

						// Advance to next token
						token_number++;

						// We expect to read a parameter ID after this
						current_state = CLI_SM_READ_PARAM;

						// SINCE THIS PARAMETER TAKES NO VALUE, THE RESPONSIBILITY 
						// OF UPDATING THE FULFILLED BUFFER NEEDS TO BE COMPLETED HERE
						fulfilled_arguments[REQUIRED_ARG_MODE] = true;

						debug_state(current_token, current_parameter_type, current_state);
						break;

					// Device file
					case 'v':
						current_parameter_type = KDT_PARAM_DEVICE_FILE;
						token_number++;
						current_state = CLI_SM_READ_VALUE;

						debug_state(current_token, current_parameter_type, current_state);
						break;

					// Invalid parameter
					default:
						current_state = CLI_SM_ERROR_INVALID_PARAM;
						break;
				}
				break;

			// Expecting to read a value for previously identified parameter
			case CLI_SM_READ_VALUE:
				switch(current_parameter_type) {
					case KDT_PARAM_USER:
						if(token_lengths[token_number] >= 64) {
							current_state = CLI_SM_ERROR_VALUE_TOO_LONG;
							current_parameter_type = KDT_PARAM_NONE;
							break;
						}
						// Set value
						strcpy(user, current_token);
						
						// Update fulfilled arguments
						fulfilled_arguments[REQUIRED_ARG_USER] = true;

						// Move onto next token
						token_number++;

						// We now expect a parameter ID
						current_state = CLI_SM_READ_PARAM;
						break;

					case KDT_PARAM_EMAIL:
						if(token_lengths[token_number] >= 64) {
							current_state = CLI_SM_ERROR_VALUE_TOO_LONG;
							break;
						}
						// Set value
						strcpy(email, current_token);
						
						// Update fulfilled arguments
						fulfilled_arguments[REQUIRED_ARG_EMAIL] = true;

						// Move onto next token
						token_number++;

						// We now expect a parameter ID
						current_state = CLI_SM_READ_PARAM;
						break;

					case KDT_PARAM_MAJOR:
						if(token_lengths[token_number] >= 64) {
							current_state = CLI_SM_ERROR_VALUE_TOO_LONG;
							break;
						}
						// Set value
						strcpy(major, current_token);
						
						// Update fulfilled arguments
						fulfilled_arguments[REQUIRED_ARG_MAJOR] = true;

						// Move onto next token
						token_number++;

						// We now expect a parameter ID
						current_state = CLI_SM_READ_PARAM;

						break;

					case KDT_PARAM_DURATION:
						// Set value
						(*typing_duration) = atoi(current_token);
						if( (*typing_duration) <= 0 ) {
							current_state = CLI_SM_ERROR_NAN;
							break;
						}

						// Update fulfilled arguments	
						fulfilled_arguments[REQUIRED_ARG_TYPING_DURATION] = true;

						// Move onto next token
						token_number++;

						// We now expect a parameter ID
						current_state = CLI_SM_READ_PARAM;
						break;

					case KDT_PARAM_REPETITIONS:
						// Set value
						(*number_of_tests) = atoi(current_token);
						if( (*number_of_tests) <= 0 ) {
							current_state = CLI_SM_ERROR_NAN;
							break;
						}

						// Update fulfilled arguments
						fulfilled_arguments[REQUIRED_ARG_NUMBER_OF_TESTS] = true;

						// Move onto next token
						token_number++;

						// We now expect a parameter ID
						current_state = CLI_SM_READ_PARAM;
						break;

					case KDT_PARAM_OUTPUT_FILE:
						// Check that file can actually be opened
						output_file_fh = fopen(current_token, "w");
						if(output_file_fh == NULL) {
							fprintf(stderr, "Could not open file \"%s\" for writing.\n", output_file_path);
							current_state = CLI_SM_ERROR_VALUE_RESOURCE_NON_WRITABLE;
							break;
						}
						fclose(output_file_fh);
						
						// Set value
						strcpy(output_file_path, current_token);

						// Update fulfilled arguments
						fulfilled_arguments[REQUIRED_ARG_OUTPUT_FILE_PATH] = true;

						// Move onto next token
						token_number++;

						// We now expect a parameter ID
						current_state = CLI_SM_READ_PARAM;
						break;

					case KDT_PARAM_DEVICE_FILE:
						// Check that string is not too long
						if( token_lengths[token_number] >= 64 ) {
							current_state = CLI_SM_ERROR_VALUE_TOO_LONG;
							break;
						}

						// Check that the file exists
						if( access(current_token, F_OK) != 0 ) {
							current_state = CLI_SM_ERROR_VALUE_RESOURCE_NON_EXISTENT; 
							break;
						}

						// Check that the file can be read
						if( access(current_token, R_OK) != 0 ) {
							current_state = CLI_SM_ERROR_VALUE_RESOURCE_NON_WRITABLE;
							break; 
						}
						
						// Set value
						strcpy(device_file_path, current_token);

						// update fulfilled arguments
						fulfilled_arguments[REQUIRED_ARG_DEVICE_FILE] = true;

						// Move onto next token
						token_number++; 

						// We now expect a parameter ID
						current_state = CLI_SM_READ_PARAM;
						break;
				}
				break;

			case CLI_SM_DISPLAY_HELP_TEXT:
				return KDT_HELP_REQUEST;
				break;

			case CLI_SM_ERROR_PARAM_TOO_SHORT:
				fprintf(stderr, "Provided parameter \"%s\" is too short. Parameters must be at least 2 characters long.\n", current_token);
				return KDT_INVALID_PARAMETER;
				break;

			case CLI_SM_ERROR_PARAM_MALFORMED:
				fprintf(stderr, "Provided parameter \"%s\" is malformed. Parameters must start with either one hyphen or two hyphens (e.g. \"-u\" or \"--username\")\n", current_token);
				return KDT_INVALID_PARAMETER;
				break;

			case CLI_SM_ERROR_INVALID_PARAM:
				fprintf(stderr, "Provided parameter \"%s\" is not a recognized parameter. Use \"kdt --help\" to see a list of valid parameters.\n", current_token);
				return KDT_INVALID_PARAMETER;
				break;

			case CLI_SM_ERROR_VALUE_TOO_LONG:
				fprintf(stderr, "Provided value \"%s\" is too long. Values for string parameters may only be up to 63 characters long.\n", current_token);
				return KDT_INVALID_ARGUMENT_VALUE;
				break;

			case CLI_SM_ERROR_NAN:
				fprintf(stderr, "Provided value \"%s\" is either not a number or is a number that is zero or non-positive. This value must be a non-zero, positive integer.\n", current_token);
				return KDT_INVALID_ARGUMENT_VALUE;
				break;
			
			case CLI_SM_ERROR_VALUE_RESOURCE_NON_WRITABLE:
				fprintf(stderr, "Provided value \"%s\" is not the path to a file that can be opened by this program for writing.\n", current_token);
				return KDT_INVALID_ARGUMENT_VALUE;
				break;

			case CLI_SM_ERROR_VALUE_RESOURCE_NON_EXISTENT:
				fprintf(stderr, "Provided value \"%s\" is not a file that exists or can be accessed by this process.\n", current_token);
				return KDT_INVALID_ARGUMENT_VALUE;
				break;

			case CLI_SM_ERROR_VALUE_RESOURCE_NON_READABLE:
				fprintf(stderr, "Provided value \"%s\" is not a file that can be read by this process. Maybe this process needs to be executed as a superuser?\n", current_token);
				return KDT_INVALID_ARGUMENT_VALUE;
				break;
			
			default:
				fprintf(stderr, "Unhandled case for state %d reached.\n", current_state);
				return KDT_UNHANDLED_ERROR;
				break;

		} // end switch(current state)
	} // end while
	
	// Check all required arguments are fulfilled
	for(int i = 0; i < REQUIRED_ARGUMENTS_COUNT; i++) {
		if(fulfilled_arguments[i] == true) 
			continue;
		
		return KDT_INSUFFICIENT_ARGUMENTS;
	}
	// Internal logic should return a kdt_error value other than KDT_NO_ERROR when 
	// something goes wrong. If this is reached, then no errors occurred.
	return KDT_NO_ERROR;
}

// Convert keycode to ASCII considering Shift
int keycode_to_ascii(int keycode, int shift, int caps_lock) {
    static char lower_map[KEY_MAX + 1] = {0};
    static char upper_map[KEY_MAX + 1] = {0};
    static int initialized = 0;  // Flag to check if initialized

    // Populate lower_map (normal keys)
    if(!initialized) {
        initialized = 1;

        lower_map[KEY_1] = '1'; lower_map[KEY_2] = '2'; lower_map[KEY_3] = '3';
        lower_map[KEY_4] = '4'; lower_map[KEY_5] = '5'; lower_map[KEY_6] = '6';
        lower_map[KEY_7] = '7'; lower_map[KEY_8] = '8'; lower_map[KEY_9] = '9';
        lower_map[KEY_0] = '0';

        lower_map[KEY_Q] = 'q'; lower_map[KEY_W] = 'w'; lower_map[KEY_E] = 'e';
        lower_map[KEY_R] = 'r'; lower_map[KEY_T] = 't'; lower_map[KEY_Y] = 'y';
        lower_map[KEY_U] = 'u'; lower_map[KEY_I] = 'i'; lower_map[KEY_O] = 'o';
        lower_map[KEY_P] = 'p';

        lower_map[KEY_A] = 'a'; lower_map[KEY_S] = 's'; lower_map[KEY_D] = 'd';
        lower_map[KEY_F] = 'f'; lower_map[KEY_G] = 'g'; lower_map[KEY_H] = 'h';
        lower_map[KEY_J] = 'j'; lower_map[KEY_K] = 'k'; lower_map[KEY_L] = 'l';

        lower_map[KEY_Z] = 'z'; lower_map[KEY_X] = 'x'; lower_map[KEY_C] = 'c';
        lower_map[KEY_V] = 'v'; lower_map[KEY_B] = 'b'; lower_map[KEY_N] = 'n';
        lower_map[KEY_M] = 'm';

        lower_map[KEY_SPACE] = ' ';
        lower_map[KEY_ENTER] = '\n';
        lower_map[KEY_BACKSPACE] = '\b';

        // Populate upper_map (Shifted keys)
        upper_map[KEY_1] = '!'; upper_map[KEY_2] = '@'; upper_map[KEY_3] = '#';
        upper_map[KEY_4] = '$'; upper_map[KEY_5] = '%'; upper_map[KEY_6] = '^';
        upper_map[KEY_7] = '&'; upper_map[KEY_8] = '*'; upper_map[KEY_9] = '(';
        upper_map[KEY_0] = ')';

        upper_map[KEY_Q] = 'Q'; upper_map[KEY_W] = 'W'; upper_map[KEY_E] = 'E';
        upper_map[KEY_R] = 'R'; upper_map[KEY_T] = 'T'; upper_map[KEY_Y] = 'Y';
        upper_map[KEY_U] = 'U'; upper_map[KEY_I] = 'I'; upper_map[KEY_O] = 'O';
        upper_map[KEY_P] = 'P';

        upper_map[KEY_A] = 'A'; upper_map[KEY_S] = 'S'; upper_map[KEY_D] = 'D';
        upper_map[KEY_F] = 'F'; upper_map[KEY_G] = 'G'; upper_map[KEY_H] = 'H';
        upper_map[KEY_J] = 'J'; upper_map[KEY_K] = 'K'; upper_map[KEY_L] = 'L';

        upper_map[KEY_Z] = 'Z'; upper_map[KEY_X] = 'X'; upper_map[KEY_C] = 'C';
        upper_map[KEY_V] = 'V'; upper_map[KEY_B] = 'B'; upper_map[KEY_N] = 'N';
        upper_map[KEY_M] = 'M';

        upper_map[KEY_SPACE] = ' ';
        upper_map[KEY_ENTER] = '\n';
        upper_map[KEY_BACKSPACE] = '\b';
    }

    if (keycode < 0 || keycode > KEY_MAX) return 0;  // Ignore invalid keycodes

    // Handle Caps Lock + Shift behavior
    if (caps_lock && shift) {
        if (lower_map[keycode] >= 'a' && lower_map[keycode] <= 'z') {
            return lower_map[keycode]; // Letters stay lowercase
        } else {
            return upper_map[keycode]; // Symbols follow Shift behavior
        }
    }
    // Handle Caps Lock without Shift
    else if (caps_lock) {
        if (lower_map[keycode] >= 'a' && lower_map[keycode] <= 'z') {
            return upper_map[keycode]; // Letters become uppercase
        } else {
            return lower_map[keycode]; // Non-letters stay the same
        }
    }
    // Handle Shift without Caps Lock
    else if (shift) {
        return upper_map[keycode];
    }
    // Default (lowercase)
    return lower_map[keycode];
}

int compare_keystrokes(const void *a, const void *b) {
    const struct keystroke *ka = (const struct keystroke *)a;
    const struct keystroke *kb = (const struct keystroke *)b;
    
    // Compare based on press time
    if (ka->press_time.tv_sec < kb->press_time.tv_sec)
        return -1;
    if (ka->press_time.tv_sec > kb->press_time.tv_sec)
        return 1;

    // If the seconds are equal, compare nanoseconds
    if (ka->press_time.tv_nsec < kb->press_time.tv_nsec)
        return -1;
    if (ka->press_time.tv_nsec > kb->press_time.tv_nsec)
        return 1;

    return 0;
}

/* 
 * Function to serialize and save sessions data to file 
 * Takes in a file handler, an array of sessions, and the number of sessions
 */
int save_sessions(FILE *file, struct session *sessions, size_t session_count) {
    // Make sure file pointer is valid
    if (!file) {
        fprintf(stderr, "Invalid file pointer for saving sessions.\n");
        return -1;
    }

    // Store the number of sessions (8 bytes)
    fwrite(&session_count, sizeof(size_t), 1, file);

    for (size_t i = 0; i < session_count; i++) {
        // Store the number of keystrokes in the session (8 bytes)
        fwrite(&sessions[i].keystrokes_length, sizeof(size_t), 1, file);

        // Store each keystroke (key, press time, release time) (33 bytes total)
        for (size_t j = 0; j < sessions[i].keystrokes_length; j++) {
            fwrite(&sessions[i].keystrokes[j].c, sizeof(char), 1, file);  // Keystroke key (1 byte)
            fwrite(&sessions[i].keystrokes[j].press_time.tv_sec, sizeof(time_t), 1, file);  // Press timestamp (8 bytes)
            fwrite(&sessions[i].keystrokes[j].press_time.tv_nsec, sizeof(long), 1, file);   // Nanoseconds (8 bytes)
            fwrite(&sessions[i].keystrokes[j].release_time.tv_sec, sizeof(time_t), 1, file);  // Release timestamp (8 bytes)
            fwrite(&sessions[i].keystrokes[j].release_time.tv_nsec, sizeof(long), 1, file);   // Nanoseconds (8 bytes)
        }

        // Store time deltas length (8 bytes)
        fwrite(&sessions[i].time_deltas_length, sizeof(size_t), 1, file);
        // Store time deltas data (8 bytes * sessions[i].time_deltas_length)
        if (sessions[i].time_deltas_length > 0) {
            fwrite(sessions[i].time_deltas, sizeof(unsigned long), sessions[i].time_deltas_length, file);
        }

        // Store dwell times length (8 bytes)
        fwrite(&sessions[i].dwell_times_length, sizeof(size_t), 1, file);
        // Store dwell times data (8 bytes * sessions[i].dwell_times_length)
        if (sessions[i].dwell_times_length > 0) {
            fwrite(sessions[i].dwell_times, sizeof(unsigned long), sessions[i].dwell_times_length, file);
        }

        // Store flight times length (8 bytes)
        fwrite(&sessions[i].flight_times_length, sizeof(size_t), 1, file);
        // Store flight times data (8 bytes * sessions[i].flight_times_length)
        if (sessions[i].flight_times_length > 0) {
            fwrite(sessions[i].flight_times, sizeof(unsigned long), sessions[i].flight_times_length, file);
        }
    }

    return 0;
}
