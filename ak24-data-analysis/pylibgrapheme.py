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

STATISTIC_TYPE_LENGTH = 3

class LibGraphemeError(IntEnum):
    ERROR_NONE = 0

    ERROR_COMBINATION_LENGTH_NON_POSITIVE = 1
    ERROR_COMBINATION_LENGTH_TOO_LARGE    = 2
    ERROR_COMBINATION_LENGTH_UNHANDLED    = 3

    ERROR_EMPTY_CONTENT       = 4
    ERROR_EMPTY_GRAPHEME_LIST = 5
    ERROR_EMPTY_STATISTIC_LIST = 6

    ERROR_INVALID_COMBINATION_TYPE = 7
    ERROR_INVALID_GRAPHEME_TYPE    = 8

# returns tuple (x, y) where x is a LibGraphemeError code and y is the return value
def get_combinations(content: str, combination_length: int, combination_type) -> list: 
    if (content == None):
        print("[get_combinations] Provided text cannot be empty or NULL.")
        return (LibGraphemeError.ERROR_EMPTY_CONTENT, [])
     
    content_length: int = len(content)
    if (content_length < combination_length):
        print("[get_combinations] Cannot get length-{} combinations of a string that is only {} characters long. \"{}\" is too short.".format(combination_length, content_length, text))
        return (LibGraphemeError.ERROR_COMBINATION_LENGTH_TOO_LARGE, [])
    
    if (combination_length <= 0):
        print("[get_combinations] Cannot get zero or negative-length combinations.")
        return (LibGraphemeError.ERROR_COMBINATION_LENGTH_NON_POSITIVE, [])

    if (combination_length == 1):
        return (LibGraphemeError.ERROR_NONE, [x for x in content])

    # No preliminary errors - main code
    error_code = LibGraphemeError.ERROR_NONE
    match(combination_type):
        # String (in Python, this is different from a list of characters)
        case CombinationType.TEXT:
            # If the text is as long as the combination length, then the only combination
            # is the text itself. 
            if (content_length == combination_length):
                return (LibGraphemeError.ERROR_NONE, [content])

            if (combination_length == 2):
                error_code = LibGraphemeError.ERROR_NONE
                combinations = ["{}{}".format(content[x], content[x+1]) for x in range(0, content_length) if x < content_length - 1]

            elif (combination_length == 3):
                error_code = LibGraphemeError.ERROR_NONE
                combinations = ["{}{}{}".format(content[x], content[x+1], content[x+2]) for x in range(0, content_length) if x < content_length - 2]

            else:
                print("[get_combinations] Length-{} combinations not yet implemented. Highest length is 3.".format(combination_length))
                combinations = []
                error_code = LibGraphemeError.ERROR_COMBINATION_LENGTH_UNHANDLED

        # Number (list of ints, floats, whatever)
        case CombinationType.NUMBER:
            if (content_length == combination_length):
                return content
            
            if (combination_length == 2):
                error_code = LibGraphemeError.ERROR_NONE
                combinations = [
                                    [ content[x], content[x+1] ]
                                    for x in range(0, content_length)
                                    if x < content_length - 1 
                               ]

            
            elif (combination_length == 3):
                error_code = LibGraphemeError.ERROR_NONE
                combinations = [
                                    [ content[x], content[x+1], content[x+2] ]
                                    for x in range(0, content_length)
                                    if x < content_length - 2 
                               ]

        case _:
            print("[get_combinations] CombinationType \"{}\" is invalid.".format(combination_type))
            combinations = []
            error_code = LibGraphemeError.ERROR_INVALID_COMBINATION_TYPE
     
    return (error_code, combinations)

# (1) Session is a dictionary
#     Session["keystrokes"] -> { "key": key,
#                                 "press_time_tv_sec": press_time_sec,
#                                 "press_time_tv_nsec": press_time_nsec,
#                                 "release_time_tv_sec": release_time_sec,
#                                 "release_time_tv_nsec": release_time_nsec
#                                }
#     Session["time_deltas"]  -> list of time deltas
#     Session["dwell_times"]  -> list of dwell times
#     Session["flight_times"] -> list of flight times
#
# (2) Returns (x, y) where x is a LibGraphemeError and y is a dictionary (table)
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
                return (LibGraphemeError.ERROR_INVALID_GRAPHEME_TYPE, None)

    # Note: "session" is a dictionary 
    # (1) Get the graphemes for the graphemes column
    columns = ["time delta", "dwell time", "flight time"]
    
    keystroke_keys = [ session["keystrokes"][x]["key"] for x in range(len(session["keystrokes"])) ] 
    original_text = "".join(keystroke_keys)
    (error_code, graphemes) = get_combinations(original_text, combination_length, CombinationType.TEXT)

    if (graphemes == []):
        print("[create_grapheme_map] Empty list returned for graphemes of length {} while processing.".format(combination_length))
        return (LibGraphemeError.ERROR_EMPTY_GRAPHEME_LIST, None)



    # (2) Get the time delta, dwell time, and flight time columns 
    session_key = ""
    keys_of_interest = ["time_deltas", "dwell_times", "flight_times"]
    statistic_lists = [ list() for x in range(len(StatisticType)) ]

    # (2.1) The logic is the same, just we access a different set of values from the session hashmap.
    #       So, do the same activity (get_combinations, then averages), but change the statistic. 
    for statistic_type in StatisticType:
        session_key = keys_of_interest[statistic_type]

        # (2.2) Get combinations for statistic list ("time deltas" is a statistic list)
        (error_code, statistic_lists[statistic_type]) = get_combinations(session[session_key], combination_length, CombinationType.NUMBER)
        if ( statistic_lists[statistic_type] == [] ):
            print("[create_grapheme_map] Empty list returned for StatisticType {} graphemes of length {} while processing.".format(statistic_type, combination_length))

            return (LibGraphemeError.ERROR_EMPTY_STATISTIC_LIST, None)
       

        # (2.3) Get averages so a grapheme is "one unit" and any value in a statistic list is "one unit"
        #       E.g. "hi," has time delta avg(x, y, z) instead of delta(x, y) and delta(y, z)
        statistic_lists[statistic_type] = [ 
                                            sum(grouping) / combination_length 
                                            for grouping in statistic_lists[statistic_type]
                                          ]

    # (3) Add the statistic lists as columns
    time_delta_list  = statistic_lists[StatisticType.TIME_DELTA]
    dwell_time_list  = statistic_lists[StatisticType.DWELL_TIME]
    flight_time_list = statistic_lists[StatisticType.FLIGHT_TIME]
    graphemes_len = len(dwell_time_list) 
    grapheme_map["#"] = list(range(1, graphemes_len + 1))
    grapheme_map["grapheme"] = graphemes
    grapheme_map["time_delta"] = time_delta_list
    grapheme_map["dwell_time"] = dwell_time_list
    grapheme_map["flight_time"] = flight_time_list
    
    return (LibGraphemeError.ERROR_NONE, grapheme_map)
    # end of function 
