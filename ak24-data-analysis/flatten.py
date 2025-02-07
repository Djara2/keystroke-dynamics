import numpy as np

# Flatten the grapheme map into individual feature vectors.
def flatten_grapheme_map(grapheme_map: dict) -> np.ndarray:
    feature_list = []

    # Iterate over each grapheme and extract the necessary features
    for i in range(len(grapheme_map["grapheme"])):
        grapheme = grapheme_map["grapheme"][i]

        # Convert grapheme to ASCII values
        grapheme_numeric = [ord(c) for c in grapheme]

        # Retrieve corresponding timing features
        time_delta = grapheme_map["time delta"][i] if i < len(grapheme_map["time delta"]) else 0
        dwell_time = grapheme_map["dwell time"][i] if i < len(grapheme_map["dwell time"]) else 0
        flight_time = grapheme_map["flight time"][i] if i < len(grapheme_map["flight time"]) else 0

        # Concatenate into a single feature vector
        feature_vector = grapheme_numeric + [time_delta, dwell_time, flight_time]

        # Add the feature vector to the list
        feature_list.append(feature_vector)

    # Convert the list of feature vectors to a numpy array and return it
    return np.array(feature_list)


# Example of how this function might be used
if __name__ == "__main__":
    # Sample grapheme map to simulate the data structure
    sample_grapheme_map = {
        "grapheme": ["ab", "bc", "cd", "de"],
        "time delta": [0.1, 0.2, 0.3, 0.4],
        "dwell time": [0.5, 0.6, 0.7, 0.8],
        "flight time": [0.9, 1.0, 1.1, 1.2]
    }

    # Flatten the grapheme map into feature vectors
    flattened_features = flatten_grapheme_map(sample_grapheme_map)

    # Print the flattened features
    print("Flattened Features:")
    print(flattened_features)
