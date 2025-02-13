#!/usr/bin/env python3

from random import randint

prompts_fh = open("res/prompts.txt", "r")
if (prompts_fh == None):
    print("There was a problem opening res/prompts.txt. Does the file exist?")
    exit()

prompts: list = prompts_fh.readlines()
prompts_fh.close()

prompts_length: int = len(prompts)
keep_playing: bool = True
user_input: str = ""
random_index: int = 0
random_prompt: str = ""
QUIT_KEYWORDS = {"q", "Q", "quit", "Quit", "QUIT"}

print("TIP: Enter \"quit\" to stop getting prompts.")
while (keep_playing):
    random_index = randint(0, prompts_length - 1)
    random_prompt = prompts[random_index]
    print("YOUR PROMPT: {}".format(random_prompt))
    print("Press enter for your next prompt... ", end = "")

    # Keep going based on user's consent
    user_input = input()
    if (user_input in QUIT_KEYWORDS):
        print("Program will terminate. Goodbye <3")
        exit() 
