import numpy as np

# Flatten the grapheme map into a single 1D feature vector.
def flatten_grapheme_map(grapheme_map: dict) -> np.ndarray:
    feature_list = []

    # Iterate over each grapheme and extract features
    for i in range(len(grapheme_map["grapheme"])):
        grapheme = grapheme_map["grapheme"][i]

        # Convert grapheme to ASCII values (No padding, just flattening)
        grapheme_numeric = [ord(c) for c in grapheme]

        # Retrieve corresponding timing features (corrected to match key names)
        time_delta = grapheme_map["time_delta"][i] if i < len(grapheme_map["time_delta"]) else 0
        dwell_time = grapheme_map["dwell_time"][i] if i < len(grapheme_map["dwell_time"]) else 0
        flight_time = grapheme_map["flight_time"][i] if i < len(grapheme_map["flight_time"]) else 0

        # Flatten everything into a single list
        feature_list.extend(grapheme_numeric + [time_delta, dwell_time, flight_time])

    # Convert the list into a single 1D NumPy array
    return np.array(feature_list, dtype=np.float64)


# Example of how this function might be used
if __name__ == "__main__":
    # Sample grapheme map to simulate the data structure
    sample_grapheme_map = {
        "#"             : [1, 2, 3, 4],
        "grapheme"      : ["ab", "bc", "cd", "de"],
        "time_delta"    : [0.1, 0.2, 0.3, 0.4],
        "dwell_time"    : [0.5, 0.6, 0.7, 0.8],
        "flight_time"   : [0.9, 1.0, 1.1, 1.2]
    }

    # Flatten the grapheme map into feature vectors
    flattened_features = flatten_grapheme_map(sample_grapheme_map)

    # Print the flattened features
    print("Flattened Features:")
    print(flattened_features)
