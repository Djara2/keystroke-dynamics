import os
import struct

# Reads the binary file data output from the keystroke logger
def read_keystroke_logger_output(file_path):
    print(f"Reading {file_path}")

    # Create variable to store the sessions data and user info
    sessions_data = []
    user_info = {}

    # Open file and begin reading
    with open(file_path, "rb") as file:
        # Read user_info fields (user, email, major, typing_duration)
        user_info['user'] = struct.unpack('64s', file.read(64))[0].decode('utf-8').strip('\x00')
        user_info['email'] = struct.unpack('64s', file.read(64))[0].decode('utf-8').strip('\x00')
        user_info['major'] = struct.unpack('64s', file.read(64))[0].decode('utf-8').strip('\x00')
        user_info['typing_duration'] = struct.unpack('h', file.read(2))[0]  # short (2 bytes)

        # Read number of sessions (8 bytes)
        session_count = struct.unpack("Q", file.read(8))[0]

        # Loop through the different sessions in the file
        for _ in range(session_count):
            # Create temp variable to store information in the session
            session = {}

            # Read keystroke count and create array variable to store all keystokes data
            keystrokes_length = struct.unpack("Q", file.read(8))[0]
            keystrokes = []

            # Read each keystroke entry
            for _ in range(keystrokes_length):
                key = struct.unpack("c", file.read(1))[0]  # [1 byte] (c for a single character)
                press_time_sec = struct.unpack("q", file.read(8))[0]  # [8 bytes] (q for long long integer)
                press_time_nsec = struct.unpack("q", file.read(8))[0]  # [8 bytes] (q for long long integer)
                release_time_sec = struct.unpack("q", file.read(8))[0]  # [8 bytes] (q for long long integer)
                release_time_nsec = struct.unpack("q", file.read(8))[0]  # [8 bytes] (q for long long integer)

                # Add the keystroke data to the keystrokes array
                keystrokes.append({
                    "key": key,
                    "press_time_tv_sec": press_time_sec,
                    "press_time_tv_nsec": press_time_nsec,
                    "release_time_tv_sec": release_time_sec,
                    "release_time_tv_nsec": release_time_nsec
                })

            # Save the keystrokes array to the current session's data
            session["keystrokes"] = keystrokes

            # Read time deltas and save time_deltas array to current session's data
            time_deltas_length = struct.unpack("Q", file.read(8))[0]  # [8 bytes] (Q for unsigned long long integer)
            session["time_deltas"] = list(struct.unpack(f"{time_deltas_length}Q", file.read(time_deltas_length * 8))) if time_deltas_length > 0 else []

            # Read dwell times and save dwell_times array to current session's data
            dwell_times_length = struct.unpack("Q", file.read(8))[0]  # [8 bytes] (Q for unsigned long long integer)
            session["dwell_times"] = list(struct.unpack(f"{dwell_times_length}Q", file.read(dwell_times_length * 8))) if dwell_times_length > 0 else []

            # Read flight times and save flight_times array to current session's data
            flight_times_length = struct.unpack("Q", file.read(8))[0]  # [8 bytes] (Q for unsigned long long integer)
            session["flight_times"] = list(struct.unpack(f"{flight_times_length}Q", file.read(flight_times_length * 8))) if flight_times_length > 0 else []

            # Save the current session to the list of all sessions in the file
            sessions_data.append(session)

    # Return user_info and the session data
    return user_info, sessions_data

    

if __name__ == '__main__':
    file_path = "../infoTest.bin"
    user_info, sessions_data = read_keystroke_logger_output(file_path)

    print(f"User Info: {user_info}")

    for i, session in enumerate(sessions_data):
        print(f"Session {i+1}: User: NOT IMPLEMENTED YET")
        print(f"    Keystrokes: {session['keystrokes'][:2]}")
        print(f"    Time Deltas: {session['time_deltas'][:2]}")
        print(f"    Dwell Times: {session['dwell_times'][:2]}")
        print(f"    Flight TImes: {session['flight_times'][:2]}")
