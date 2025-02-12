from read_binary import read_keystroke_logger_output
from pylibgrapheme import create_grapheme_map, get_combinations, GraphemeType
from masterDictionaryBuilder import create_combined_dictionary, write_to_csv, update_master_dictionary, print_master_dictionary

from knn import preprocess_features, train_knn
from algorithms import kolmogorov_smirnov_test
from neural_net import standardize_data, run_neural_net
from ova_svm import ova_svm

import pandas
import numpy as np
import argparse
import matplotlib.pyplot as plt
from sklearn.metrics import accuracy_score, confusion_matrix, classification_report
from sklearn.model_selection import train_test_split


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
    print("\nK-Nearest Neighbors (KNN):")
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

def perform_svm(X, y):

    output = ova_svm(X, y)
    print("\nSupport Vector Machine (SVM):")
    print("decision scores: {}".format(output.decision_scores))
    print("final predictions: {}".format(output.final_predictions))
    print("accuracy_score: {}\n".format(output.accuracy_score))


def perform_ks_test(X, y):
    # Split data into train and test sets
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

    # Store predictions
    y_pred = []

    # Add 'User' column back to X_train and X_test for kolmogrov
    X_train["User"] = y_train
    X_test["User"] = y_test

    # Run KS test on each test sample
    for i, (index, test_sample) in enumerate(X_test.iterrows()):
        # Extract the user from the test sample
        user = y_test.iloc[i]

        # Prepare the test row (add user info back to the test row)
        test_row = test_sample.to_frame().T  # Convert to DataFrame (single row)
        test_row["User"] = user  # Add user information to test_row

        # Call the Kolmogorov-Smirnov test with the train data and the test sample
        result = kolmogorov_smirnov_test(X_train, test_row)

        # Assign classification based on the result
        if result:  
            predicted_label = y_train.mode()[0]
        else:  
            predicted_label = "Unknown"

        y_pred.append(predicted_label)

    # Convert lists to NumPy arrays for evaluation
    y_pred = np.array(y_pred)
    y_test = np.array(y_test)

    # Compute evaluation metrics
    accuracy = accuracy_score(y_test, y_pred)  # Calculate accuracy
    conf_matrix = confusion_matrix(y_test, y_pred, labels=np.unique(y))
    
    # Avoid UndefinedMetricWarning: handle zero division using zero_division=0
    class_report = classification_report(y_test, y_pred, labels=np.unique(y), zero_division=0)

    # Print results
    print("\nKolmogorov-Smirnov Classifier:")
    print("\nConfusion Matrix:\n", conf_matrix)
    print("\nAccuracy:", accuracy)  # Print the accuracy
    print("\nClassification Report:\n", class_report)


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

    # FEATURE SELECTION


    # Run Kolmogrov
    perform_ks_test(X, y)

    # Perform KNN
    perform_knn(X, y)

    # SVM
    perform_svm(X, y)

    # Neural Net
    perform_neural_net(X, y)



if __name__ == "__main__":
    main()

