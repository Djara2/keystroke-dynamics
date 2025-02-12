# Table of Contents

1. [kdt: Data Collection Tool](#kdt-data-collection-tool)
2. [ak24: Data Analysis Tool](#ak24-data-analysis-tool)

# kdt Data Collection Tool

kdt ("keystrokes dynamics tool") is a command-line keystroke dynamics data collection tool. It is written in GNU C. Its implementation takes advantage of input events stored in Linux device files (files in `/dev/input/`), likewise the tool is compatible with Linux only. **You must specify the appropriate device file for your keyboard when running the program.** 

- Because of this, the tool cannot be run using Windows Subsystem for Linux (WSL). WSL distributions do not include a `/dev/input` directory. This can maybe be resolved by using usbipd-win, but we have not personally tested it. If you are on Windows, consider using Qemu or VirtualBox; this program has been tested using native Linux installations and non-WSL Linux virtual machines. 

⚠️ This tool requires **superuser privileges** to execute, because it reads from files in `/dev/input`. 

Entering `kdt` in the terminal with no parameters or with the help parameter via `kdt -h` or `kdt --help` shows how to use the program:
```
Usage: kdt [OPTIONS]... [FILE] 
Collect keystroke dynamics from a user.This program requires superuser
permissions, because it reads events from device files in /dev/input/

Required arguments:
	-u, --user 	           [TEXT]	the identifier for the person taking the typing examination.

	-e, --email 		   [TEXT]	the username and domain at which the person identified by the -u or --user arguments
						may be reached, after the typing examination.

	-m, --major 	           [TEXT]	the educational major or professional occupation of the person taking the typing examination.

	-d, --duration 		   [INTEGER]	the amount of seconds for which the data collection will occur. 

	-n, --number 		   [INTEGER]	the amount of typing exams the individual will take in the session.

	-o, --output 	           [FILE]	the name of the file to which typing data will be written in CSV format.

	EITHER -f, --free-text     [NONE] 	use free-text data collection, instead of fixed-text.
	OR     -x, --fixed-text    [TEXT]	use fixed-text data collection, instead of free-text.

	-v, --device-file          [FILE]	the device file that corresponds to your machine's keyboard. 
						You can browse these files in /dev/input
	
Examples:
  Using short style:
  	sudo kdt -u dave -e dave@coolmail.com -m "computer science" -d 60 -n 10 -o dave.bin -f -v /dev/input/event10

  Using long style:
  	sudo kdt --user dave --email dave@coolmail.com --major "computer science" --duration 60 --number 10 --output dave.csv \
	  	 --free-text --device-file /dev/input/event10

Runtime Terror/BCDE Group 2024-2025
```

The output of this program is a binary file containing keystroke dynamics data based on what the user typed. You can deserialize this data using the `deserializer` program or the tools in our [ak24 tool.](#ak24-data-analysis-tool)

# ak24 Data Analysis Tool

This is a collection of Python scripts that convert the binary files created by our [kdt program](#kdt-data-collection-tool) into something better suited for analysis.
