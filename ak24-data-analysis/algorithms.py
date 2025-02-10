# import numpy as np
# from collections import Counter
# import random
# from scipy.stats import norm, ks_2samp
from enum import auto, Enum
from statistics import mean, stdev


class Error(Enum):
    SUCCESS = auto()
    INVALID_USER = auto()
    LIBRARY_FAILURE = auto()
    NO_DATA = auto()


class Constant(Enum):
    ALPHA = 0.05
    TRAINING_PERCENT = 0.8
    CORRELATION_COEFFICIENT = 0.6


# Feature Selection
# Look at Principal Component Analysis
def pearson_correlation(raw_data: dict) -> dict:
    # Check to make sure data exists
    if raw_data == None:
        print(f'[pearson_correlation] Data was not found in the raw_data input.\n')
        return Error.NO_DATA
    feature_list: list[str] = [key for key in raw_data.keys() if key not in ("SequenceNumber", "User")] # converting to list for iterability
    # Assign an importance of 1 to each feature. The idea is to compare two features for correlation and if they are correlated, reduce the importance of one feature to 0.
    feature_importance: dict = {key:1 for key in feature_list}
    n: int = max(raw_data["SequenceNumber"]) # n needs to be the number of rows in the data
    for index_left in range(n-1):
        left_feature: str = feature_list[index_left]
        for index_right in range(index_left, n-1):
            right_feature: str = feature_list[index_right]
            if(feature_importance[left_feature] and feature_importance[right_feature]):
                #  The link to the formula used: https://www.geeksforgeeks.org/pearson-correlation-coefficient/
                x_data: list = feature_list[index_left]
                y_data: list = feature_list[index_right]
                xy_data: list = [x+y for x in x_data for y in y_data]
                x_squared: list = [x*x for x in x_data]
                y_squared: list = [y*y for y in y_data]

                x_sum: int = sum(x_data)
                y_sum: int = sum(y_data)
                xy_sum: int = sum(xy_data)
                x_squared_sum: int = sum(x_squared)
                y_squared_sum: int = sum(y_squared)

                r_numerator = ( n * xy_sum ) - ( x_sum * y_sum )
                r_denomenator_left = ( n * x_squared_sum ) - ( x_sum ** 2 )
                r_denomenator_right = ( n * y_squared_sum ) - ( y_sum ** 2 )

                r_value = r_numerator / ( ( r_denomenator_left * r_denomenator_right ) ** .5 )

                if(r_value < Constant.CORRELATION_COEFFICIENT):
                    feature_importance[right_feature] = 0
    # returning the feature importance dictionary. This will be used to know which features are important if they have a 1. 
    return feature_importance




def principal_component_analysis(raw_data: dict, raw_data_as_2d_array: list[list[float]]) -> dict:
    # to perform PCA:
    # 1. Standardize the data (Convert all data to have a mean of 0 and standard deviation of 1)
    standard_data: dict = {feature:[ ( feature_index - mean(raw_data[feature] ) ) / stdev(raw_data[feature]) for feature_index in raw_data[feature]] for feature in raw_data.keys()}

    # 2. Get the covariance matrix
    standard_data_as_2d_array: list[list[float]] = 

    covariance_matrix: list[list[float]] = np.cov(standard_data_as_2d_array)

    return covariance_matrix




def kolmogorov_smirnov_test(train_data: dict, test_data: dict) -> bool|Error:
    # returns a boolean except on error, which then returns None
    try:
        user: str = test_data["user"]
        user_data: list[dict] = [user_session for user_session in train_data if user_session["user"] == user]
    except:
        print(f'[kolmogorov_smirnov_test] The user provided was not found within the training data.\n')
        return Error.INVALID_USER
    try:
        # Perform the kolmogorov-smirnov test on the digraph and trigraph independent of each other. 
        di_ks_statistic, di_p_value = ks_2samp(test_data[user]["digraph"], train_data[user]["digraph"])
        tri_ks_statistic, tri_p_value = ks_2samp(test_data[user]["trigraph"], train_data[user]["trigraph"])

        # False represents a rejection of the null hypothesis whereas a True represents a failure to reject the null hypothesis, which in a way means an acceptance.
        return False if di_p_value < Constant.ALPHA and tri_p_value < Constant.ALPHA else True
    except Exception as e:
        print(f'[kolmogorov_smirnov_test] An error occured when running the ks_2samp function from the scipy.stats library.\nError from library:\n{e}\n')
        return Error.LIBRARY_FAILURE





def csv_to_python() -> dict:
    with open('master_dict_output.csv', 'r') as csv:
        matrix: list[str] = csv.readlines()
        csv.close()
    max_sequence_number = len(matrix)
    # row[0] represents the features. The following rows are the data in matrix-ish form. i represents a given row, j represents a column such that data that shares the same j index is in the same column of the table
    features: list[str] = matrix[0].split(',')
    raw_data_dictionary: dict = { features[j].strip():[ float(matrix[i].split(',')[j].strip()) for i in range( 1, max_sequence_number ) ] for j in range( len( features ) ) }
    
    return raw_data_dictionary



def main():
    # Step 1:
    #   create the data dictionary from the csv file
    raw_data_in_table_form: dict = csv_to_python()

    raw_data_as_2d_array: list[list[float]] = [raw_data_in_table_form[feature] for feature in raw_data_in_table_form.keys() if feature not in ("SequenceNumber", "User")]

    result = principal_component_analysis(raw_data_in_table_form)

    print(result)


main()

# training_data = [[51, 565], [654, 54], [6541, 654], [65, 56], [321, 48], [652, 98]]
# classifier_list = ['person', 'imposter', 'person', 'person', 'imposter', 'imposter']

# predicted_classifier_euclidean = knn_euclidean(training_data, classifier_list, test_point, k)
# predicted_classifier_manhattan = knn_manhattan(training_data, classifier_list, test_point, k)

# print(predicted_classifier_euclidean)
# print(predicted_classifier_manhattan)


# Training data is full data
# Testing data will be a seperate


# # Classifiers
# def knn_euclidean(training_data, classifier_list, test_point, k):
#     distances = []
#     for i in range(len(training_data)):
#         distance = np.sqrt( np.sum( (np.array(test_point) - np.array(training_data[i]))**2 ) )
#         distances.append((distance, classifier_list[i]))
#     distances.sort(key=lambda x: x[0])
#     k_nearest_labels = [label for _, label in distances[:k]]
#     return Counter(k_nearest_labels).most_common(1)[0][0]


# def knn_manhattan(training_data, classifier_list, test_point, k):
#     distances = []
#     for i in range(len(training_data)):
#         distance = ( np.sum( np.array(test_point) - np.array(training_data[i]) ) )
#         distances.append((distance, classifier_list[i]))
#     distances.sort(key=lambda x: x[0])
#     k_nearest_labels = [label for _, label in distances[:k]]
#     return Counter(k_nearest_labels).most_common(1)[0][0]


