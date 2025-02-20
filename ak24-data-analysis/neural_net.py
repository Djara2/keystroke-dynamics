import os
import pandas as pd
import numpy as np
import keras
import tensorflow as tf
from tensorflow.keras.models import Sequential # type: ignore
from tensorflow.keras.layers import Dense, Input # type: ignore
from tensorflow.keras.regularizers import l2 # type: ignore
import matplotlib.pyplot as plt
from sklearn.preprocessing import OneHotEncoder, StandardScaler
from sklearn.model_selection import train_test_split
from tensorflow.keras.callbacks import EarlyStopping # type: ignore

# Print TensorFlow version
print(f"Using TensorFlow version: {tf.__version__}")

def run_neural_net(X_scaled, y):
    # One-hot encode the labels using OneHotEncoder
    ohe = OneHotEncoder()
    y_encoded = ohe.fit_transform(y.values.reshape(-1, 1)).toarray()  # One-hot encode labels

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

    return history

def main():
    # Load dataset
    dataset = load_data()

    # Process dataset to get features(X) and labels(y)
    X, y = process_dataset(dataset)

    # Standarize the features(X) and lables(y) data
    X_scaled, y_encoded = standardize_data(X, y)

    # Run the neureal network with the standarized data
    history = run_neural_net(X_scaled, y_encoded)

    # Plot accuracy over epochs
    plt.plot(history.history["accuracy"], label="Train Accuracy")
    plt.plot(history.history["val_accuracy"], label="Validation Accuracy")
    plt.title("Model Accuracy Over Epochs")
    plt.xlabel("Epoch")
    plt.ylabel("Accuracy")
    plt.legend()
    plt.show()

    # Save the model in the new Keras format
    #model.save("keystroke_nn_classifier.keras")

    print("Training complete. Model saved as 'keystroke_nn_classifier.keras'.")

if __name__ == '__main__':
    main()