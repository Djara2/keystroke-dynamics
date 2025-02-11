#!/usr/bin/env python3

from sklearn import datasets
from sklearn.model_selection import train_test_split
from sklearn.svm import SVC
from sklearn.metrics import accuracy_score

from typing import Type, List
from dataclasses import dataclass

""" 
This is the OvA implementation. N classes -> N classifiers.

This is better if you have more than 15 classifications.
"""

TEST_SIZE: float = 0.2

@dataclass
class SVMOutput:
    decision_scores: list
    final_predictions: list
    accuracy_score: float

def ova_svm_demo_with_wine():
    """
    Demonstrates the basic functionality of the OvA SVM from sklearn using their wine dataset. 
    Parameters: None
    Return:     Accuracy score (float)
    """
    # Import wine dataset
    wine = datasets.load_wine()
    x, y = wine.data, wine.target
    
    # Split dataset into training and testing
    x_training_data, x_testing_data, y_training_data, y_testing_data =  train_test_split(x, y, test_size = 0.3, random_state = 42)

    # Create the SVM classifier
    svm_ova = SVC(decision_function_shape="ovr", probability=True)
    svm_ova.fit(x_training_data, y_training_data)

    # Predict and evaluate the OvA model
    decision_scores_ova = svm_ova.decision_function(x_testing_data)
    print("DECISION SCORES: (1) When passed the entire training data, this will be a list of lists." )
    print("                 (2) The 1st list corresponds to the first row in the training data.")
    print("                     Likewise the 2nd list corresponds to the second row in the training data.")
    print("                 (3) Each list will be size N, where N is the number of classes/classifications possible.")
    print("                 (4) The value within each list is a probability score for the classification. The index of")
    print("                     each value corresponds to the class/classification. For example the prediction score")
    print("                     for class 0 is at index 0. The prediction score for class 1 is at index 1.")
    print("Decision scores (indices are classes): {}".format(decision_scores_ova))
    print()

    final_prediction_ova = svm_ova.predict(x_testing_data)
    print("FINAL PREDICTION: (1) When passed the entire training data, this will be a singular 1D list.")
    print("                  (2) Each value inside the list is the predicted classification.")
    print("                      For example, the 1st item in the list is the predicted class number (0 to n)")
    print("                      for the 1st row in the training data set.")
    print("                  (3) If prediction[0] = 5 then the prediction is class 5 for the 1st row in the data.")
    print("final prediction: {}".format(final_prediction_ova))
    print()
    
    # y_prediction_proba_ova = svm_ova.predict_proba(x_testing_data)
    # print("y_prediction_proba_ova: {}".format(y_prediction_proba_ova))

    ova_accuracy_score = accuracy_score(y_testing_data, final_prediction_ova)
    print("OvA accuracy: {}".format(ova_accuracy_score))

def ova_svm(x_data: list[list], y_data: list) -> SVMOutput:
    # Split into training and testing data
    x_train, x_test, y_train, y_test = train_test_split(x_data, y_data, test_size = TEST_SIZE, random_state = 42)

    # Create the SVM classifier
    svm_ova = SVC(decision_function_shape="ovr", probability=True)
    svm_ova.fit(x_train, y_train)

    # Get predictions, final prediction, and accuracy score
    decision_scores = svm_ova.decision_function(x_test)
    final_predictions = svm_ova.predict(x_test)
    svm_accuracy_score = accuracy_score(y_test, final_predictions)
    output = SVMOutput(decision_scores, final_predictions, svm_accuracy_score)
    
    return output

if __name__ ==  "__main__":
    wine: list = datasets.load_wine()
    x, y = wine.data, wine.target
    output = ova_svm(x, y)
    print("decision scores: {}".format(output.decision_scores))
    print("final predictions: {}".format(output.final_predictions))
    print("accuracy_score: {}".format(output.accuracy_score))
