#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>


#define SPACE ' '
#define byte unsigned char
#define SHORT_ID_MATCH_START 1
#define LONG_ID_MATCH_START 2
#define MODE_FREE_TEXT 0
#define MODE_FIXED_TEXT 1
#define REQUIRED_ARGUMENTS_COUNT 8
#define debug_state(current_token,current_parameter_type,sm_state) printf("[DEBUG] The current token is \"%s\". The parameter type is %d. The state is now %d.\n", current_token, current_parameter_type, sm_state)

enum cli_sm_state    {  CLI_SM_START, CLI_SM_READ_PARAM, CLI_SM_READ_VALUE,
			CLI_SM_FAST_MATCH_LONG_NAME,
			CLI_SM_CRASH, CLI_SM_ERROR_PARAM_TOO_SHORT, CLI_SM_ERROR_VALUE_TOO_LONG, CLI_SM_ERROR_INVALID_PARAM, 
			CLI_SM_ERROR_PARAM_MALFORMED, 
			CLI_SM_ERROR_NAN,
			CLI_SM_ERROR_VALUE_RESOURCE_NON_EXISTENT, 
			CLI_SM_ERROR_VALUE_RESOURCE_NON_READABLE,
			CLI_SM_ERROR_VALUE_RESOURCE_NON_WRITABLE
		     };
                                      
enum kdt_parameter   {  KDT_PARAM_NONE,         // 0
			KDT_PARAM_USER,         // 1
			KDT_PARAM_EMAIL,  	// 2
			KDT_PARAM_MAJOR, 	// 3
			KDT_PARAM_DURATION, 	// 4
			KDT_PARAM_REPETITIONS, 	// 5
			KDT_PARAM_OUTPUT_FILE, 	// 6
			KDT_PARAM_DEVICE_FILE, 	// 7
			KDT_PARAM_MODE };      	// 8

enum required_arguments { REQUIRED_ARG_USER,		 // 0
		 	  REQUIRED_ARG_EMAIL,		 // 1
			  REQUIRED_ARG_MAJOR,		 // 2
			  REQUIRED_ARG_TYPING_DURATION,  // 3
			  REQUIRED_ARG_NUMBER_OF_TESTS,  // 4
			  REQUIRED_ARG_OUTPUT_FILE_PATH, // 5
			  REQUIRED_ARG_DEVICE_FILE,      // 6
			  REQUIRED_ARG_MODE              // 7
			};

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


enum kdt_error {KDT_NO_ERROR, KDT_INVALID_PARAMETER, KDT_INVALID_ARGUMENT_VALUE, KDT_INVALID_OUTPUT_FILE,
	        KDT_INSUFFICIENT_ARGUMENTS, KDT_UNHANDLED_ERROR };

enum kdt_error parse_command_line_arguments(int *typing_duration, char *output_file_path, FILE *output_file_fh, 
						    char *user, char *email, char *major, short *number_of_tests, short *duration, byte *mode, char *device_file_path, FILE *device_file_fh,
						    int argc, char **argv) {
	
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

int main(int argc, char **argv) {
	enum kdt_error error = KDT_NO_ERROR;
	int typing_duration = 0;
	char output_file_path[64];
	FILE *output_file_fh;
	char user[64];
	char email[64];
	char major[64];
	short number_of_tests = 0;
	short duration = 0;
	byte mode = MODE_FREE_TEXT;
	char device_file_path[64];
	FILE *device_file_fh;

	error = parse_command_line_arguments(&typing_duration, output_file_path, output_file_fh, user, email, major, &number_of_tests, &duration, &mode, device_file_path, device_file_fh, argc, argv);
	if(error != KDT_NO_ERROR) {
		printf("Something went wrong.\n");
		return 1;
	}

	printf("Everything went well!\n");
	display_environment_details(user, email, major, typing_duration, number_of_tests, output_file_path, device_file_path, mode);
	return 0;
}
