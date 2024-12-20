#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#define BACKSPACE 127
#define ENTER 10

// Shared data structure for threads
typedef struct {
	bool flag;
	int seconds;
	pthread_mutex_t lock;
} SharedData;

void non_canonical_mode_with_echoing() { 
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	// Disable canonical mode (enter non-canonical mode)
	t.c_lflag &= ~(ICANON);  
	// Leave echoing
	t.c_lflag |= ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void canonical_mode_with_echoing() { 
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag |= ICANON | ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
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

struct keystroke {
	char c;
	struct timespec timestamp;	
};

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

void keystroke_function(void* arg) {
	SharedData* data = (SharedData*)arg;

}

int main(int argc, char **argv) {
	int seconds = 0;

	// Provide usage instructions (e.g. --help) if no arguments are provided
	if(argc < 2) {
		FILE *help_fh = fopen("help.txt", "r");
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
					return 1;
				}
			}

			c = fgetc(help_fh);
		}

		// Close file and display contents
		fclose(help_fh);
		printf("%s\n", help_fh_contents);
		free(help_fh_contents);
		return 0;
	}
	
	// Parse command line arguments
	
	int sec = atoi(argv[2]);
	printf("The second count is: %d\n", sec);

	// Initialize shared data for timer fucntion
	SharedData shared_data = {
		.flag = true,
		.seconds = atoi(argv[2])
	};
	pthread_mutex_init(&shared_data.lock, NULL);
	
	size_t keystrokes_length = 0;
	size_t keystrokes_capacity = 64;
	struct keystroke keystrokes[keystrokes_capacity];

	unsigned char c;
	unsigned char bytes_read = 0;

	// Prompt
	printf("rawInputTool$ "); 
	fflush(stdout);

	// Enter non-canonical mode without echoing to collect raw data
	disable_buffering_and_echoing();

	// Create thread for timer function
	pthread_t timer_thread, keystroke_thread;
	if(pthread_create(&timer_thread, NULL, (void*)timer_function, &shared_data) != 0) {
		fprintf(stderr, "Error creating timer thread.\n");
		return 1;
	}

	// Actually collect the raw data
	while(data->flag == true) {
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

	// Print the numeric values of keys pressed
	fflush(stdout);
	enable_buffering_and_echoing();
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
