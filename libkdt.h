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
			KDT_UNHANDLED_ERROR,
			KDT_HELP_REQUEST,
			KDT_MALLOC_FAILURE,
			KDT_INADEQUATE_DATA,
			KDT_NULL_ERROR
		     };


enum kdt_statistic   {  STATISTIC_TIME_DELTAS,
			STATISTIC_DWELL_TIMES,
			STATISTIC_FLIGHT_TIMES
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
			CLI_SM_ERROR_VALUE_RESOURCE_NON_WRITABLE,

			CLI_SM_DISPLAY_HELP_TEXT
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

struct session {
	struct user_info *user_info;

	struct keystroke *keystrokes;
	size_t keystrokes_length;

	unsigned long *time_deltas;
	size_t time_deltas_length;

	unsigned long *dwell_times;
	size_t dwell_times_length;

	unsigned long *flight_times;
	size_t flight_times_length;
};

struct user_info {
	char user[64];
	char email[64];
	char major[64];
	short typing_duration;
};

void timer_function(void* arg);

// Enable/disable raw terminal mode
void disable_buffering_and_echoing();
void enable_buffering_and_echoing();

// Time array stuff
enum kdt_error set_session_statistic_data( struct session *s, enum kdt_statistic statistic_code);

unsigned long* get_time_deltas_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length);
unsigned long* get_dwell_times_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length);
unsigned long* get_flight_times_in_milliseconds(struct keystroke *keystrokes, size_t keystrokes_length);

// Debugging stuff
void display_help_text();
void display_environment_details(char user[], char email[], char major[], int duration, short number_of_samples, char output_file_path[], char device_file_path[], byte mode);

// CLI paraser
enum kdt_error parse_command_line_arguments(char *user, char *email, char *major, byte *mode, short *number_of_tests, short *typing_duration, char *device_file_path, char *output_file_path, FILE *output_file_fh, int argc, char **argv); 

// Interpreting event file 
int keycode_to_ascii(int keycode, int shift, int caps_lock);
int compare_keystrokes(const void *a, const void *b);

int save_sessions(FILE *file, struct user_info *user_info, struct session *sessions, size_t session_count);

#endif
