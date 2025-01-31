import numpy
from collections import Counter


def knn_euclidean(training_data, classifier_list, test_point, k):
    distances = []
    for i in range(len(training_data)):
        distance = numpy.sqrt( numpy.sum( (numpy.array(test_point) - numpy.array(training_data[i]))**2 ) )
        distances.append((distance, classifier_list[i]))
    distances.sort(key=lambda x: x[0])
    k_nearest_labels = [label for _, label in distances[:k]]
    return Counter(k_nearest_labels).most_common(1)[0][0]




def knn_manhattan(training_data, classifier_list, test_point, k):
    distances = []
    for i in range(len(training_data)):
        distance = ( numpy.sum( numpy.array(test_point) - numpy.array(training_data[i]) ) )
        distances.append((distance, classifier_list[i]))
    distances.sort(key=lambda x: x[0])
    k_nearest_labels = [label for _, label in distances[:k]]
    return Counter(k_nearest_labels).most_common(1)[0][0]



training_data = [[51, 565], [654, 54], [6541, 654], [65,56], [321, 48]]
classifier_list = ['person', 'imposter', 'person', 'person', 'imposter']
test_point = [652, 98]
k = 3

predicted_classifier_euclidean = knn_euclidean(training_data, classifier_list, test_point, k)
predicted_classifier_manhattan = knn_manhattan(training_data, classifier_list, test_point, k)

print(predicted_classifier_euclidean)
print(predicted_classifier_manhattan)