# phase 1, parse binary data
# phase 2, put it into dictionary

from sys import argv
import struct
import json
import os

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

            deltas = []
            for _ in range(delta_length):
                if f.tell() + 8 > file_size:
                    raise ValueError("Unexpected end of file while reading delta values.")

                delta_value = struct.unpack('Q', f.read(8))[0] / 1_000_000  # Convert ns → ms
                deltas.append(delta_value)

            print(f'\tDelta values (ms): {deltas}')

            # Read dwell array (stored in nanoseconds → convert to milliseconds)
            if f.tell() + 8 > file_size:
                raise ValueError("Unexpected end of file while reading dwell count.")

            dwell_length = struct.unpack('Q', f.read(8))[0]
            print(f'\tDwell array length: {dwell_length}')

            dwells = []
            for _ in range(dwell_length):
                if f.tell() + 8 > file_size:
                    raise ValueError("Unexpected end of file while reading dwell values.")

                dwell_value = struct.unpack('Q', f.read(8))[0] / 1_000_000  # Convert ns → ms
                dwells.append(dwell_value)

            print(f'\tDwell values (ms): {dwells}')

            # Read flight array (stored in nanoseconds → convert to milliseconds)
            if f.tell() + 8 > file_size:
                raise ValueError("Unexpected end of file while reading flight count.")

            flight_length = struct.unpack('Q', f.read(8))[0]
            print(f'\tFlight array length: {flight_length}')

            flights = []
            for _ in range(flight_length):
                if f.tell() + 8 > file_size:
                    raise ValueError("Unexpected end of file while reading flight values.")

                flight_value = struct.unpack('Q', f.read(8))[0] / 1_000_000  # Convert ns → ms
                flights.append(flight_value)

            print(f'\tFlight values (ms): {flights}')

            sessions.append({
                'keystrokes': keystrokes,
                'deltas': deltas,
                'dwells': dwells,
                'flights': flights
            })
    
    return sessions


if __name__ == "__main__":
    input_file = "keystroke_log_test.bin"

    try:
        sessions = read_keystroke_logger(input_file)
        print("\nProcessing Complete!")

    except Exception as e:
        print(f"[Error] {e}")



# if __name__ == "__main__":
#     # -i or --input  everything after is filepath(s)
#     # -o or --output end, exactly 1 token after
#     # input_file = "kdt"
#     input_file = "keystroke_log_test.bin"
#     argc = len(argv)
#     if(argc < 3):
#         print_help()
#     if(argv[argc-2] != "-o"):
#         print("[Command Error] Must include output file as last argument.")
#         print_help()
#     for i in range(argc):
#         if(argv[i].lower() in ("-h", "--help")):
#             print_help()
#         elif(argv[i] in ("-i", "--input")):
#             try:
#                 input_file = argv[i+1]
#                 if(input_file == None):
#                     print("[Input Error] Provided file is empty.\n")
#                 else:
#                     session = read_keystroke_logger(input_file)
#                     hashmap = generate_keystroke_hashmap(session)
#             except Exception as e:
#                 print(f"[Input Error] The file provided produced an error: {e}")

#                 print_help()
#         elif(argv[i] in ("-o", "--output")):
#             try:
#                 output_file = argv[i+1]
#                 with open(output_file, "w") as out:
#                     # out.write(hashmap)
#                     json.dump(hashmap, out, indent=4)
#             except Exception as e:
#                 print(f"[Output Error] The file provided produced an error: {e}")
#         else:
#             pass
