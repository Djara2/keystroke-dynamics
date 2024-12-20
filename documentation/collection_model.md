# Table of Contents

[toc]

# Data Collected

This is a **free-text system**, as it is the focus of our research. Likewise, we are only collecting the symbols hit on the keyboard over the duration of the typing exam(s) and the time intervals between those keypresses. 

We then form the overall string typed (including backspaces), and iterate over it in size-2 chunks to determine phonemes. We then derive a record of the time intervals elapsed for individual phonemes, between phonemes, and the time between the individual characters that make the phoneme. The phonemes considered for any one run are not fixed or known ahead-of-time. They are derived according to what the user types for that session.

# Workflow

1. Until the number of sessions is completed...
	1. Run the typing exam.
	2. Store the data for that session in their relevant buffers.
	3. Move all of that data into a nearest, unused `session` struct within the `sessions` buffer. 
	4. Clear out per-session buffers for next run.
	5. Prompt user if they are ready to take the next exam.
2. Construct a hashmap which maps phonemes to an array of time deltas.
3. For each session in the `sessions` buffer...
	1. For each size-2 substring (phoneme) in the overall string typed by the user...
		1. Add the substring (phoneme) as a key of the hashmap. 
		2. Add the related time delta to the array of time deltas pointed to by the phoneme (key). 
4. Write the contents of the hashmap to the specified output file as a CSV.
	1. For each key (phoneme) in the hashmap...
		1. Write the phoneme as a column of the CSV.
	2. Define an iterator variable `i`.
	3. While NO time arrays have a length smaller than `i`...
		1. Set a boolean `i_smaller` flag to false.
		2. Set an integer variable `i_smaller_count` to 0.
		3. For each key (phoneme) in the hashmap...
			1. If the length of the time delta array is larger than `i`, set `i_smaller` to true AND increment `i_smaller_count`. Otherwise, set `i_smaller` to false.
			2. IF `i_smaller` IS FALSE, skip the current key (phoneme) and move to the next. OTHERWISE write `i`th time delta for the current key in the CSV.
		4. IF `i_smaller_count` IS 0, stop looping. 
5. All data is now written to the CSV file and it may be used for analysis.
6. Program terminates.

