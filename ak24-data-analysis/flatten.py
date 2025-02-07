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

