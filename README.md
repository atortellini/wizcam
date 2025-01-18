# Current Issues and Notes

## ISSUE: The character continues to move in freecam mode
### Observations:
- All keyboard events are still passed to the game's process while in freecam mode.
- The game continues processing inputs, causing the character to move.
- Modifying how the game handles input seems too complicated.

---

## ISSUE: Camera state isn't restored if the program crashes or freecam is closed while patched
### Possible Solutions:
1. Keep the overwritten instructions in a file. This file will be read to set the `orig_instr` member of each `instruction_t` struct.
2. Use an interrupt signal (`SIGINT`) to ensure the original instructions are written back before the program exits.

---

## ISSUE: Free camera only syncs with the game at startup
### Notes:
- If you get lost in freecam mode, there's no way to return to the game's camera position.
- This issue also occurs when changing zones.

### Possible Solutions:
1. Sync the freecam camera back to the game:
   - Just after toggling freecam off.
   - Just before toggling freecam on.
2. Add a reset keybind to restore the freecam to the game's current camera position.
   - Take a snapshot of the last game camera state before toggling freecam.

---

## ISSUE: Camera movement feels unintuitive
### Observations:
- Movement along the x, y, and z axes is predictable.
- Inability to change yaw or pitch makes movement feel unintuitive.
- The freecam doesn't sync from the game's values beyond the initial setup, making yaw/pitch static.

### Ideal Solution:
- Calculate a forward vector for the camera and base movement on it for more intuitive controls.
- Capture mouse input to adjust pitch and yaw dynamically.

---

## ISSUE: Input handling is poor
### Observations:
- Holding down the toggle freecam hotkey causes rapid toggling between patching and unpatching.
- Ideally, holding the button would have no effect after the first toggle.

---

# Past Issues

<details>
<summary>ISSUE: Camera movement speed was too fast at 5.</summary>

- The high speed was due to the input handler thread processing inputs too quickly.
- Adding a debounce to the input thread (33ms delay) highlighted that a movement speed of `5.0` was actually slow.
</details>
