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

unsigned long* get_dwell_times_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length) {
	if(keystrokes == NULL) return NULL;
		
	unsigned long *time_deltas = malloc(sizeof(unsigned long) * (keystrokes_length));
	if(time_deltas == NULL) {
		printf("Error allocating memory for time deltas buffer.\n");
		return NULL;
	}

	for(size_t i = 0; i < keystrokes_length; i++) {
		unsigned long whole_seconds_difference_in_ms = 1000 * (keystrokes[i].release_time.tv_sec - keystrokes[i].press_time.tv_sec);
		unsigned long nanoseconds_difference_in_ms   = (keystrokes[i].release_time.tv_nsec - keystrokes[i].press_time.tv_nsec) / 1000000;
		time_deltas[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;	
	}

	return time_deltas;
}

unsigned long* get_flight_times_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length) {
	if(keystrokes == NULL) return NULL;
		
	unsigned long *time_deltas = malloc(sizeof(unsigned long) * (keystrokes_length - 1));
	if(time_deltas == NULL) {
		printf("Error allocating memory for time deltas buffer.\n");
		return NULL;
	}

    if(keystrokes_length >= 2) {
        for(size_t i = 0; i < keystrokes_length - 1; i++) {
            if(keystrokes[i + 1].press_time.tv_sec > keystrokes[i].release_time.tv_sec ||
        keystrokes[i + 1].press_time.tv_nsec > keystrokes[i].release_time.tv_nsec) {
                unsigned long whole_seconds_difference_in_ms = 1000 * (keystrokes[i + 1].press_time.tv_sec - keystrokes[i].release_time.tv_sec);
                unsigned long nanoseconds_difference_in_ms   = (keystrokes[i + 1].press_time.tv_nsec - keystrokes[i].release_time.tv_nsec) / 1000000;
                time_deltas[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;
		time_deltas[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;	
                time_deltas[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;
            }
            else {
                unsigned long whole_seconds_difference_in_ms = 1000 * (keystrokes[i].release_time.tv_sec - keystrokes[i + 1].press_time.tv_sec);
                unsigned long nanoseconds_difference_in_ms   = (keystrokes[i].release_time.tv_nsec - keystrokes[i + 1].press_time.tv_nsec) / 1000000;
                time_deltas[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;
            }
	    }
    }
	

	return time_deltas;
}

void display_help_text() {
	FILE *help_fh = fopen("res/help.txt", "r");
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

enum kdt_error parse_command_line_arguments(char *user, char *email, char *major, byte *mode, short *number_of_tests, short *typing_duration, char *device_file_path, FILE *device_file_fh, char *output_file_path, FILE *output_file_fh, int argc, char **argv) {	
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
					// Username
					case 'u':
						current_parameter_type = KDT_PARAM_USER;
						current_state = CLI_SM_READ_VALUE;

						debug_state(current_token, current_parameter_type, current_state);

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

struct phoneme* phoneme_create(char *str, byte length) {
	if(str == NULL) {
		fprintf(stderr, "[phoneme_create] Cannot create a phoneme using a string that points to NULL.\n");
		return NULL;
	}
	if(length <= 1) {
		fprintf(stderr, "[phoneme_create] Cannot create a phoneme of length 0 or 1.\n");
		return NULL;
	}

	struct phoneme *new_phoneme = malloc(sizeof(struct phoneme));
	if(new_phoneme == NULL) {
		fprintf(stderr, "[phoneme_create] Failed to allocate sufficient memory to create a new phoneme struct.\n");
		return NULL;
	}

	// Set string member
	new_phoneme->str = malloc( (sizeof(char) * length) + 1 );
	if(new_phoneme->str == NULL) {
		fprintf(stderr, "[phoneme_create] Failed to allocate sufficient memory to create the memory buffer for the new phoneme's string attribute.\n");
		return NULL;
	}
	strcpy(new_phoneme->str, str);

	// Set length member
	new_phoneme->length = length;

	return new_phoneme;
}

bool phoneme_compare(struct phoneme *p1, struct phoneme *p2) {
	if(p1 == NULL) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme that points to NULL");
		return false;
	} 
	if(p1->str == NULL) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme with a string member that points to NULL.\n");
		return false;
	}
	if(p1->length <= 1) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme with an invalid length member. Phoneme lengths must be greater than 1.\n");
		return false;
	}
	if(p2 == NULL) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme that points to NULL.\n");
		return false;
	}
	if(p2->str == NULL) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme with a string member that points to NULL.\n");
		return false;
	}
	if(p2->length <= 1) {
		fprintf(stderr, "[phoneme_compare] Cannot compare a phoneme with an invalid length member. Phoneme lengths must be greater than 1.\n");
		return false;
	}
	
	if(p1->length != p2->length)      return false;
	if(strcmp(p1->str, p2->str) != 0) return false;

	return true;
}

struct time_delta_array* time_delta_array_create(unsigned long *values, size_t length, size_t capacity) {
	struct time_delta_array *new_time_delta_array = malloc(sizeof(struct time_delta_array));
	if(new_time_delta_array == NULL) {
		fprintf(stderr, "[time_delta_array_create] Failed to allocate memory for a new time delta array.\n");
		return NULL;
	}

	new_time_delta_array->values = values;
	new_time_delta_array->length = length;
	new_time_delta_array->capacity = capacity;

	return new_time_delta_array;
}

// key is required, but time deltas is not (it can be NULL). Just check for this later
struct kv_pair *kv_pair_create(struct phoneme *key, struct time_delta_array *value) {
	if(key == NULL) {
		fprintf(stderr, "[kv_pair_create] Cannot create a new key-value pair with a phoneme key that points to NULL.\n");
		return NULL;
	}

	// Create new pair
	struct kv_pair *new_kv_pair = malloc(sizeof(struct kv_pair));
	if(new_kv_pair == NULL) {
		fprintf(stderr, "[kv_pair_create] Failed to allocate memory for a new kv_pair struct.\n");
		return NULL;
	}

	// Assign key and value
	new_kv_pair->key = key;	
	new_kv_pair->value = value;
	new_kv_pair->next = NULL;
}

short hash_phoneme_struct(struct phoneme *p) {
	if(p == NULL) {
		fprintf(stderr, "[hash_phoneme_struct] Cannot hash phoneme struct that points to NULL.\n");
		return -1;
	}
	
	unsigned long hash = 0;
	for(int i = 0; i < p->length; i++) 
		hash += p->str[i];

	hash *= HASH_SCALAR;
	hash = hash % MODULUS;
	return (unsigned short) hash;
}

bool hashmap_set(struct kv_pair **hashmap, struct phoneme *key, struct time_delta_array *value) {
	if(hashmap == NULL) {
		fprintf(stderr, "[hashmap_insert] Insertion into a hashmap that points to NULL cannot be done.\n");
		return false;
	}

	if(key == NULL) {
		fprintf(stderr, "[hashmap_insert] Insertion of a phoneme key that points to NULL cannot be done.\n");
		return false;
	}

	short hashed_index = hash_phoneme_struct(key);
	if(hashed_index == -1) { 
		fprintf(stderr, "[hashmap_insert] Hashed index came back as -1 for provided phoneme key.\n");
		return false;
	}
	
	// Case 1: Index is NULL, meaning it is has never been used before and is free for insertion
	if(hashmap[hashed_index] == NULL) {
		hashmap[hashed_index] = kv_pair_create(key, value);
		if(hashmap[hashed_index] == NULL) {
			fprintf(stderr, "[hashmap_insert] Index %hd assigned to NULL after call to kv_pair_create.\n");
			return false;
		}
		return true;
	}
	else {
	// Case 2: Index is occupied. The key may or may not already exist
		struct kv_pair *current = hashmap[hashed_index];
		while(current->next != NULL)
		{
			// Case 2.1: Key already exists, just as a node in a linear chain. Update value and then return true.
			if(phoneme_compare(hashmap[hashed_index]->key, key) == 0) {
				// Prevent memory leaks
				if(hashmap[hashed_index]->value != NULL) {
					free(hashmap[hashed_index]->value->values);
					free(hashmap[hashed_index]->value);
				}
				// Update value mapped by pre-existing key and return
				hashmap[hashed_index]->value = value;
				return true;
			}
		
			current = current->next;
		}
		// Case 2.2: Key does NOT exist. Add it via linear probing.
		current->next = kv_pair_create(key, value);
		if(current->next == NULL) {
			fprintf(stderr, "[hashmap_set] Failed to insert new hashmap key. Call to kv_pair_create returned NULL.\n");
			return false;
		}
		return true;
	}
}

struct time_delta_array* hashmap_get(struct kv_pair **hashmap, struct phoneme *key) {
	if(hashmap == NULL) {
		fprintf(stderr, "[hashmap_get] Cannot retrieve values from hashmap that points to NULL.\n");
		return NULL;
	}
	if(key == NULL) {
		fprintf(stderr, "[hashmap_get] Cannot search hashmap for a key that points to NULL.\n");
		return NULL;
	}

	short hashed_index = hash_phoneme_struct(key);
	if(hashed_index == -1) {
		fprintf(stderr, "[hashmap_get] Failed to hash provided key. Function hash_phoneme_struct returned -1.\n");
		return NULL;
	}
	
	// Case 1: Immediate miss
	if(hashmap[hashed_index] == NULL) {
		fprintf(stderr, "[hashmap_get] Cannot get the value mapped to a key that does not exist in the hashmap.\n");
		return NULL;
	}
	else{
		// Do linear probing
		struct kv_pair *current = hashmap[hashed_index];
		while(current->next != NULL) {
			// Case 2.1: Key found, return value
			if(phoneme_compare(current->key, key) == 0)
				return current->value;
			
			current = current->next;
		}

		// Case 2.2: Key does not exist in the hashmap, return NULL
		return NULL;
	}
}

// Convert keycode to ASCII considering Shift
int keycode_to_ascii(int keycode, int shift) {
    static char lower_map[KEY_MAX + 1] = {0};
    static char upper_map[KEY_MAX + 1] = {0};

    // Populate lower_map (normal keys)
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

    if (keycode < 0 || keycode > KEY_MAX) return 0;  // Ignore invalid keycodes
    return shift ? upper_map[keycode] : lower_map[keycode];
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
