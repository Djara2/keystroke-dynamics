#include <stdint.h>
#define SPACE ' '
#define byte unsigned char
#define SHORT_ID_MATCH_START 1
#define LONG_ID_MATCH_START 2
#define MODE_FREE_TEXT 0
#define MODE_FIXED_TEXT 1

enum cli_sm_state    {  CLI_SM_START, CLI_SM_READ_PARAM, CLI_SM_READ_VALUE,
			CLI_SM_FAST_MATCH_LONG_NAME,
			CLI_SM_CRASH, CLI_SM_ERROR_PARAM_TOO_SHORT, CLI_SM_ERROR_INVALID_PARAM, 
			CLI_SM_ERROR_PARAM_MALFORMED, 
			CLI_SM_ERROR_NAN };

enum kdt_parameter   {  KDT_PARAM_NONE, KDT_PARAM_USER, KDT_PARAM_EMAIL, KDT_PARAM_MAJOR, KDT_PARAM_DURATION, 
			KDT_PARAM_REPETITIONS, KDT_PARAM_OUTPUT_FILE, KDT_PARAM_MODE };

enum required_arguments { REQUIRED_ARG_USER,		// 0
		 	  REQUIRED_ARG_EMAIL,		// 1
			  REQUIRED_ARG_MAJOR,		// 2
			  REQUIRED_ARG_TYPING_DURATION, // 3
			  REQUIRED_ARG_NUMBER_OF_TESTS, // 4
			  REQUIRED_ARG_OUTPUT_FILE_PATH // 5
			};

enum kdt_error {KDT_NO_ERROR, KDT_INVALID_ARGUMENT, KDT_INVALID_ARGUMENT_VALUE, KDT_INVALID_OUTPUT_FILE,
	        KDT_INSUFFICIENT_ARGUMENTS, KDT_UNHANDLED_ERROR };

enum kdt_error parse_command_line_arguments(int *typing_duration, char *output_file_path, FILE *output_file_fh, 
						    char *user, char *email, char *major, short *number_of_tests, short *duration,
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
	enum cli_sm_state  current_state = CLI_READ_PARAM;
	enum kdt_error     error_code = KDT_NO_ERROR;
	void               *current_parameter;
	uint8_t            match_start_index = 1;
	enum kdt_parameter current_parameter_type = KDT_PARAM_NONE;
	while(token_number < argc) {
		current_token = argv[token_number];
		switch(current_state) {
			case CLI_SM_READ_PARAM:
				// Parameter identifiers must be at least 2 chars long.  
				if(token_lengths[current_token] < 2) {
					current_state = CLI_SM_ERROR_PARAM_TOO_SHORT;
					break;
				}
				
				// Parameter identifiers must start with one hyphen or two at most
				if(current_token[0] != '-') {
					current_state = CLI_SM_ERROR_PARAM_MALFORMED;
					break;
				}

				// allows for shared logic of short ID matching
				// and long ID autocompletion
				if(current_token[1] == '-')
					match_start_index = LONG_ID_MATCH_START;
				else
					match_start_index = SHORT_ID_MATCH_START;

				// Discern short identifier or long identifier
				switch(current_token[match_start_index]) { 
					// Username
					case 'u':
						current_parameter = (void*) user;
						current_state = CLI_SM_READ_VALUE;
						token_number++;
						break;

					// Email
					case 'e':
						current_parameter = (void*) email;	
						current_state = CLI_SM_READ_VALUE;
						token_number++;
						break;

					// Major
					case 'm':
						current_parameter = (void*) major;
						current_state = CLI_SM_READ_VALUE;
						token_number++;
						break;

					// Number of tests
					case 'n':
						current_parameter = (void*) number_of_tests;
						current_state = CLI_SM_READ_VALUE;
						token_number++;
						break;

					// Duration
					case 'd':
						current_parameter = (void*) duration;	
						current_state = CLI_SM_READ_VALUE;
						token_number++;
						break;

					// Output
					case 'o':
						current_parameter = (void*) output;
						current_state = CLI_SM_READ_VALUE;
						token_number++;
						break;

					// Free text
					case 'f':
						// this one has ambiguity between short and long identifiers.
						// --f does not necessarily map to --free, it could also map 
						//     to --fixed
						if(match_start_index == SHORT_ID_MATCH_START) {
							current_parameter = (void*) mode;
							(*current_parameter) = MODE_FREE_TEXT;

							// since this parameter takes no value, we skip
							// to the next token and the mode remains in 
							// CLI_SM_READ_PARAM
							current_state = CLI_SM_READ_PARAM;
							token_number++;
							break;
						}
						// to be either "--free" or "--fixed", the token will have to be 
						// at least 6 characters long.
						if(token_lengths[token_number] < 6) {
							current_state = CLI_SM_ERROR_INVALID_PARAM;
							break;
						}

						// long identifier autocomplete
						switch(current_token[3]) {
							case 'r':
								current_parameter = (void*) mode;
								(*current_parameter) = MODE_FREE_TEXT;
								current_state = CLI_READ_PARAM;
								token_number++;
								break;
							case 'i':
								current_parameter = (void*) mode;
								(*current_parameter) = MODE_FIXED_TEXT;
								current_state = CLI_SM_READ_PARAM;
								token_number++;
								break;

							// User misspelled ID completely
							default:
								current_state = CLI_SM_ERROR_INVALID_PARAM;
								break;
						}
						break;

					// Fixed text
					case 'x':
						current_parameter = (void*) mode;
						(*current_parameter) = MODE_FIXED_TEXT;
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
						if(output_file_path == NULL) {
							
						}
						break;

					case KDT_PARAM_MODE:
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

			default:
				fprintf(stderr, "Unhandled case for state %d reached.\n", current_state);
				return KDT_UNHANDLED_ERROR;
				break;
		} // end switch(current state)
	} // end while

	// Internal logic should return a kdt_error value other than KDT_NO_ERROR when 
	// something goes wrong. If this is reached, then no errors occurred.
	return KDT_NO_ERROR;
}
