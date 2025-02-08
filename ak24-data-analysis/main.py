from read_binary import read_keystroke_logger_output
from pylibgrapheme import create_grapheme_map, get_combinations, GraphemeType
from masterDictionaryBuilder import create_combined_dictionary, write_to_csv, update_master_dictionary, print_master_dictionary

import numpy as np

def process_sessions(sessions):
   
    for i, session in enumerate(sessions):
        # Create grapheme map for each session
        grapheme_map_error_code, grapheme_map = create_grapheme_map(session, GraphemeType.DIGRAPH)

        # Temperary fix, since these two lists have 1 less value than the rest
        grapheme_map["time_delta"].append(0)
        grapheme_map["flight_time"].append(0)

        # If a grapheme map exists
        if grapheme_map:
            # Create a combined_dictionary for it
            combined_dictionary = create_combined_dictionary(grapheme_map, i+1)

            print(f"\n\n {combined_dictionary}")
            
            # Update master dictionary with combined dictionary
            update_master_dictionary(combined_dictionary, i+1)

        else:
            print("[ERROR] Failed to process session.")

    print_master_dictionary()
    write_to_csv()

        

if __name__ == "__main__":

    # ADD WAY TO GET ARGS FOR WHAT FILE PATHS TO USE

    #file_path = "./output/dataTest.bin"  # Path to your binary file
    file_paths = ["./output/dataTest.bin", "./output/dataTest2.bin"]
    
    for file_path in file_paths:
        # Read sessions from the binary file
        user_info, sessions_data = read_keystroke_logger_output(file_path)

        if sessions_data:
            # Process sessions and flatten the data
            process_sessions(sessions_data)
            
        else:
            print("[ERROR] No valid session data found.")
