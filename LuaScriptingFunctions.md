See also [Lua manual](http://www.lua.org/manual/5.1/).

**Important: nil is not 0. nil and 0 are two different entities.**


# emu/vba #

### vba.frameadvance() ###
Pauses script until the current frame has been emulated. Cannot be called by a coroutine or registered function.

### vba.pause() ###
Pauses emulator when the current frame has finished emulating.

### int vba.framecount() ###
Returns the frame count for the movie, or the number of frames from last reset otherwise.

### int vba.lagcount() ###
Returns the lag count.

### boolean vba.lagged() ###
Returns true if the current frame is a lag frame, false otherwise.

### boolean vba.emulating() ###
Returns true if a ROM is being emulated (regardless of being paused).

### vba.registerbefore(function) ###
The function passed will be executed just after the GBA input has updated at the start of the next frame.

### vba.registerafter(function) ###
The function passed will be executed just after the frame has finished, but before any emulation of the next frame.

### vba.registerexit(function) ###
The function passed will be executed when the Lua script has been ended.

### vba.message(string) ###
Displays the message on the screen.

### vba.print(string) ###
Prints the message to the Lua console.


# memory #

### int memory.readbyte(int) / readbyteunsigned ###
### int memory.readword(int) / readwordunsigned / readshort / readshortunsigned ###
### int memory.readdword(int) / readdwordunsigned / readlong / readlongunsigned ###
### int memory.readbytesigned(int) ###
### int memory.readwordsigned(int) / readshortsigned ###
### int memory.readdwordsigned(int) / readlongsigned ###
Returns the value read from the passed memory address. GB addresses at 01:Cxxx are should be passed as Dxxx. Byte = 1 byte, word/short = 2 bytes, dword/long = 4 bytes.

### string memory.readbyterange(int addr, int length) ###
Returns a string containing the chunk of memory from addr to addr+length. Length can be negative, data is still returned in ascending order. To access, use _string.byte(str,offset)_.

### memory.writebyte(int addr, int val) ###
### memory.writeword(int addr, int val) / writeshort ###
### memory.writedword(int addr, int val) / writelong ###
Writes the passed value to the passed memory address addr. GB addresses at 01:Cxxx are should be passed as Dxxx. Byte = 1 byte, word/short = 2 bytes, dword/long = 4 bytes.

### int memory.getregister(string) ###
Returns the value of the hardware register passed.  Hardware registers are: GB - a f b c d e h l af bc de hl sp pc; GBA - [r0](https://code.google.com/p/vba-rerecording/source/detail?r=0) .. [r15](https://code.google.com/p/vba-rerecording/source/detail?r=15), cpsr, spsr.

### memory.setregister(string, int) ###
Sets the hardware register passed with the value passed.

### memory.registerwrite(int addr, function func) / register ###
### memory.registerexec(int addr, function func) / registerrun / registerexecute ###
Registers the passed function to be called whenever the given address is written to / executed.


# joypad #

Before the next frame is emulated, one may set keys to be pressed. The buffer is cleared each frame.

### table joypad.get(int port) / read ###
Returns a table of all buttons. Does not read movie input. Key values are 1 for pressed, nil for not pressed. Keys for joypad table: (A, B, select, start, right, left, up, down, R, L). Keys are case-sensitive. When passed 0, the default joypad is read

### table joypad.getdown(int port) / readdown ###
Returns a table of only the buttons that are being pressed.

### table joypad.getup(int port) / readup ###
Returns a table of only the buttons that are not being pressed.

### joypad.set(int port, table buttons) / write ###
Sets the buttons to be pressed next frame.  Cannot clear buttons.


# savestate #

### object savestate.create(int slot=nil) ###
Creates a savestate object. If any argument is given, it must be from 1 to 12, and it corresponds with the numbered savestate slots. If no argument is given, the savestate can only be accessed by Lua.

### savestate.save(object savestate) ###
Saves the current state to the savestate object.

### savestate.load(object savestate) ###
Loads the state of the savestate object as the current state.


# movie #

### boolean movie.active() ###
Returns true if a movie is active.

### boolean movie.recording() ###
Returns true if a movie is recording.

### boolean movie.playing() ###
Returns true if a movie is playing.

### string movie.mode() ###
Returns "record" if movie is recording, "playback" if movie is replaying input, or nil if there is no movie.

### int movie.length() ###
Returns the length of the active movie, or otherwise 0.

### string movie.author() / getauthor ###
Returns the author info field of the movie file, or nil if there is no movie.

### string movie.name() / getname ###
Returns the name of the movie file, or nil if there is no movie.

### int movie.rerecordcount() ###
Returns the rerecord count of the active movie, or otherwise 0.

### movie.setrerecordcount(int) ###
Sets the rerecord count to the passed value.

### movie.rerecordcounting(boolean) ###
If set to true, no rerecords done by Lua are counted in the rerecord total. If set to false, rerecords done by Lua count. By default, rerecords count.

### movie.stop() / close ###
Stops the movie. Cannot be used if there is no movie.


# gui #

Before the next frame is drawn, one may draw to the buffer. The buffer is cleared each frame.

All functions assume that the width is 256 and the height of the image is 239, regardless of the type of game. GB(C) resolution = 160x144, SGB = 256x224, GBA = 240x160

Color can be given as 0xRRGGBBAA; like HTML ("#RRGGBBAA"); as a string (white, black, clear, gray, grey, red, orange, yellow, chatreuse, green, teal, cyan, blue, purple, magenta); a random colour ("rand"); or as a table, where the keys are 'r', 'g', 'b', and 'a' repsectively. For alpha, 00 is transparent, FF is opaque.

### function gui.register(function func) ###
The passed function will be called just before the screen is updated each frame. The passed function will be registered in place of the previous, and the previous returned. Functions can be unregistered by passing nil.

### gui.text(int x, int y, string msg, type fillcolor, type bordercolor) / drawtext ###
Draws the given text at (x,y). Not to be confused with _vba.message(string msg)_.

### gui.box(int x1, int y1, int x2, int y2, type fillcolor, type bordercolor) / drawbox / rect / drawrect ###
Draws a box with (x1,y1) and (x2,y2) as opposite corners with the given colors.

### gui.line(int x1, int y1, int x2, int y2, type color, boolean skipfirst) / drawline ###
Draws a line from (x1,y1) to (x2,y2) with the given color. The first pixel of the line is only drawn when skipfirst is false.

### gui.pixel(int x, int y, type color) / drawpixel / setpixel / writepixel ###
Draws a pixel at (x,y) with the given color.

### gui.opacity(float alpha) ###
Sets the opacity of drawings depending on alpha. 0.0 is transparent, 1.0 is opaque. Values less than 0.0 or greater than 1.0 work by extrapolation.

### gui.transparency(float strength) ###
4.0 is transparent, 0.0 is opaque. Values less than 0.0 or greater than 4.0 work by extrapolation.

### string gui.popup(string msg, string type = "ok") ###
Creates a pop-up dialog box with the given text and some dialog buttons. There are three types: "ok", "yesno", and "yesnocancel". If "yesno" or "yesnocancel", it returns either "yes", "no", or "cancel", depending on the button pressed. If "ok", it returns nil.

### int int int int gui.parsecolor(type colour) ###
Returns the red, green, blue and alpha values for the passed colour.

### string gui.gdscreenshot() ###
Takes a screenshot and returns it as a string that can be used by the [gd library](http://lua-gd.luaforge.net/).

For direct access, use _string.byte(str,offset)_. The gd image consists of a 11-byte header and each pixel is alpha,red,green,blue (1 byte each, alpha is 0 in this case) left to right then top to bottom.

### gui.gdoverlay(int destX=0, int destY=0, string gdimage, [srcX, int srcY, int width, int height,](int.md) float alpha=1.0) / drawimage / image ###
Overlays (a section of) the passed gd image with top-left corner at (destX,destY) with the passed opacity.  If giving the source parameters, all four must be given.

### int int int gui.getpixel(int x, int y) / readpixel ###
Returns the red, green and blue values for the pixel at the passed (x,y).


# input #

### table input.get() / read ###
Returns a table of which keyboard buttons are pressed as well as mouse status. Key values for keyboard buttons and mouse clicks are true for pressed, nil for not pressed. Mouse position is returned in terms of game screen pixel coordinates. Keys for mouse are (xmouse, ymouse, leftclick, rightclick, middleclick).

Keys for keyboard buttons: backspace, tab, enter, shift, control, alt, pause, capslock, escape, space, pageup, pagedown, end, home, left, up, right, down, insert, delete, 0 .. 9, A .. Z, numpad0 .. numpad9, numpad`*`, numpad+, numpad-, numpad., numpad/, F1 .. F24, numlock, scrolllock, semicolon, plus, comma, minus, period, slash, tilde, leftbracket, backslash, rightbracket, quote.

Keys are case-sensitive. Keys for keyboard buttons are for buttons, **not** ASCII characters, so there is no need to hold down shift. Key names may differ depending on keyboard layout. On US keyboard layouts, "slash" is /?, "tilde" is `````~, "leftbracket" is `[``{`, "backslash" is \|, "rightbracket" is `]``}`, "quote" is '".


# bit #

All the functions from the bitop module are included, see http://bitop.luajit.org/


# avi #

### int avi.framcount() ###
Returns the number of frames recorded to AVI if an AVI is being recorded, else 0 is returned.


# other #

### print(string) ###
Prints the message to the Lua console.

### string tostring(type) ###
Returns a string representation of the passed data.

### int addressof(pointer) ###
Returns the value of the passed pointer, because the print function returns the content of the pointer.

### table copytable(table) ###
Returns a shallow copy of the passed table.

### int AND(int, ...) ###
### int OR(int, ...) ###
### int XOR(int, ...) ###
Aliases for the same functions in the bitop module.

### int SHIFT(int, int) ###
Shifts right when positive, shifts the inverse left when negative.

### int BIT(int, ...) ###
Returns an integer with the passed bits set.