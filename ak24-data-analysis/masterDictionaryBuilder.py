import pandas as pd
import csv

# Global variable to store the master dictionary
master_dictionary = {}

# ADD A USER COLUMN TO HANDLE FILTERING THE DATASET

# Function to find the longest values array in the dictionary
def find_max_values_array_length():
    # Access global variable
    global master_dictionary

    # Loop through dictionary
    max_length = -1
    for key, value in master_dictionary.items():
        value_length = len(value)
        
        # Update the max_length
        if value_length > max_length:
            max_length = value_length
    
    return max_length

# Print the master_dictionary
def print_master_dictionary():
    global master_dictionary

    print("\n\nMaster Dictionary:")
    for key, value in master_dictionary.items():
        print(f"{key}: {value}")

# Helper function to fill in master database with -1's
def fill_in_empty_values():
    global master_dictionary

    array_length_size = find_max_values_array_length()

    # Add -1's to value arrays that arent the same length as the longest
    for key, values in master_dictionary.items():
        while len(values) < array_length_size:
            values.append(-1)

# Function to update the master dictionary
def update_master_dictionary(combined_dictionary):
    global master_dictionary

    array_length_size = find_max_values_array_length()

    
    for key, value in combined_dictionary.items():
        # Key doesn't exist in master dictionary and not intializing dictionary
        if(key not in master_dictionary and array_length_size != 0):
            # Prepend/Pad -1's to the front, to match size of other value arrays
            master_dictionary[key] = [-1] * (array_length_size)
            # Add in the new dictionary key value pairs
            master_dictionary[key].append(value)
        # If intializing the master dictionary, just copy the dictionary over
        elif(array_length_size == 0):
            master_dictionary[key] = [value]
        # If key already exists in the dictionary, just append new value
        else:
            master_dictionary[key].append(value)

    # Call helper function to add -1's where needed
    fill_in_empty_values()

def create_combined_dictionary(grapheme_map):
    # Convert the dictionary to a Pandas DataFrame
    df = pd.DataFrame(grapheme_map)

    # Initialize an empty dictionary
    combined_dictionary = {}

    # Iterate over the rows and create the keys by combining 'grapheme' with the column names
    for idx, row in df.iterrows():
        grapheme = row['grapheme']
        for column in ['time_delta', 'dwell_time', 'flight_time']:
            key = f"{grapheme}+{column}"
            combined_dictionary[key] = row[column]

    return combined_dictionary

def write_to_csv(user_info):
    global master_dictionary
    # File path where you want to save the CSV
    csv_file_path = "master_dict_output.csv"

    # Define the custom delimiter
    delimiter = ','
    # Quote character to handle commas within values
    quotechar = '"' 

    # Write the data to a CSV file with the custom delimiter and quoting
    with open(csv_file_path, mode='w', newline='') as file:
        # Set paramaters for csv writer
        writer = csv.writer(file, delimiter=delimiter, quotechar=quotechar, quoting=csv.QUOTE_ALL)

        # Write the header (keys from master_dictionary)
        headers = list(master_dictionary.keys())
        headers = ["SequenceNumber", "User"] + headers
        writer.writerow(headers)

        # Write rows of the data
        rows = zip(*master_dictionary.values())
        for count, row in enumerate(rows, 1):
            row_with_user_count = [count, user_info["user"]] + list(row)
            writer.writerow(row_with_user_count)


    print(f"Data successfully written to {csv_file_path} with delimiter '{delimiter}' and quoting applied.")


if __name__ == '__main__':

    from read_binary import read_keystroke_logger_output
    from pylibgrapheme import create_grapheme_map, get_combinations, GraphemeType

    file_path = "./output/dataTest.bin"  # Path to your binary file
        
    # Read sessions from the binary file
    user_info, sessions_data = read_keystroke_logger_output(file_path)

    for i, session in enumerate(sessions_data):
        # Create grapheme map for each session
        grapheme_map_error_code, grapheme_map = create_grapheme_map(session, GraphemeType.DIGRAPH)

        grapheme_map["time_delta"].append(0)
        grapheme_map["flight_time"].append(0)

        if grapheme_map:
            combined_dictionary = create_combined_dict(grapheme_map)

            print(f"\n\n {combined_dictionary}")

            update_master_dict(combined_dictionary)

        session_count += 1


    # Improved print format for the master dictionary
    print("\n\nMaster Dictionary:")
    for key, value in master_dictionary.items():
        print(f"{key}: {value}")

    write_to_csv()