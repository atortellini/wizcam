## ISSUE: Might want to consider writing the instructions out into a file that can be read in to set the orig_instr member of each instruction_t struct
- Benefit: If the program were to crash/close freecam when the patch was applied, could still return to a state of unpatched camera

- Alternative Solution: On interrupt sig the original instructions are first written back

## ISSUE: Speed of cam movement is much too fast at 5.
### Observations:
- Tried setting it to 0.1/.1/0.1f and it was giving me truncated from double to float warnings and wasn't working

- Maybe because 0.1 is more precise so it was defaulting to being a double, but then getting truncated to a float since all my arguemnts are floats

- Need to think about how I can slow the movement speed of the camera then since coords of cam in game's memory are floats

- x/y is treated as a double but they're combined so it's essentially two floats

- Or maybe something more is going on and me treating it like two floats is why the movement seems unpredictable

- Maybe can do an experiment where I just monitor the camera values in cheat engine as they are naturally updated while I run around in wizard101

- 0.5 had no warnings when compiling and worked, but was still too fast

## ISSUE: The free camera is only synced from the game once at the beginning of the program
### Observation:
- If you get lost with the freecam essentially screwed no way to reset the freecam back to your coords since its 'simulated' separately
### Solution:
- Maybe consider syncing 'simulated' freecam camera back to the game either just after toggling off freecam or just before toggling on freecam

### Alternative Solution:
- Or, maybe, have a reset keybind to reset the camera back to your games camera while in freecam mode.

- This would mean I would have to keep a snapshot of the last game Camera state before toggling freecam

## ISSUE: Movement of the camera feels very unintuitive
### Observations:
- The movement in x/y direction seems very unpredictable, at least from the little testing I was able to do

- z direction worked as expected
### Ideal:
- Get the camera to a point where the forward vector is calculated and movement is based on that vector for the most intuitive experience