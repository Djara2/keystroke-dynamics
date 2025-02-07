import numpy as np

# Function to flatten the grapheme map
def flatten_grapheme_map(grapheme_map):
    feature_list = []
    
    for i in range(len(grapheme_map["grapheme"])):
        grapheme = grapheme_map["grapheme"][i]

        # Convert grapheme to ASCII values?
        # grapheme_numeric = [ord(c) for c in grapheme]
        grapheme_numeric = [c for c in grapheme]

        # Retrieve corresponding timing features (ensure they exist)
        time_delta = grapheme_map["time delta"][i] if i < len(grapheme_map["time delta"]) else 0
        dwell_time = grapheme_map["dwell time"][i] if i < len(grapheme_map["dwell time"]) else 0
        flight_time = grapheme_map["flight time"][i] if i < len(grapheme_map["flight time"]) else 0

        # Concatenate into a single feature vector
        feature_vector = grapheme_numeric + [time_delta, dwell_time, flight_time]

        feature_list.append(feature_vector)

    return np.array(feature_list)

if __name__ == '__main__':
    from read_binary import read_keystroke_logger_output
    from pylibgrapheme import create_grapheme_map, GraphemeType

    file_path = "./output/infoTest.bin"

    # Read the binary keystroke data
    user_info, sessions_data = read_keystroke_logger_output(file_path)

    print(f"User Info: {user_info}")

    # Limit to just for sessions for now to see how it works
    for i, session in enumerate(sessions_data[:1]):
        print(f"\nProcessing Session {i+1}...")

        # Call create_grapheme_map for digraphs
        grapheme_map = create_grapheme_map(session, GraphemeType.DIGRAPH)

        if grapheme_map:
            print(f"Grapheme Map for Session {i+1}:")
            for key, value in grapheme_map.items():
                # Print the first 5 for now
                print(f"{key}: {value[:5]}")

            flattened_data = flatten_grapheme_map(grapheme_map)
            print(f"\n\nFlattened Features for Session {i+1}:")
            print(flattened_data[:5])  # Print first 5 feature vectors for brevity
        else:
            print(f"[ERROR] Failed to process Session {i+1}.")
