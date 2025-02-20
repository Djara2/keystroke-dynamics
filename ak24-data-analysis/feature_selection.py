from sklearn.feature_selection import SelectKBest, mutual_info_classif
import numpy as np

def feature_selection(X, y, k=200):

    # Use Mutual Information
    selector = SelectKBest(score_func=mutual_info_classif, k=min(k, X.shape[1])) 
    X_selected = selector.fit_transform(X, y)

    # Get selected feature indices
    selected_features = selector.get_support(indices=True)

    print(f"Selected {len(selected_features)} features (top {k} based on mutual info):")
    print(X.columns[selected_features])

    # Return the reduced dataset with selected features
    return X.iloc[:, selected_features], selected_features


# Select feature based on a threshold value
def feature_select_with_threshold(X, y, p_value_threshold=0.05):

    selector = SelectKBest(score_func=f_classif, k='all')  # We use 'all' to evaluate all features
    selector.fit(X, y)

    # Get the p-values for each feature
    p_values = selector.pvalues_

    # Select features where the p-value is lower than the threshold
    selected_features = np.where(p_values < p_value_threshold)[0]

    print(f"Selected features (with p-value < {p_value_threshold}):")
    print(X.columns[selected_features])

    # Return the reduced dataset with selected features
    X_selected = X.iloc[:, selected_features]
    
    return X_selected, selected_features

