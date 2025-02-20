from read_binary import read_keystroke_logger_output
from pylibgrapheme import create_grapheme_map, get_combinations, GraphemeType
from masterDictionaryBuilder import create_combined_dictionary, write_to_csv, update_master_dictionary, print_master_dictionary

from knn import knn
from algorithms import kolmogorov_smirnov_test, Error
from neural_net import run_neural_net
from ova_svm import ova_svm
from feature_selection import feature_selection, feature_select_with_threshold

import pandas
import numpy as np
import argparse
import matplotlib.pyplot as plt
from sklearn.metrics import accuracy_score, confusion_matrix, classification_report
from sklearn.model_selection import train_test_split
from sklearn.decomposition import PCA
import os



def process_sessions(sessions, user_info):
   
    for i, session in enumerate(sessions):
        # Create grapheme map for each session
        grapheme_map_error_code, grapheme_map = create_grapheme_map(session, GraphemeType.DIGRAPH)

        # Temperary fix, since these two lists have 1 less value than the rest
        grapheme_map["time_delta"].append(-1)
        grapheme_map["flight_time"].append(-1)

        # If a grapheme map exists
        if grapheme_map:
            # Create a combined_dictionary for it
            combined_dictionary = create_combined_dictionary(grapheme_map, user_info)
            
            # Update master dictionary with combined dictionary
            update_master_dictionary(combined_dictionary)

        else:
            print("[ERROR] Failed to process session.")

# Function to read the CSV and return features and labels
def read_csv_file(csv_file_path):
    """
    Read the CSV file, return features (X) and labels (y)
    """
    data_frame = pandas.read_csv(csv_file_path)
    y = data_frame["User"]
    X = data_frame.drop(columns=["User", "SequenceNumber"])  # Drop 'User' and 'SequenceNumber' columns
    return X, y

def perform_knn(X, y):
    #Peform KNN
    accuracy, conf_matrix, class_report = knn(X, y, 5, "cosine")

    # Print results
    print("\nK-Nearest Neighbors (KNN):")
    print("\nConfusion Matrix:\n", conf_matrix)
    print("\nAccuracy:", accuracy)
    print("\nClassification Report:\n", class_report)

def perform_svm(X, y):
    # Perform SVM
    output = ova_svm(X, y)

    # Print results
    print("\nSupport Vector Machine (SVM):")
    print("decision scores: {}".format(output.decision_scores))
    print("final predictions: {}".format(output.final_predictions))
    print("accuracy_score: {}\n".format(output.accuracy_score))

def perform_neural_net(X, y):
    # Perform Neural Network
    history = run_neural_net(X, y)

    # Plot accuracy over epochs
    plt.plot(history.history["accuracy"], label="Train Accuracy")
    plt.plot(history.history["val_accuracy"], label="Validation Accuracy")
    plt.title("Model Accuracy Over Epochs")
    plt.xlabel("Epoch")
    plt.ylabel("Accuracy")
    plt.legend()
    plt.show()

def perform_ks_test(X, y):

    # Add `User` column back to X since kolmogorov_smirnov_test() requires it
    X_with_labels = X.copy()
    X_with_labels["User"] = y  # Restore the User column

    # Split into training and testing sets
    train_data, test_data = train_test_split(X_with_labels, test_size=0.2, random_state=42)

    # Store predictions
    y_pred = []

    # Iterate through each test sample
    for _, test_sample in test_data.iterrows():
        test_sample_df = test_sample.to_frame().T  # Convert single row to DataFrame

        # Run Kolmogorov-Smirnov test
        result = kolmogorov_smirnov_test(train_data, test_sample_df)

        # Assign classification based on result
        if result == Error.INVALID_USER or result == Error.LIBRARY_FAILURE:
            y_pred.append("Unknown")
        elif result:  
            y_pred.append(test_sample["User"])  # If True, assign same user
        else:  
            y_pred.append("Unknown")

    # Convert lists to NumPy arrays for evaluation
    y_test = test_data["User"].values
    y_pred = np.array(y_pred)

    # Compute evaluation metrics
    accuracy = accuracy_score(y_test, y_pred)
    conf_matrix = confusion_matrix(y_test, y_pred, labels=np.unique(y))
    class_report = classification_report(y_test, y_pred, labels=np.unique(y), zero_division=0)

    # Print results
    print("\nKolmogorov-Smirnov Classifier:")
    print("\nConfusion Matrix:\n", conf_matrix)
    print("\nAccuracy:", accuracy)  # Print the accuracy
    print("\nClassification Report:\n", class_report)

def parse_arguments():
    # Initialize argument parser
    parser = argparse.ArgumentParser(description="Process multiple file paths.")

    # Add argument for multiple directory paths
    parser.add_argument(
        'directories', 
        nargs='+',  # '+' means one or more arguments are required
        help="Paths to the directories of the file"
    )

    # Parse the arguments
    args = parser.parse_args()
    
    return args.directories 

def main():
    # Get directory paths from arguments
    directories = parse_arguments()

    # Print out the paths for verification
    print(f"Directories to process: {directories}")
    
    # Loop through directories
    for directory in directories:
        # Loop through the file paths
        for file_path in os.listdir(directory):

            full_file_path = os.path.join(directory, file_path)
            
            # Read sessions from the binary file
            user_info, sessions_data = read_keystroke_logger_output(full_file_path)

            if sessions_data:
                # Process sessions and update master dictionary
                process_sessions(sessions_data, user_info)

            else:
                print("[ERROR] No valid session data found.")

    
    # Write the master dictionary to a csv file
    write_to_csv()

    # Read in the csv file data to use with classifers
    X, y = read_csv_file("master_dict_output.csv")


    # Perform feature selection
    #X_selected, selected_features = feature_selection_with_threshold(X, y, p_value_threshold=0.05)
    X_selected, selected_features = feature_selection(X, y, k=300)

    # Print the shape of the reduced feature set
    print("\nAfter feature selection:")
    print(X_selected.shape)  


    # Apply PCA to reduce dimensionality further
    pca = PCA(n_components=0.8)  # Keep 80% of variance
    X_pca = pca.fit_transform(X_selected)
    # Print the shape after PCA
    print(f"\nAfter PCA: {X_pca.shape}")  


    """
    Run the classifers after the feature selection

    DOESN'T CURRENTLY USE PCA FOR FURTHER DIMENSIONALITY REDUCTION
    """
    # Run Kolmogrov
    perform_ks_test(X_selected, y)

    # Perform KNN
    perform_knn(X_selected, y)

    # SVM
    perform_svm(X_selected, y)

    # Neural Net
    perform_neural_net(X_selected, y)


if __name__ == "__main__":
    main()

