from read_binary import read_keystroke_logger_output
from pylibgrapheme import create_grapheme_map, GraphemeType
from flatten import flatten_grapheme_map

import numpy as np

def process_sessions(sessions):
   
    for session in sessions:
        # Create grapheme map for each session
        grapheme_map_error_code, grapheme_map = create_grapheme_map(session, GraphemeType.DIGRAPH)

        if grapheme_map:
            # Flatten the grapheme map into feature vectors
            flattened_data = flatten_grapheme_map(grapheme_map)
            if flattened_data is not None:
                
                print(f"Flattened Features for All Sessions:\n{flattened_data}")

            else:
                print("[ERROR] Flattening failed for a session.")
        else:
            print("[ERROR] Failed to process session.")
    

if __name__ == "__main__":
    file_path = "./output/infoTest.bin"  # Path to your binary file
    
    # Read sessions from the binary file
    user_info, sessions_data = read_keystroke_logger_output(file_path)

    if sessions_data:
        # Process sessions and flatten the data
        process_sessions(sessions_data)
        
    else:
        print("[ERROR] No valid session data found.")
