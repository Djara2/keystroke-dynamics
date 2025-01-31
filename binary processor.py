# phase 1, parse binary data
# phase 2, put it into dictionary

import struct

import struct

def read_keystroke_logger(filename):
    with open(filename, 'rb') as f:
        # Read the number of sessions (8 bytes, unsigned long long)
        num_sessions = struct.unpack('Q', f.read(8))[0]
        print(f'Number of sessions: {num_sessions}')
        
        sessions = []
        
        for s in range(num_sessions):
            print(f'\nSession {s+1}:')
            
            # Read the number of keystrokes (8 bytes, unsigned long long)
            num_keystrokes = struct.unpack('Q', f.read(8))[0]
            print(f'  Number of keystrokes: {num_keystrokes}')
            
            keystrokes = []
            
            for _ in range(num_keystrokes):
                key = struct.unpack('B', f.read(1))[0]  # 1 byte for key value
                press_time = struct.unpack('Q', f.read(8))[0]  # 8 bytes for press timestamp
                release_time = struct.unpack('Q', f.read(8))[0]  # 8 bytes for release timestamp
                keystrokes.append((key, press_time, release_time))
                print(f'    Key: {key}, Press Time: {press_time}, Release Time: {release_time}')
            
            # Read delta array
            delta_length = struct.unpack('Q', f.read(8))[0]  # 8 bytes for delta array length
            delta_capacity = struct.unpack('Q', f.read(8))[0]  # 8 bytes for delta array capacity (ignored)
            print(f'  Delta array length: {delta_length}')
            
            deltas = []
            for _ in range(delta_length):
                delta_value = struct.unpack('Q', f.read(8))[0]  # 8 bytes for each delta value
                deltas.append(delta_value)
                
            print(f'  Delta values: {deltas}')
            
            sessions.append({
                'keystrokes': keystrokes,
                'deltas': deltas
            })
            
    return sessions

def generate_keystroke_hashmap(sessions):
    hashmaps = []
    for session in sessions:
        keystrokes = session['keystrokes']
        hashmap = {}
        for i in range(len(keystrokes) - 1):
            key1, press1, release1 = keystrokes[i]
            key2, press2, release2 = keystrokes[i + 1]
            
            key_pair = (key1, key2)
            hashmap[key_pair] = {
                'time_between_press': press2 - press1,
                'time_between_release': release2 - release1,
                'time_between_release_press': press2 - release1,
                'time_between_press_release': release2 - press1
            }
        hashmaps.append(hashmap)
    return hashmaps

# Example usage:
# sessions_data = read_keystroke_logger('keystroke_data.bin')
# keystroke_hashmaps = generate_keystroke_hashmap(sessions_data)
