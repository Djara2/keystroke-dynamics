# import numpy as np
# import pandas
# from scipy.stats import ks_2samp
from enum import auto, Enum
# from sklearn.decomposition import PCA


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
    # Assign an importance of 1 to each feature. The idea is to compare two features for correlation and if they are correlated, reduce the importance of one feature to 0.
    feature_importance: dict = {key:1 for key in raw_data.keys()}
    n: int = max(raw_data[raw_data.keys()[0]]) # n needs to be the number of rows in the data
    for index_left in range(n-1):
        left_feature: str = raw_data[index_left]
        for index_right in range(index_left, n-1):
            right_feature: str = raw_data[index_right]
            if(feature_importance[left_feature] and feature_importance[right_feature]):
                #  The link to the formula used: https://www.geeksforgeeks.org/pearson-correlation-coefficient/
                x_data: list = raw_data[index_left]
                y_data: list = raw_data[index_right]
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
    for feature in raw_data:
        if(feature_importance[feature] == 0):
            raw_data.drop(columns=feature, inplace=True)
    return raw_data




def principal_component_analysis(raw_data) -> dict:
    # to perform PCA:
    # 1. Standardize the data (Convert all data to have a mean of 0 and standard deviation of 1)
    # standard_data: dict = {feature:[ ( feature_index - mean(raw_data[feature] ) ) / stdev(raw_data[feature]) for feature_index in raw_data[feature]] for feature in raw_data.keys()}

    # # 2. Get the covariance matrix
    # covariance_matrix: list[list[float]] = np.cov(raw_data_as_2d_array)

    raw_mean = raw_data.mean()
    raw_standard_deviation = raw_data.std()
    z = (raw_data - raw_mean) / raw_standard_deviation

    covariance_matrix = z.cov()

    eigenvalues, eigenvectors = np.linalg.eig(covariance_matrix)

    eigen_index_descending = eigenvalues.argsort()[::-1]
    eigenvalues = eigenvalues[eigen_index_descending]
    eigenvectors = eigenvectors[:,eigen_index_descending]

    explained_var = np.cumsum(eigenvalues) / np.sum(eigenvalues)

    number_of_principal_components: int = np.argmax(explained_var >= 0.8) + 1

    unit_matrix = eigenvectors[:,:number_of_principal_components]

    selected_dataframe = pandas.Dataframe(unit_matrix, columns=raw_data.columns.to_numpy().toList())


    return selected_dataframe




def kolmogorov_smirnov_test(train_data: dict, test_data: dict) -> bool|Error:
    # returns a boolean except on error, which then returns None
    try:
        user: str = test_data["User"]
        user_data: dict = train_data[train_data["User"] == user].drop(columns=["SequenceNumber", "User"], inplace=True)
        test_data = test_data.drop(columns=["SequenceNumber", "User"], inplace=True)
    except:
        print(f'[kolmogorov_smirnov_test] The user provided was not found within the training data.\n')
        return Error.INVALID_USER
    try:
        # Perform the kolmogorov-smirnov test on the digraph and trigraph independent of each other. 
        ks_statistic, p_value = ks_2samp(test_data, user_data)

        # False represents a rejection of the null hypothesis whereas a True represents a failure to reject the null hypothesis, which in a way means an acceptance.
        return False if p_value < Constant.ALPHA  else True
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

    pandas_raw_data = pandas.DataFrame(raw_data_in_table_form, index=raw_data_in_table_form.keys())

    # raw_data_as_2d_array: list[list[float]] = [raw_data_in_table_form[feature] for feature in raw_data_in_table_form.keys() if feature not in ("SequenceNumber", "User")]

    result = principal_component_analysis(pandas_raw_data)

    print(result)


# main()




def test():
    array = [["hello", "goodbye"], [0, 2], [5, 6]]

    array = array[::-1]

    print(array)



test()