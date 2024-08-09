
# zyd-toolkit

This project is a toolkit designed for developers working on Windows. It brings together a bunch of handy modules that make it easier to manage different aspects of the system and build custom features. Whether you're looking to dig deeper into Windows or just need some extra tools to get things done, this toolkit has got you covered. It's all built specifically for Windows, so you can count on it to work seamlessly with your system.


## Features

- Mouse
```lua
-- Click mouse buttons at specific coords (if not specified then current location will be used)
mouse.left(x,y)
mouse.right(x,y)
mouse.middle(x,y)
```
- Keyboard
```lua
keyboard.send_keys(text)
```
- Screen
```lua
screen.getResolution() -- returns width and height of main screen
screen.getScreenshot() -- returns screenshot as base64 encoded string
```
- Window
```lua
window.alert(text,title) -- Windows alert box
window.confirm(text,title) -- Wndows confirm box (returns true/false depending on what was clicked)
```
- Process
```lua
process.list() -- returns table that contains PIDs
process.get_name(PID) -- returns name of the process
process.get_path(PID) -- returns executable path of the process
process.get_memory_usage(PID) -- returns memory usage of process in bytes
process.freeze(PID)
process.unfreeze(PID)
process.kill(PID) -- kills specified process
```
- Requests
```lua
local session = ZYD.session:new(proxy,username,password) -- Args are not required
session:get(url,headers) -- returns html code as string, headers should be in table - ex. {["Host"]="a.com"}
session:post(url,headers,data) -- headers and data should be in table
```
-
## Credits
[Lodepng](https://github.com/lvandeve/lodepng)

[ImGui](https://github.com/ocornut/imgui)
