
---

### Previous version resolution: ###

  * V21 fixes the emulation bug that v20 did (Echo RAM bug fix).  However, it does so without breaking compatibility with 19.x movies!
  * V20 bugs/regressions are not present in v21 (basically v21 takes 19.3 and re-fixed it in a much better way)


---


### New Features: ###
  * Lua scripting implemented! For a listing of lua functions: http://code.google.com/p/vba-rerecording/wiki/LuaScriptingFunctions?ts=1239498334&updated=LuaScriptingFunctions
  * Lag counter implemented

---

### Bug Fixes: ###
  * Fixed 'xn' window size would not reset when clicking on the option that is already checked.
  * Fix for not recording the  lag Reduction flag in GBA movies caused by using Ctrl-key-combos right before opening the recording
  * Fix crash caused by trying to open a file that doesn't exist and ends with .gba
  * Fix crash caused by the sequence "Load movie, use cheat search, close movie, open new movie, open cheat search"
  * Fixed a fatal bug that VBA doesn't clear out SRAM before movie playback.
  * Fixed RTC snapshot length problem

---

### Feature enhancements: ###
  * Drag and Drop for movie files
  * Merged the movie play and movie record menu items