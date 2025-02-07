from enum import Enum

class CombinationType(Enum):
    STRING = 0
    NUMBER = 1

def get_combinations(content: str, combination_length: int, combination_type) -> list: 
    if (content == None):
        print("[get_combinations] Provided text cannot be empty or NULL.")
        return []
     
    content_length: int = len(content)
    if (content_length < combination_length):
        print("[get_combinations] Cannot get length-{} combinations of a string that is only {} characters long. \"{}\" is too short.".format(combination_length, content_length, text))
        return []
    
    if (combination_length <= 0):
        print("[get_combinations] Cannot get zero or negative-length combinations.")
        return []

    if (combination_length == 1):
        return [x for x in content]

    # No preliminary errors - main code
    match(combination_type):
        # String (in Python, this is different from a list of characters)
        case CombinationType.STRING:
            # If the text is as long as the combination length, then the only combination
            # is the text itself. 
            if (content_length == combination_length):
                return [content]

            if (combination_length == 2):
                combinations = ["{}{}".format(content[x], content[x+1]) for x in range(0, content_length) if x < content_length - 1]

            elif (combination_length == 3):
                combinations = ["{}{}{}".format(content[x], content[x+1], content[x+2]) for x in range(0, content_length) if x < content_length - 2]

            else:
                print("[get_combinations] Length-{} combinations not yet implemented. Highest length is 3.".format(combination_length))
                combinations = []

        # Number (list of ints, floats, whatever)
        case CombinationType.NUMBER:
            if (content_length == combination_length):
                return content
            
            if (combination_length == 2):
                combinations = [
                                    [ content[x], content[x+1] ]
                                    for x in range(0, content_length)
                                    if x < content_length - 1 
                               ]
            
            elif (combination_length == 3):
                combinations = [
                                    [ content[x], content[x+1], content[x+2] ]
                                    for x in range(0, content_length)
                                    if x < content_length - 2 
                               ]

        case _:
            print("[get_combinations] CombinationType \"{}\" is invalid.".format(combination_type))
            return []
 
    return combinations

# Sessions is an array of dictionaries
# Sessions["keystrokes"] -> { "key": key,
#                             "press_time_tv_sec": press_time_sec,
#                             "press_time_tv_nsec": press_time_nsec,
#                             "release_time_tv_sec": release_time_sec,
#                             "release_time_tv_nsec": release_time_nsec
#                            }
# Sessions["time_deltas"]  -> list of time deltas
# Sessions["dwell_times"]  -> list of dwell times
# Sessions["flight_times"] -> list of flight times
def create_grapheme_map(sessions: list) -> dict:
    # Rotated 90 degrees clockwise, it's a table
    grapheme_map: dict = { "#"           : [],
                           "grapheme"    : [],
                           "time delta"  : [],
                           "dwell time"  : [],
                           "flight time" : []
                         }

    # "session" is a dictionary 
    for session in sessions: 
        #  ...
        pass        
    # end of function 
