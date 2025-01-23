#include <stdint.h>
#define SPACE ' '
#define byte unsigned char

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

enum cli_sm_state = {   CLI_SM_START, CLI_SM_SHORT_ID, CLI_SM_LONG_ID, CLI_SM_READ_VALUE,
			CLI_SM_ASSIGN_VALUE, CLI_SM_FETCH_NEXT, CLI_SM_BAD_ID, CLI_SM_BAD_VALUE };

enum system_error_code parse_command_line_arguments(int *typing_duration, char *output_file_path, FILE *output_file_path_fh, 
						    char *user, char *email, char *major, short *number_of_tests,
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
	enum cli_sm_state current_state = CLI_SM_START;
	enum kdt_error    error_code = KDT_NO_ERROR;
	while(token_number < argc) {
		switch(current_state) {
			case CLI_SM_START:
				// (1) length too short to be a short or long identifier (min length is 2)
				// (2) parameters always start with at least one hyphen
				if(token_lengths[token_number] <= 1 || current_token[0] != ' ') {
					current_state = CLI_SM_BAD_ID; 				
					break;
				}

				// short ID
				if(token_lengths[token_number] < 3) {	
					current_sate = CLI_SM_SHORT_ID;
					token_index = 1;
					break;
				}

				// long ID
				current_state = CLI_SM_LONG_ID;
				token_index = 2;
				break;	

			case CLI_SM_SHORT_ID:
				switch(current_token[token_index]) {
					case 'u':
						error_code = set_username_from_cli(user, fulfilled_arguments, argv, token_number + 1);
						// Whole state machine should not continue if there is an error with
						// setting this value
						if(error_code != KDT_NO_ERROR) {
							current_state = CLI_SM_BAD_VALUE;
							break;
						}
						
						current_state = CLI_SM_START;
						break;
					case 'e':
						error_code = set_email_from_cli(user
				}
		}
	}
		/*
		// Typing duration argument (required) (-d or --duration)
		if( (strcmp(argv[argv_iterator], "-d") == 0) || (strcmp(argv[argv_iterator], "--duration") == 0) )
		{ 
			// If the current token is -d or --duration, then the next token is surely the time (otherwise, error).
			(*typing_duration) = atoi(argv[argv_iterator + 1]);
			if( (*typing_duration) <= 0) {
				fprintf(stderr, "The value for the -d or --duration flags must be a non-zero positive integer.\n");
				return INVALID_ARGUMENT_VALUE;
			}

			// Update record of fulfilled arguments
			fulfilled_arguments[TYPING_DURATION] = true;

			// Skip next argv item, since it is just the value for the current argument
			argv_iterator++;
		}



		// Output file argument (required) (-o or --output)
		if( (strcmp(argv[argv_iterator], "-o") == 0) || (strcmp(argv[argv_iterator], "--output") == 0) )
		{
			output_file_path = argv[argv_iterator + 1];

			// Check that the specified file can actually be opened
			output_file_path_fh = fopen(output_file_path, "w");
			if(output_file_path_fh == NULL) {
				fprintf(stderr, "Could not open file \"%s\" for writing.\n");
				return INVALID_OUTPUT_FILE;
			}
			fclose(output_file_path_fh);
			
			// Update record of fulfilled arguments
			fulfilled_arguments[OUTPUT_FILE_PATH] = true;

			// Skip next argv item, since it is just the value for the current argument
			argv_iterator++;
		}
		
		// User information
		if( (strcmp(argv[argv_iterator], "-u") == 0) || (strcmp(argv[argv_iterator], "--user") == 0) )
		{
			if(strlen(argv[argv_iterator + 1]) >= 64) {
				fprintf(stderr, "User identifier cannot be longer than 64 characters.\n");
				return INVALID_ARGUMENT_VALUE;
			}
			strcpy(user, argv[argv_iterator + 1]);

			// Update record of fulfilled arguments
			fulfilled_arguments[USER] = true;			

			// Skip next argv item, since it is just the value for the current argument.
			argv_iterator++;
		}

		// Email information
		if( (strcmp(argv[argv_iterator], "-e") == 0) || (strcmp(argv[argv_iterator], "--email") == 0) ) {
			if(strlen(argv[argv_iterator + 1]) >= 64) {
				fprintf(stderr, "Email cannot be longer than 64 characters.\n");
				return INVALID_ARGUMENT_VALUE;
			}		
			strcpy(email, argv[argv_iterator + 1]);

			// Update record of fulfilled arguments
			fulfilled_arguments[EMAIL] = true;

			// Skip next argv item, since it is just the value for the current argument
			argv_iterator++;
		}

		// Major information
		if( (strcmp(argv[argv_iterator], "-m") == 0) || (strcmp(argv[argv_iterator], "--major") == 0) ) {
			if(strlen(argv[argv_iterator + 1]) >= 64) {
				fprintf(stderr, "Major cannot be longer than 64 characters.\n");
				return INVALID_ARGUMENT_VALUE;
			}
			strcpy(major, argv[argv_iterator + 1]);
			
			// Update record of fulfilled arguments
			fulfilled_arguments[MAJOR] = true;
			
			// Skip next item, since it is just the value for the current argument
			argv_iterator++;
		}

		// Number of tests/samples to take
		if( (strcmp(argv[argv_iterator], "-n") == 0) || (strcmp(argv[argv_iterator], "--number") == 0) ) {
			(*number_of_tests) = atoi(argv[argv_iterator + 1]);
			if( (*number_of_tests) <= 0) {
				fprintf(stderr, "The number of tests must be a non-zero positive integer.\n");
				return INVALID_ARGUMENT_VALUE;
			}

			// Update record of fulfilled arguments
			fulfilled_arguments[NUMBER_OF_TESTS] = true;

			// Skip next item, since it is just the value for the current argument
			argv_iterator++;
		}

		// Help text (-h or --help)
		if( (strcmp(argv[argv_iterator], "-h") == 0) || (strcmp(argv[argv_iterator], "--help") == 0) )
		{
			display_help_text();
			return INSUFFICIENT_ARGUMENTS;
		}

		argv_iterator++;
	}

	// Ensure that required arguments were provided
	for(char i = 0; i < REQUIRED_ARGUMENTS_COUNT; i++) {
		if(fulfilled_arguments[i] == true)
			continue;

		return INSUFFICIENT_ARGUMENTS;
	}

	return NO_ERROR;
	*/
}

