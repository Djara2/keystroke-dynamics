import os
import pandas as pd
import numpy as np
import keras
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Input
from tensorflow.keras.regularizers import l2
import matplotlib.pyplot as plt
from sklearn.preprocessing import OneHotEncoder, StandardScaler
from sklearn.model_selection import train_test_split
from tensorflow.keras.callbacks import EarlyStopping

# Print TensorFlow version
print(f"Using TensorFlow version: {tf.__version__}")

# Specify the correct path to the CSV
csv_file = "ak24-data-analysis/master_dict_output.csv"

# Check if the file exists before proceeding
if not os.path.exists(csv_file):
    raise FileNotFoundError(f"Error: '{csv_file}' not found in {os.getcwd()}")

# Load the dataset
dataset = pd.read_csv(csv_file)

# Ensure necessary columns exist
if "User" not in dataset.columns or "SequenceNumber" not in dataset.columns:
    raise ValueError("Error: Required columns ('User', 'SequenceNumber') not found in dataset.")

# Drop `SequenceNumber` column
dataset = dataset.drop(columns=["SequenceNumber"])

# Extract features and labels
X = dataset.drop(columns=["User"]).values  # Keystroke timing features
y = dataset["User"].values  # User classification labels

# Standardize features while preserving -1 values
scaler = StandardScaler()
mask = X != -1  # Mask to avoid modifying -1 values
X_scaled = X.copy()

for col in range(X.shape[1]):
    valid_values = X[:, col][mask[:, col]]  # Extract non -1 values
    if valid_values.size > 0:  # Ensure there's data to scale
        X_scaled[:, col][mask[:, col]] = scaler.fit_transform(valid_values.reshape(-1, 1)).flatten()

# One-hot encode the labels
ohe = OneHotEncoder()
y_encoded = ohe.fit_transform(y.reshape(-1, 1)).toarray()

# Split dataset into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X_scaled, y_encoded, test_size=0.2, random_state=42)

# Convert training data into TensorFlow datasets for better performance
batch_size = min(32, len(X_train))  # Adjust batch size based on dataset size
train_dataset = tf.data.Dataset.from_tensor_slices((X_train, y_train)).batch(batch_size)
test_dataset = tf.data.Dataset.from_tensor_slices((X_test, y_test)).batch(batch_size)

# Define the neural network model with input layer
model = Sequential([
    Input(shape=(X_train.shape[1],)),  # Best practice instead of input_dim
    Dense(128, activation="relu", kernel_regularizer=l2(0.001)),  # L2 regularization to prevent overfitting
    Dense(64, activation="relu", kernel_regularizer=l2(0.001)),
    Dense(len(ohe.categories_[0]), activation="softmax")  # Output neurons match number of unique users
])

# Compile the model
model.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])

# Implement Early Stopping to prevent unnecessary overfitting
early_stopping = EarlyStopping(monitor="val_loss", patience=10, restore_best_weights=True)

# Train the model with early stopping
history = model.fit(train_dataset, epochs=100, validation_data=test_dataset, callbacks=[early_stopping])

# Plot accuracy over epochs
plt.plot(history.history["accuracy"], label="Train Accuracy")
plt.plot(history.history["val_accuracy"], label="Validation Accuracy")
plt.title("Model Accuracy Over Epochs")
plt.xlabel("Epoch")
plt.ylabel("Accuracy")
plt.legend()
plt.show()

# Save the model in the new Keras format
model.save("keystroke_nn_classifier.keras")

print("Training complete. Model saved as 'keystroke_nn_classifier.keras'.")
