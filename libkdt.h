#include <stdbool.h>
#include <stdint.h>
#ifndef LIBKDT_H
#define LIBKDT_H

#define SPACE ' '
#define byte unsigned char
#define SHORT_ID_MATCH_START 1
#define LONG_ID_MATCH_START 2
#define MODE_FREE_TEXT 0
#define MODE_FIXED_TEXT 1
#define REQUIRED_ARGUMENTS_COUNT 8
#define debug_state(current_token,current_parameter_type,sm_state) printf("[DEBUG] The current token is \"%s\". The parameter type is %d. The state is now %d.\n", current_token, current_parameter_type, sm_state)

#define BUFFER_SIZE 256
#define ENTER 10
#define BACKSPACE 127

#define MODULUS 211
#define HASH_SCALAR 37

enum kdt_error       {  KDT_NO_ERROR,
			KDT_INVALID_PARAMETER,
			KDT_INVALID_ARGUMENT_VALUE,
			KDT_INVALID_OUTPUT_FILE,
			KDT_INSUFFICIENT_ARGUMENTS,
			KDT_UNHANDLED_ERROR
		     };

enum cli_sm_state    {  CLI_SM_START,
			CLI_SM_READ_PARAM,
			CLI_SM_READ_VALUE,
			CLI_SM_FAST_MATCH_LONG_NAME,
			CLI_SM_CRASH,
			CLI_SM_ERROR_PARAM_TOO_SHORT,
			CLI_SM_ERROR_VALUE_TOO_LONG, 
			CLI_SM_ERROR_INVALID_PARAM, 
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
			KDT_PARAM_MODE		// 8
		     };      	

enum required_arguments { REQUIRED_ARG_USER,		 // 0
		 	  REQUIRED_ARG_EMAIL,		 // 1
			  REQUIRED_ARG_MAJOR,		 // 2
			  REQUIRED_ARG_TYPING_DURATION,  // 3
			  REQUIRED_ARG_NUMBER_OF_TESTS,  // 4
			  REQUIRED_ARG_OUTPUT_FILE_PATH, // 5
			  REQUIRED_ARG_DEVICE_FILE,      // 6
			  REQUIRED_ARG_MODE              // 7
			};

// Principal data collection object
struct keystroke {
	char c;
	struct timespec press_time;
	struct timespec release_time;
};

// Shared data structure for threads
struct timer_state {
	bool flag;
	int seconds;
	// pthread_mutex_t lock;
};

struct phoneme {
	char length;
	char *str;
};

struct time_delta_array {
	unsigned long *values;
	size_t length;
	size_t capacity;
};

struct kv_pair {
	struct phoneme *key;
	struct time_delta_array *value;
	struct kv_pair *next;
};

struct session {
	struct keystroke *keystrokes;
	size_t keystrokes_length;
	struct time_delta_array *time_deltas;
};

struct phoneme* phoneme_create(char *str, byte length);
bool phoneme_compare(struct phoneme *p1, struct phoneme *p2);

struct time_delta_array* time_delta_array_create(unsigned long *values, size_t length, size_t capacity);

struct kv_pair *kv_pair_create(struct phoneme *key, struct time_delta_array *value);

short hash_phoneme_struct(struct phoneme *p);

bool hashmap_set(struct kv_pair **hashmap, struct phoneme *key, struct time_delta_array *value);
struct time_delta_array* hashmap_get(struct kv_pair **hashmap, struct phoneme *key);

void disable_buffering_and_echoing();
void enable_buffering_and_echoing();

void timer_function(void* arg);

unsigned long* get_dwell_times_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length);
unsigned long* get_flight_times_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length);

void display_help_text();
void display_environment_details(char user[], char email[], char major[], int duration, short number_of_samples, char output_file_path[], char device_file_path[], byte mode);

enum kdt_error parse_command_line_arguments(char *user, char *email, char *major, byte *mode, short *number_of_tests, short *typing_duration, char *device_file_path, char *output_file_path, FILE *output_file_fh, int argc, char **argv); 

int keycode_to_ascii(int keycode, int shift, int caps_lock);
int compare_keystrokes(const void *a, const void *b);

#endif
