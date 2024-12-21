#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#define byte unsigned char
#define BACKSPACE 127
#define ENTER 10
#define REQUIRED_ARGUMENTS_COUNT 6
#define MODULUS 211
#define HASH_SCALAR 37 

// Principal data collection object
struct keystroke {
	char c;
	struct timespec timestamp;	
};

// Shared data structure for threads
typedef struct {
	bool flag;
	int seconds;
	pthread_mutex_t lock;
} SharedData;

struct phoneme {
	char length;
	char *str;
};

struct kv_pair {
	struct phoneme *key;
	unsigned long  *value;
	struct kv_pair *next;
};

struct phoneme* phoneme_create(char *str, byte length);
bool phoneme_compare(struct phoneme *p1, struct phoneme *p2);

struct kv_pair *kv_pair_create(struct phoneme *key, unsigned long *value);

unsigned short hash_phoneme_struct(struct phoneme p);	

//                       0     1      2      3                4                5
enum required_arguments {USER, EMAIL, MAJOR, TYPING_DURATION, NUMBER_OF_TESTS, OUTPUT_FILE_PATH};
enum system_error_code  {NO_ERROR, INVALID_ARGUMENT_VALUE, INVALID_OUTPUT_FILE, INSUFFICIENT_ARGUMENTS};

void disable_buffering_and_echoing();
void enable_buffering_and_echoing();

void timer_function(void* arg);

unsigned long* get_time_deltas_in_milliseconds(struct keystroke keystrokes[], size_t keystrokes_length);

void display_help_text();
void display_environment_details(char user[], char email[], char major[], int duration, short number_of_samples);

enum system_error_code parse_command_line_arguments(int *typing_duration, char *output_file_path, FILE *output_file_path_fh, 
						    char *user, char *email, char *major, short *number_of_tests,
						    int argc, char **argv);

int main(int argc, char **argv) {
	// Provide usage instructions (e.g. --help) if no arguments are provided
	if(argc < 2) {
		display_help_text();
		exit(EXIT_FAILURE);
	}
	
	// Parse command line arguments
	int typing_duration = 0;
	char *output_file_path = NULL;
	FILE *output_file_path_fh = NULL;
	char user[64];
	char email[64];
	char major[64];
	short number_of_tests = 0;
	enum system_error_code error_code = NO_ERROR;
	error_code = parse_command_line_arguments(&typing_duration, output_file_path, output_file_path_fh, user, email, major, &number_of_tests, argc, argv);
	switch(error_code) {
		case NO_ERROR:
			break;
		case INVALID_ARGUMENT_VALUE:
			exit(EXIT_FAILURE);
			break;
		case INVALID_OUTPUT_FILE:
			exit(EXIT_FAILURE);
			break;
		case INSUFFICIENT_ARGUMENTS:
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
	display_environment_details(user, email, major, typing_duration, number_of_tests);

	// Initialize shared data for timer fucntion
	SharedData shared_data = {
		.flag = true,
		.seconds = atoi(argv[2])
	};
	pthread_mutex_init(&shared_data.lock, NULL);
	
	size_t keystrokes_length = 0;
	size_t keystrokes_capacity = 2048;
	struct keystroke keystrokes[keystrokes_capacity];

	unsigned char c;
	unsigned char bytes_read = 0;

	// Prompt
	printf("rawInputTool$ "); 
	fflush(stdout);

	// Enter non-canonical mode without echoing to collect raw data
	disable_buffering_and_echoing();

	// Create thread for timer function
	pthread_t timer_thread;
	if(pthread_create(&timer_thread, NULL, (void*)timer_function, &shared_data) != 0) {
		fprintf(stderr, "Error creating timer thread.\n");
		return 1;
	}

	// Actually collect the raw data
	while(shared_data.flag == true) {
		bytes_read = read(STDIN_FILENO, &c, 1);
		if(bytes_read <= 0) continue;
		
		// Capture character stroke and timestamp
		if(keystrokes_length < keystrokes_capacity) {
			keystrokes[keystrokes_length].c = c;
			clock_gettime( CLOCK_MONOTONIC, &(keystrokes[keystrokes_length].timestamp) );
			keystrokes_length++;
		}

		// echo back what was written
		switch(c) {
			case BACKSPACE:
				printf("\b \b");
				fflush(stdout);
				break;

			default:
				printf("%c", c);
				fflush(stdout);
				break;
		}

	}
	
	// Restore canonical mode and echoing
	fflush(stdout);
	enable_buffering_and_echoing();

	// Print the numeric values of keys pressed
	printf("\nNumeric codes entered:\n");
	printf("\n%d", (int) keystrokes[0].c);
	for(size_t i = 1; i < keystrokes_length ; i++) 
		printf(", %d", (int) keystrokes[i].c);

	printf("\n");
	unsigned long *time_deltas = get_time_deltas_in_milliseconds(keystrokes, keystrokes_length);
	if(time_deltas == NULL) {
		fprintf(stderr, "Could not get time deltas.\n"); 
	}

	printf("\nTime deltas:\n");
	printf("%ld", time_deltas[0]);
	for(size_t i = 1; i < keystrokes_length - 1; i++) 
		printf(", %ld", time_deltas[i]);

	printf("\n");



	pthread_join(timer_thread, NULL);

	// Clean up thread mutex
	pthread_mutex_destroy(&shared_data.lock);

	printf("Program terminated.\n");
	return 0;
}

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
	SharedData* data = (SharedData*)arg;

	sleep(data->seconds);

	pthread_mutex_lock(&data->lock);
	data->flag = false;
	printf("\nTimer has elapsed! Flag changed to: %s\n", data->flag ? "true" : "false");
	pthread_mutex_unlock(&data->lock);
}

unsigned long* get_time_deltas_in_milliseconds(struct keystroke keystrokes[], size_t keystrokes_length) {
	if(keystrokes == NULL) return NULL;
		
	unsigned long *time_deltas = malloc(sizeof(unsigned long) * (keystrokes_length - 1));
	if(time_deltas == NULL) {
		printf("Error allocating memory for time deltas buffer.\n");
		return NULL;
	}

	for(size_t i = 0; i < keystrokes_length - 1; i++) {
		unsigned long whole_seconds_difference_in_ms = 1000 * (keystrokes[i + 1].timestamp.tv_sec - keystrokes[i].timestamp.tv_sec);
		unsigned long nanoseconds_difference_in_ms   = (keystrokes[i + 1].timestamp.tv_nsec - keystrokes[i].timestamp.tv_nsec) / 1000000;
		time_deltas[i] = whole_seconds_difference_in_ms + nanoseconds_difference_in_ms;	
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

void display_environment_details(char user[], char email[], char major[], int duration, short number_of_samples) {
	printf("Environment:\n\tUser: %s\n\tEmail: %s\n\tMajor: %s\n\tTyping duration: %d\n\tSamples to take: %hd\n",
			user, email, major, duration, number_of_samples);
}

enum system_error_code parse_command_line_arguments(int *typing_duration, char *output_file_path, FILE *output_file_path_fh, 
						    char *user, char *email, char *major, short *number_of_tests,
						    int argc, char **argv) {
	bool fulfilled_arguments[REQUIRED_ARGUMENTS_COUNT];
	for(char i = 0; i < REQUIRED_ARGUMENTS_COUNT; i++) 
		fulfilled_arguments[i] = false;

	char argv_iterator = 1;
	while(argv_iterator < argc) {
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

// key is required, but time deltas is not (it can be NULL). Just check for this later
struct kv_pair *kv_pair_create(struct phoneme *key, unsigned long *value) {
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
	new_kv_pair->value = value; // might be NULL if creating key for first time.
	new_kv_pair->next = NULL 
}

unsigned short hash_phoneme_struct(struct phoneme *p) {
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
