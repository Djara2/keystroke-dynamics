#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>

#define BACKSPACE 127

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

int main(void) {
	/*
	unsigned short input_buffer_capacity = 512;
	unsigned short input_buffer_length = 0;
	unsigned char input_buffer[input_buffer_capacity];
	*/
	size_t keystrokes_length = 0;
	size_t keystrokes_capacity = 64;
	struct keystroke keystrokes[keystrokes_capacity];

	unsigned char c;
	unsigned char bytes_read = 0;

	// Prompt
	printf("rawInputTool$ "); 
	fflush(stdout);

	// Begin collecting raw data, no echoing, non-canonical mode
	disable_buffering_and_echoing();
	while(keystrokes_length < keystrokes_capacity) {
		bytes_read = read(STDIN_FILENO, &c, 1);
		if(bytes_read <= 0) continue;
		
		// Capture character stroke and timestamp
		keystrokes[keystrokes_length].c = c;
		clock_gettime( CLOCK_MONOTONIC, &(keystrokes[keystrokes_length].timestamp) );
		keystrokes_length++;

		// echo back what was written
		switch(c) {
			case BACKSPACE:
				printf("\b \b");
				fflush(stdout);
				break;
			case '\n':
				break;
			default:
				printf("%c", c);
				fflush(stdout);
				break;
		}
	}

	// Print the numeric values of keys pressed	
	enable_buffering_and_echoing();
	printf("Numeric codes entered:\n");
	printf("\n%c", keystrokes[0].c);
	for(size_t i = 1; i < keystrokes_length ; i++) 
		printf(", %d", (int) keystrokes[i].c);

	printf("\n");
	unsigned long *time_deltas = get_time_deltas_in_milliseconds(keystrokes, keystrokes_length);
	if(time_deltas == NULL) {
		fprintf(stderr, "Could not get time deltas.\n"); 
		return 1;
	}

	printf("Time deltas:\n");
	printf("%ld", time_deltas[0]);
	for(size_t i = 1; i < keystrokes_length - 1; i++) 
		printf(", %ld", time_deltas[i]);

	printf("\n");
	return 0;
}
