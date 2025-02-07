import numpy as np
from collections import Counter
import random
from scipy.stats import norm, ks_2samp

percent_to_train = 0.8
alpha = 0.05


def knn_euclidean(training_data, classifier_list, test_point, k):
    distances = []
    for i in range(len(training_data)):
        distance = np.sqrt( np.sum( (np.array(test_point) - np.array(training_data[i]))**2 ) )
        distances.append((distance, classifier_list[i]))
    distances.sort(key=lambda x: x[0])
    k_nearest_labels = [label for _, label in distances[:k]]
    return Counter(k_nearest_labels).most_common(1)[0][0]




def knn_manhattan(training_data, classifier_list, test_point, k):
    distances = []
    for i in range(len(training_data)):
        distance = ( np.sum( np.array(test_point) - np.array(training_data[i]) ) )
        distances.append((distance, classifier_list[i]))
    distances.sort(key=lambda x: x[0])
    k_nearest_labels = [label for _, label in distances[:k]]
    return Counter(k_nearest_labels).most_common(1)[0][0]


# Kolmogorov-Smirno Test
train_data = []
def kolmogorov_smirnov_classifier(data_row):
    train_data.append(data_row)



def kolmogorov_smirnov_test(test_data, user):
    try:
        # Perform the ks test on the digraph and trigraph independent of each other. 
        di_ks_statistic, di_p_value = ks_2samp(test_data[user]["digraph"], train_data[user]["digraph"])
        tri_ks_statistic, tri_p_value = ks_2samp(test_data[user]["trigraph"], train_data[user]["trigraph"])

        # False represents a rejection of the null hypothesis whereas a True represents a failure to reject the null hypothesis which in a way means an acceptance.
        return False if di_p_value < alpha and tri_p_value < alpha else True
    except:
        print("[General Error] User may not be in the dataset OR incorrect feature labels OR ks_2samp errored.")
        return





# training_data = [[51, 565], [654, 54], [6541, 654], [65, 56], [321, 48], [652, 98]]
# classifier_list = ['person', 'imposter', 'person', 'person', 'imposter', 'imposter']

# predicted_classifier_euclidean = knn_euclidean(training_data, classifier_list, test_point, k)
# predicted_classifier_manhattan = knn_manhattan(training_data, classifier_list, test_point, k)

# print(predicted_classifier_euclidean)
# print(predicted_classifier_manhattan)


# Training data is full data
# Testing data will be a seperate