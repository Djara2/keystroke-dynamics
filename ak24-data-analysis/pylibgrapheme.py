from enum import IntEnum 

class CombinationType(IntEnum):
    TEXT = 0
    NUMBER = 1

class GraphemeType(IntEnum):
    UNIGRAPH = 0
    DIGRAPH = 1
    TRIGRAPH = 2

class StatisticType(IntEnum):
    TIME_DELTA = 0
    DWELL_TIME = 1
    FLIGHT_TIME = 2

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

# Session is a dictionary
# Session["keystrokes"] -> { "key": key,
#                             "press_time_tv_sec": press_time_sec,
#                             "press_time_tv_nsec": press_time_nsec,
#                             "release_time_tv_sec": release_time_sec,
#                             "release_time_tv_nsec": release_time_nsec
#                            }
# Session["time_deltas"]  -> list of time deltas
# Session["dwell_times"]  -> list of dwell times
# Session["flight_times"] -> list of flight times
def create_grapheme_map(session: dict, grapheme_type) -> dict:
    # Rotated 90 degrees clockwise, it's a table
    grapheme_map: dict = { "#"           : [],
                           "grapheme"    : [],
                           "time delta"  : [],
                           "dwell time"  : [],
                           "flight time" : []
                         }

    match(grapheme_type):
            case GraphemeType.DIGRAPH:
                combination_length = 2

            case GraphemeType.TRIGRAPH:
                combination_length = 3

            case _:
                print("[create_grapheme_map] GraphemeType \"{}\" is not supported for this function. Please use GraphemeType.DIGRAPH or GraphemeType.TRIGRAPH.".format(grapheme_type))
                return None

    # Note: "session" is a dictionary 
    # (1) Get the graphemes for the graphemes column
    columns = ["time delta", "dwell time", "flight time"]
    graphemes: list = get_combinations(session["keystrokes"], combination_length, CombinationType.TEXT)
    if (graphemes == []):
        print("[create_grapheme_map] Empty list returned for graphemes of length {} while processing.".format(combination_length))
        return None

    # (2) Get the time delta, dwell time, and flight time columns 
    session_key = ""
    keys_of_interest = ["time_deltas", "dwell_times", "flight_times"]
    statistic_lists = [ list() for x in len(StatisticType) ]
    # (2.1) The logic is the same, just we access a different set of values from the session hashmap.
    #       So, do the same activity (get_combinations, then averages), but change the statistic. 
    for statistic_type in StatisticType:
        session_key = keys_of_interest[statistic_type]

        # Get combinations for statistic list ("time deltas" is a statistic list)
        statistic_lists[statistic_type] = get_combinations(session[session_key], combination_length, CombinationType.NUMBER)
        if ( statistic_lists[statistic_type] == [] ):
            print("[create_grapheme_map] Empty list returned for StatisticType {} graphemes of length {} while processing.".format(statistic_type, combination_length))
            return None
        
        # Get averages so a grapheme is "one unit" and any value in a statistic list is "one unit"
        # E.g. "hi," has time delta avg(x, y, z) instead of delta(x, y) and delta(y, z)
        statistic_lists[statistic_type] = [ 
                                            sum(grouping) / combination_length 
                                            for grouping in statistic_lists[statistic_type]
                                          ]
      
    grapheme_counter = 1
    for grapheme in graphemes:
        grapheme_map["#"].append(grapheme_counter)
        grapheme_map["grapheme"].append(grapheme)
        grapheme_map["time_delta"].appennd
        grapheme_counter += 1
    # end of function 
