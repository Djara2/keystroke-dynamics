from read_binary import read_keystroke_logger_output
from pylibgrapheme import create_grapheme_map, get_combinations, GraphemeType
from masterDictionaryBuilder import create_combined_dictionary, write_to_csv, update_master_dictionary, print_master_dictionary

from knn import preprocess_features, train_knn
from algorithms import principal_component_analysis, pearson_correlation
from neural_net import standardize_data, run_neural_net

import pandas
import numpy as np
import argparse
import matplotlib.pyplot as plt

def process_sessions(sessions, user_info):
   
    for i, session in enumerate(sessions):
        # Create grapheme map for each session
        grapheme_map_error_code, grapheme_map = create_grapheme_map(session, GraphemeType.DIGRAPH)

        # Temperary fix, since these two lists have 1 less value than the rest
        grapheme_map["time_delta"].append(0)
        grapheme_map["flight_time"].append(0)

        # If a grapheme map exists
        if grapheme_map:
            # Create a combined_dictionary for it
            combined_dictionary = create_combined_dictionary(grapheme_map, user_info)
            
            # Update master dictionary with combined dictionary
            update_master_dictionary(combined_dictionary)

        else:
            print("[ERROR] Failed to process session.")

def read_csv_file(csv_file_path) -> tuple[pandas.DataFrame, pandas.Series]:
    # Load in the csv file
    data_frame = pandas.read_csv(csv_file_path)

    # Drop SequenceNumber (not a feature)
    data_frame.drop(columns=["SequenceNumber"], inplace=True)

    # Extract features and labels
    y = data_frame["User"]
    X = data_frame.drop(columns=["User"])

    return X, y

def perform_knn(X, y):
    #Peform KNN
    X_scaled = preprocess_features(X)
    accuracy, conf_matrix, class_report, (y_test, y_pred) = train_knn(X_scaled, y, k=5)

    # Print results
    print("\nConfusion Matrix:\n", conf_matrix)
    print("\nAccuracy:", accuracy)
    print("\nClassification Report:\n", class_report)

    # Show true vs. predicted labels
    print("\nTrue vs Predicted Labels:")
    for true_label, pred_label in zip(y_test, y_pred):
        print(f"True: {true_label}, Predicted: {pred_label}")

def perform_neural_net(X, y):

    X_scaled, y_encoded = standardize_data(X, y)

    history = run_neural_net(X_scaled, y_encoded)

    # Plot accuracy over epochs
    plt.plot(history.history["accuracy"], label="Train Accuracy")
    plt.plot(history.history["val_accuracy"], label="Validation Accuracy")
    plt.title("Model Accuracy Over Epochs")
    plt.xlabel("Epoch")
    plt.ylabel("Accuracy")
    plt.legend()
    plt.show()

def parse_arguments():
    # Initialize argument parser
    parser = argparse.ArgumentParser(description="Process multiple file paths.")

    # Add argument for multiple file paths
    parser.add_argument(
        'file_paths', 
        nargs='+',  # '+' means one or more arguments are required
        help="Paths to the input files"
    )

    # Parse the arguments
    args = parser.parse_args()
    
    return args.file_paths  

def main():
    # Get file paths from arguments
    file_paths = parse_arguments()

    # Print out the paths for verification
    print(f"Files to process: {file_paths}")
    
    # Loop through the file paths
    for file_path in file_paths:
        # Read sessions from the binary file
        user_info, sessions_data = read_keystroke_logger_output(file_path)

        if sessions_data:
            # Process sessions and update master dictionary
            process_sessions(sessions_data, user_info)

        else:
            print("[ERROR] No valid session data found.")

    
    # Write the master dictionary to a csv file
    write_to_csv()

    # Read in the csv file data to use with classifers
    X, y = read_csv_file("master_dict_output.csv")

    # Perform feaure selection with PCA
    #selected_features = principal_component_analysis(X)

    # Make complex numbers into real numbers and make it into a dataframe for classifers
    #X_pca = selected_features.astype(np.float64)
    #X_pca = pandas.DataFrame(X_pca, columns=selected_features.columns)


    # Perform KNN
    perform_knn(X, y)

    # Neural Net
    perform_neural_net(X, y)



if __name__ == "__main__":
    main()

