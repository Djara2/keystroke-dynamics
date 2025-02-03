import struct
import json
import os
import numpy as np  # ✅ Using NumPy for efficient processing

def read_keystroke_logger(filename):
    with open(filename, 'rb') as f:
        file_size = os.path.getsize(filename)
        print(f"File size: {file_size} bytes\n")

        # Read the number of sessions (8 bytes, unsigned long long)
        if file_size < 8:
            raise ValueError("File too small to contain session count.")

        num_sessions = struct.unpack('Q', f.read(8))[0]
        print(f'Number of sessions: {num_sessions}')
        
        sessions = []
        
        for s in range(num_sessions):
            print(f'\nSession {s+1}: Reading at position {f.tell()}')

            if f.tell() + 8 > file_size:
                raise ValueError("Unexpected end of file while reading keystroke count.")

            num_keystrokes = struct.unpack('Q', f.read(8))[0]
            print(f'\tNumber of keystrokes: {num_keystrokes}')
            
            keystrokes = []
            
            for _ in range(num_keystrokes):
                if f.tell() + 17 > file_size:  # 1 (key) + 8 (press) + 8 (release)
                    raise ValueError("Unexpected end of file while reading keystroke data.")

                key = struct.unpack('B', f.read(1))[0]  # 1 byte for key value
                press_time = struct.unpack('d', f.read(8))[0]  # 8 bytes as double (seconds)
                release_time = struct.unpack('d', f.read(8))[0]  # 8 bytes as double (seconds)
                keystrokes.append((key, press_time, release_time))
                print(f'\t\tKey: {chr(key)}, Press Time: {press_time}, Release Time: {release_time}')
            
            # Read delta array (stored in nanoseconds → convert to milliseconds)
            if f.tell() + 8 > file_size:
                raise ValueError("Unexpected end of file while reading delta count.")

            delta_length = struct.unpack('Q', f.read(8))[0]
            print(f'\tDelta array length: {delta_length}')

            deltas = np.zeros(delta_length, dtype=np.int64)  # ✅ Use NumPy array for efficiency
            for i in range(delta_length):
                if f.tell() + 8 > file_size:
                    raise ValueError("Unexpected end of file while reading delta values.")

                deltas[i] = struct.unpack('Q', f.read(8))[0] // 1_000_000  # ✅ Convert ns → ms

            print(f'\tDelta values (ms): {deltas.tolist()}')

            # Read dwell array (stored in nanoseconds → convert to milliseconds)
            if f.tell() + 8 > file_size:
                raise ValueError("Unexpected end of file while reading dwell count.")

            dwell_length = struct.unpack('Q', f.read(8))[0]
            print(f'\tDwell array length: {dwell_length}')

            dwells = np.zeros(dwell_length, dtype=np.int64)  # ✅ NumPy for efficiency
            for i in range(dwell_length):
                if f.tell() + 8 > file_size:
                    raise ValueError("Unexpected end of file while reading dwell values.")

                dwells[i] = struct.unpack('Q', f.read(8))[0] // 1_000_000  # ✅ Convert ns → ms

            print(f'\tDwell values (ms): {dwells.tolist()}')

            # Read flight array (stored in nanoseconds → convert to milliseconds)
            if f.tell() + 8 > file_size:
                raise ValueError("Unexpected end of file while reading flight count.")

            flight_length = struct.unpack('Q', f.read(8))[0]
            print(f'\tFlight array length: {flight_length}')

            flights = np.zeros(flight_length, dtype=np.int64)  # ✅ NumPy for efficiency
            for i in range(flight_length):
                if f.tell() + 8 > file_size:
                    raise ValueError("Unexpected end of file while reading flight values.")

                flights[i] = struct.unpack('Q', f.read(8))[0] // 1_000_000  # ✅ Convert ns → ms

            print(f'\tFlight values (ms): {flights.tolist()}')

            sessions.append({
                'keystrokes': keystrokes,
                'deltas': deltas.tolist(),  # Convert NumPy array to a list for JSON output
                'dwells': dwells.tolist(),
                'flights': flights.tolist()
            })
    
    return sessions

if __name__ == "__main__":
    input_file = "keystroke_log_final.bin"

    try:
        sessions = read_keystroke_logger(input_file)
        print("\nProcessing Complete!")

    except Exception as e:
        print(f"[Error] {e}")
