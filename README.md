## ISSUE: If the program were to crash, or you close freecam when the patch was applied no way for camera to return to original state

### Possible Solution:
- Keep the overwritten instructions in a file that will be read to set the orig_instr member of each instruction_t struct

### Possible Solution:
- On interrupt sig the original instructions are first written back

## ISSUE: Speed of cam movement is much too fast at 5.
### Observations:
- Tried setting it to 0.1/.1/0.1f and it was giving me truncated from double to float warnings and wasn't working

- Maybe because 0.1 is more precise so it was defaulting to being a double, but then getting truncated to a float since all my arguemnts are floats

- Need to think about how I can slow the movement speed of the camera then since coords of cam in game's memory are floats

- x/y is treated as a double but they're combined so it's essentially two floats

- Or maybe something more is going on and me treating it like two floats is why the movement seems unpredictable

- Maybe can do an experiment where I just monitor the camera values in cheat engine as they are naturally updated while I run around in wizard101

- 0.5 had no warnings when compiling and worked, but was still too fast

### Possible Solution:
- Had an epiphany while sleeping that maybe the reason the camera speed seems to be too fast is due to how quickly the input handler thread is updating the local camera.
- There was no delay/debouncing when handling user keystrokes so way too many updates are likely occurring simply from one button press.
- Added a 250ms debounce that the input thread will sleep for after each iteration of checking for keystates.

## ISSUE: Since the free camera is only synced from the game at the beginning of the program, if you get lost in freecam mode no way to return back

### Possible Solution:
- Maybe consider syncing 'simulated' freecam camera back to the game either just after toggling off freecam or just before toggling on freecam

### Possible Solution:
- Have a reset keybind to reset the camera back to your games camera while in freecam mode.

- A snapshot of the last game Camera state needs to be taken before toggling freecam

## ISSUE: Movement of the camera feels very unintuitive
### Observations:
- The movement in x/y direction seems very unpredictable, at least from the little testing I was able to do

- z direction worked as expected
### Ideal:
- Get the camera to a point where the forward vector is calculated and movement is based on that vector for the most intuitive experience


## ISSUE: My input handling is pretty shitty

### Observations:
- When holding down the toggle freecam hotkey it switches quickly back and forth between patching/unpatching
- Ideally would do nothing if the button is held