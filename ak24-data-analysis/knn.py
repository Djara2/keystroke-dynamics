import pandas
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder, StandardScaler
from sklearn.neighbors import KNeighborsClassifier
from sklearn.metrics import accuracy_score
from enum import IntEnum 

class MinimumValues(IntEnum):
    MINIMUM_K = 2

# Load and preprocess data from csv
def load_and_preprocess_data(csv_file_path):
    try:
        # Load in the csv file
        data_frame = pandas.read_csv(csv_file_path)

        # Drop SequenceNumber (not a feature)
        data_frame.drop(columns=["SequenceNumber"], inplace=True)

        # Extract features and labels
        y = data_frame["User"]
        X = data_frame.drop(columns=["User"])

        # Standardize features but preserve -1 values
        scaler = StandardScaler()
        X_scaled = X.copy()

        # Avoid scaling -1 values
        mask = X != -1
        X_scaled[mask] = scaler.fit_transform(X[mask])

        return X_scaled, y

    except Exception as error:
        print(f"[load_and_process_data] Data Loading error: {error}")

# Train a KNN model and return accuracy and the trained model
def train_knn(X, y, k=2, test_size=0.2, random_state=42):

    try:
        if k < MinimumValues.MINIMUM_K:
            raise ValueError("Error: k must be a positive integer greater than 1")

        # Split data into training and testing sets
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=test_size, random_state=random_state)

        # Train KNN model
        knn = KNeighborsClassifier(n_neighbors=k, metric="euclidean")
        knn.fit(X_train, y_train)

        # Predict and evaluate
        y_pred = knn.predict(X_test)
        accuracy = accuracy_score(y_test, y_pred)

        return knn, accuracy

    except Exception as error:
        print(f"[train_knn] KNN Training Error: {error}")

if __name__ == "__main__":
    csv_file = "master_dict_output.csv"
    X, y = load_and_preprocess_data(csv_file)
    knn_model, acc = train_knn(X, y, k=2)
    print(f"KNN Accuracy: {acc:.2f}")
