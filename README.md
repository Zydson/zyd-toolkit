
# zyd-toolkit

This project is a toolkit designed for developers working on Windows. It brings together a bunch of handy modules that make it easier to manage different aspects of the system and build custom features. Whether you're looking to dig deeper into Windows or just need some extra tools to get things done, this toolkit has got you covered. It's all built specifically for Windows, so you can count on it to work seamlessly with your system.

## Compilation
For compilation we will be using [w64devkit](https://github.com/skeeto/w64devkit), stick to their installation tutorial
```sh
gcc -shared -o modules/z_screen.dll -I lua src/z_screen.c src/lodepng.c -L lua/lib -llua54 -lUser32 -lgdiplus -lGdi32 -Os -s
gcc -shared -o modules/z_mouse.dll -I lua src/z_mouse.c -L lua/lib -llua54 -lUser32 -lGdi32 -Os -s
gcc -shared -o modules/z_window.dll -I lua src/z_window.c -L lua/lib -llua54 -lUser32 -lGdi32 -Os -s
gcc -shared -o modules/z_process.dll -I lua src/z_process.c -L lua/lib -llua54 -lUser32 -lGdi32 -Os -s
gcc -shared -o modules/z_keyboard.dll -I lua src/z_keyboard.c -L lua/lib -llua54 -lUser32 -lGdi32 -Os -s
gcc -shared -o modules/z_memory.dll -I lua src/z_memory.c -L lua/lib -llua54 -lUser32 -lkernel32 -lGdi32 -Os -s
gcc -shared -o modules/z_sqlite.dll -I lua -I sqlite src/z_sqlite.c sqlite/sqlite3.c -L lua/lib -llua54 -Os -s
gcc -shared -o modules/z_httpserver.dll -I lua src/z_httpserver.c -L lua/lib -llua54 -lUser32 -lws2_32 -lGdi32 -Os -s
gcc -shared -o modules/z_threading.dll -I lua src/z_threading.c -L lua/lib -llua54 -lUser32 -lGdi32 -Os -s
g++ -shared -o modules/z_imgui.dll -I lua src/z_imgui.c -L lua/lib -llua54 imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_tables.cpp imgui/imgui_demo.cpp imgui/backends/imgui_impl_dx10.cpp imgui/backends/imgui_impl_win32.cpp -I imgui -I imgui/backends -ld3d10 -ld3dcompiler -lcomdlg32 -lole32 -luser32 -lgdi32 -ldwmapi -Os -s
```

## How to use it?

```lua
require "main" -- Imports all the features that you can see below

print(screen.getResolution()["width"])
```

## Features

- HTTP Server

```lua
-- request table contains (url,method,ip,headers)
http_serv.route("/", function(request)
    return Render("index.html",{title="abc",some="value"}) --return dynamic html page, see usage below
    --[[
        <title>{{ title }}</title> -- {{ title }} will be replaced with "abc"
        <h1>{{some}}</h1> -- {{some}} will be replaced with "value"
    ]]
end)

http_serv.route("/", function(request)
    return "Hello, World!" -- You can return plain text
end)

http_serv.route("/json", function(request)
    return {a = "value", a1 = "value"} -- You can return table which will be parsed to json
end)

http_serv.route("/error", function(request)
    return 404 -- You can also return status code
    return 429, "Rate limited" -- it will work too
end)

http_serv.start(port) 
```

- sqlite

```lua
local db, err = sqlite.open("test.db") -- If not found, new one will be created

db:exec("CREATE TABLE `test`(`id` INTEGER PRIMARY KEY AUTOINCREMENT, `something` TEXT NOT NULL)")
db:exec("INSERT INTO `test`(`something`) VALUES ('nothing')")
local results = db:exec("SELECT * FROM `test`;")

for _, row in ipairs(results) do
    print(row.id, row.something)
end

db:close()
```

- ImGui

```lua
local text = "Hello, World!"
local checkboxValue = false
local sliderFloatValue = 0.5
local sliderIntValue = 50
local inputTextValue = "It is an example"
local comboIndex = 1
local comboItems = {"Option 1", "Option 2", "Option 3"}
local colorValue = {1.0, 0.0, 0.0, 1.0}

function lua_render()
    ImGuiC.Text(text)
    checkboxValue = ImGuiC.Checkbox(text, checkboxValue)
    sliderIntValue = ImGuiC.SliderInt(text, sliderIntValue, 0, 100)
    sliderFloatValue = ImGuiC.SliderFloat(text, sliderFloatValue, 0.0, 1.0)
    inputTextValue = ImGuiC.InputText(text, inputTextValue)
    comboIndex = ImGuiC.Combo(text, comboItems, comboIndex)
    local r, g, b, a = ImGuiC.ColorButton(text, colorValue[1], colorValue[2], colorValue[3], colorValue[4])
end

imgui.initialize(title, width, height)

while true do
    imgui.renderFrame()
end
```

- Threads

```lua
threading.Thread(some_func,arg1,arg2,arg3,...)
print("Hello") -- This will be executed wheter the function above was fully executed
```

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
process.freeze(PID) -- freezes all threads execution
process.unfreeze(PID)
process.kill(PID) -- kills specified process
```

- Requests

```lua
local session = ZYD.session:new(proxy,username,password) -- Args are not required
session:get(url,headers) -- returns html code as string, headers should be in table - ex. {["Host"]="a.com"}
session:post(url,headers,data) -- headers and data should be in table
```

## Credits
[Lodepng](https://github.com/lvandeve/lodepng)

[ImGui](https://github.com/ocornut/imgui)

[sqlite](https://github.com/sqlite/sqlite)