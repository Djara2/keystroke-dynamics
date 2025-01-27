#include <stdint.h>
#define SPACE ' '
#define byte unsigned char
#define SHORT_ID_MATCH_START 1
#define LONG_ID_MATCH_START 2
#define MODE_FREE_TEXT 0
#define MODE_FIXED_TEXT 1

enum kdt_error set_cli_parameter(enum kdt_parameter parameter_id, void *parameter, char *value, bool *fulfilled_arguments) {
	enum kdt_error error_code = KDT_NO_ERROR;

	// more clean than using function pointers, faster switching than if-else
	switch(parameter_id) {
		case USER: 
			char *user = (char *) parameter;
			if(user == NULL) {
				fprintf(stderr, "[set_username_from_cli] Cannot use NULL username pointer when setting username from CLI args.\n");
				return INVALID_ARGUMENT_VALUE;
			}

			if(fulfilled_arguments == NULL) {
				fprintf(stderr, "[set_username_from_cli] Cannot use NULL pointer for fulfilled arguments buffer.\n");
				return INVALID_ARGUMENT_VALUE;
			}

			if(strlen(argv[value_position]) >= 64) {
				fprintf(stderr, "[set_username_from_cli] Value for username parameter cannot be longer than 64 characters.\n");
				return INVALID_ARGUMENT_VALUE;
			}
			
			fulfilled_arguments[USER] = true;

			return NO_ERROR;

			break;
		case EMAIL:
			break;
		case MAJOR:
			break;
		case DURATION:
			break;
		case NUMBER:
			break;
		case OUTPUT:
			break;
		case MODE:
			break;
		// invalid parameter ID
		default:
			break;

	}
	return error_code;
}

enum cli_sm_state = {   CLI_SM_START, CLI_SM_READ_PARAM, CLI_SM_READ_VALUE,
			CLI_SM_FAST_MATCH_LONG_NAME,
			CLI_SM_CRASH, CLI_SM_ERROR_PARAM_TOO_SHORT, CLI_SM_ERROR_INVALID_PARAM, 
			CLI_SM_ERROR_PARAM_MALFORMED };

enum system_error_code parse_command_line_arguments(int *typing_duration, char *output_file_path, FILE *output_file_path_fh, 
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
	
	uint8_t           token_number = 1;
	uint16_t          token_index = 0;
	char              *current_token = argv[token_number];
	enum cli_sm_state current_state = CLI_READ_PARAM;
	enum kdt_error    error_code = KDT_NO_ERROR;
	void              *current_parameter;
	uint8_t           match_start_index = 1;
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
						token_number++;
						break;

					// Email
					case 'e':
						current_parameter = (void*) email;
						token_number++;
						break;

					// Major
					case 'm':
						current_parameter = (void*) major;
						token_number++;
						break;

					// Number of tests
					case 'n':
						current_parameter = (void*) number_of_tests;
						token_number++;
						break;

					// Duration
					case 'd':
						current_parameter = (void*) duration;	
						token_number++;
						break;

					// Output
					case 'o':
						current_parameter = (void*) output;
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
				break;
			case CLI_SM_ERROR_PARAM_TOO_SHORT:
				fprintf(stderr, "Provided parameter \"%s\" is too short. Parameters must be at least 2 characters long.\n", current_token);
				return KDT
				break;
			case CLI_SM_ERROR_PARAM_MALFORMED:
				break;
			case CLI_SM_ERROR_INVALID_PARAM:
				break;
			default:
				fprintf(stderr, "Unhandled case for state %d reached.\n", current_state);
				break;
		}
	}

