# pylibgrapheme Documentation

**Table of Contents:**

1. [Getting Contiguous Graphemes or Substrings from an Input String](#getting-contiguous-graphemes-or-substrings-from-an-input-string)
2. [Getting Delta Times or Dwell Times or Flight Times for Graphemes or Substrings](#getting-delta-times-or-dwell-times-or-flight-times-for-graphemes-or-substrings)

# Getting Contiguous Graphemes or Substrings from an Input String

The relevant function for this goal is `get_combinations(content, length, type)` where...
- `content` is either a string or a list of numbers (ints/floats)
- `length` is the length of the combinations to be made
- `type` is a value of the `CombinationType` enum. It should be `STRING` when the `content` parameter is a string and `NUMBER` when the `content` parameter is a list of ints or floats.
    - As per Python enumerations, `STRING` and `NUMBER` must be prefixed with `CombinationType` (e.g. `CombinationType.NUMBER`)

This function returns an empty list when the arguments are inappropriate.

One may wish to get every length-2 contiguous grapheme/substring of the text "Hello, world!". To do this, they can execute the following code:

```python
from pylibgrapheme import *

combinations = get_combinations("Hello, world!", 2, CombinationType.STRING)
```

# Getting Delta Times or Dwell Times or Flight Times for Graphemes or Substrings

As like in [Getting Contiguous Graphemes or Substrings from an Input String](#getting-contiguous-graphemes-or-substrings-from-an-input-string), the relevant function for this is also `get_combinations(content, length, type)`, where...
- `content` is either a string or a list of numbers (ints/floats)
- `length` is the length of the combinations to be made
- `type` is a value of the `CombinationType` enum. It should be `STRING` when the `content` parameter is a string and `NUMBER` when the `content` parameter is a list of ints or floats.
    - As per Python enumerations, `STRING` and `NUMBER` must be prefixed with `CombinationType` (e.g. `CombinationType.NUMBER`)

```python
from pylibgrapheme import *

combinations = get_combinations([1, 2, 3, 4], 2, CombinationType.NUMBER)
```

On its own, the function is not handy for the purposes of finding the time delta/dwell time/flight time for a grapheme. A single number is desired to associate with the grapheme for the time delta/dwell time/flight time. To reduce the combination into just one number for a grapheme, we can take the average of the combination:

```python
from pylibgrapheme import *

combinations = get_combinations([1, 2, 3, 4], 2, CombinationType.NUMBER)
dwell_times = [sum(combination) / len(combination) for combination in combinations]
```
