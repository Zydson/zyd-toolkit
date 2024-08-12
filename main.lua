---@diagnostic disable: param-type-mismatch
ZYD = {}
ZYD.OperatingSystem = ""
ZYD.session = {}
ZYD.session.__index = ZYD.session
ZYD.Base64 = {}

if tostring(package.cpath:match("%p[\\|/]?%p(%a+)")) == "dll" then
	ZYD.OperatingSystem = "Windows"
else
	ZYD.OperatingSystem = "Linux"
end

ZYD.Errors = {
	["Count"] = 0,
	["Threeshold"] = 10
}

package.cpath = package.cpath .. ";./modules/?.dll"
json = require "modules/json" 		-- Json Library
base64 = require "modules/base64"	-- Base64 Library
mouse = require("z_mouse")			-- Mouse API
keyboard = require("z_keyboard")	-- Keyboard API
screen = require("z_screen")		-- Screen API
window = require("z_window")		-- Window API
process = require("z_process")		-- Process API
imgui = require("z_imgui")			-- ImGui
sqlite = require("z_sqlite")		-- Sqlite
http_serv = require("z_httpserver")
threading = require("z_threading")

local _screenshot_hook = screen.getScreenshot
screen.getScreenshot = function()
	return ZYD.Base64:encode(_screenshot_hook())
end

ZYD.Error = function(text,functionName, critical, count)
	if count then
		ZYD.Errors["Count"] = ZYD.Errors["Count"] + 1
	end
	if critical then
		print("Critical error occured: "..text.." - [Function: "..functionName.."]")
		print("killing process...")
		os.exit()
	end
	if functionName ~= nil then
		print("Error occured: "..text.." - [Function: "..functionName.."]")
	else
		print("Error occured: "..text)
	end
	if ZYD.Errors["Count"] >= ZYD.Errors["Threeshold"] then
		print("Error threeshold has been reached, killing process")
		os.exit()
	end
end

ZYD.Download = function(url, path)
	if ZYD.OperatingSystem == "Linux" then
		if path == nil then
			os.execute('wget "'..url..'"')
		else
			os.execute('wget -P '..path..' "'..url..'"')
		end
	elseif ZYD.OperatingSystem == "Windows" then
		if path == nil then
			print('curl -O "'..url..'"')
			os.execute('curl -O "'..url..'"')
		else
			os.execute('curl -o '..path..' "'..url..'"')
		end
	end
end

function ZYD.session:new(proxy,username,password)
    local instance = setmetatable({}, ZYD.session)
	if proxy ~= nil then
    	instance.proxy = proxy:gsub("http://",""):gsub("https://","")
	else
		instance.proxy = nil
	end
	instance.username = username
	instance.password = password
    return instance
end

function ZYD.session:convert_headers(headers)
	local construct_string = ""
	if type(headers) == "string" then
		for x in string.gmatch(headers,'[^\r\n]+') do 
			construct_string = construct_string .. " " .. '-H "'..x..'"'
		end
	elseif type(headers) == "table" then
		for x,y in pairs(headers) do
			construct_string = construct_string .. " " .. '-H "'..x..': '..y..'"'
		end
	end
	return construct_string
end

function ZYD.session:get_proxy_string()
	local construct_string = ""
	if self.proxy then
		construct_string = ' -x "http://'..self.proxy..'"'
		if self.username and self.password then
			construct_string = construct_string..' --proxy-user "'..self.username..":"..self.password..'"'
		end
	end
	return construct_string
end

function ZYD.session:get(url,headers)
    if url ~= nil then
        local hand = assert(io.popen('curl "'..url..'" -s '..self:convert_headers(headers)..self:get_proxy_string()))
        local response = hand:read("*all")
        io.close(hand)
        return response
    end
end

function ZYD.session:post(url,headers)
    if url ~= nil then
        local handler = assert(io.popen('curl -s -m 3 -X POST "'..url..'" -s '..self:convert_headers(headers)..self:get_proxy_string()))
        local response = handler:read("*all")
        io.close(handler)
        return response
    end
end

ZYD.Execute = function(command)
	os.execute(command)
end

ZYD.WaitPC = function(ms)
	if ZYD.OperatingSystem == "Linux" then
		local sec = tonumber(ms/1000)
		ZYD.Execute("sleep "..sec)
	else
		ZYD.Execute("timeout "..ms.." > nul")
	end
end

ZYD.Wait = function(ms)
	if type(ms) == "number" then
		pcall(ZYD.WaitPC, ms)
	else
		ZYD.Error("expected number not ["..type(ms).."] args[ms]", "ZYD.Wait", false)
		return "Error"
	end
end

ZYD.JsonValidation = function(text)
	json.decode(text)
	return true
end

ZYD.LoadJson =  function(file)
	local fileJ = io.open(file, "r")
		if not fileJ then
			ZYD.Error("can't find ["..file.."]", "ZYD.SaveJson", true)
		end
	if fileJ ~= nil then
		local jsonT = fileJ:read("*all")
		io.close(fileJ)
		if pcall(ZYD.JsonValidation, jsonT) then
			return json.decode(jsonT)
		else
			if string.len(jsonT) == 0 then
				return "free"
			else
				ZYD.Error("can't decode ["..file.."]-'possible syntax error'", "ZYD.SaveJson", true)
				return "Validiation error"
			end
		end
	end
end

ZYD.SaveJson = function(file, tab, new)
	local currentJ = ZYD.LoadJson(file)
	if currentJ == "Validiation error" then
		--pass
	elseif currentJ == "free" or new then
		local jsonG = io.open(file, "w+")
		if jsonG ~= nil then
			jsonG:write(json.encode(tab))
		end
		io.close(jsonG)
	elseif currentJ == "no file" then
		--pass
	else
		local tempTab = {}
		for a,b in pairs(currentJ) do
			table.insert(tempTab,b)
		end
		for a,b in pairs(tab) do
			table.insert(tempTab,b)
		end
		local jsonG = io.open(file, "w+")
		if jsonG ~= nil then
			jsonG:write(json.encode(tempTab))
		end
		io.close(jsonG)
	end
end

ZYD.Average = function(tab)
	if type(tab) ~= "table" then
		ZYD.Error("expected table not ["..type(tab).."] args[tab]","ZYD.Average",false)
		return "Error"
	elseif #tab == 0 then
		ZYD.Error("table is empty ["..json.encode(tab).."] args[tab]","ZYD.Average",false)
		return "Error"
	end

	local overall = 0
	for a,b in ipairs(tab) do
		if type(b) == "number" then
			overall = overall + b
		end
	end
	return (overall/#tab)
end

ZYD.Parse = function(targetString, leftstring, rightstring)
	if targetString ~= nil then
		if type(targetString) == "string" then
			local _ls = string.find(targetString,leftstring) or -9
			local toright = string.sub(targetString, _ls+#leftstring, #targetString)
			local _rs = string.find(toright,rightstring) or -9
			if (_ls ~= nil and _rs ~= nil) and not (_ls == -9 or _rs == -9) then
				return string.sub(targetString, _ls+#leftstring, _rs-2+_ls+#leftstring)
			else
				return "Can't find matching string"
			end
		end
	else
		ZYD.Error("args[targetString] cant be nil","ZYD.Parse",false)
		return "Error"
	end
end

function ZYD.Base64:decode(encryptedString)
	local status, result = pcall(base64.decode,encryptedString)
	if status then
		return result
	else
		ZYD.Error("while trying to encode string args[encryptedString]","ZYD.Base64:decode",false)
		return false
	end
end

function ZYD.Base64:encode(decrytpedString)
	local status, result = pcall(base64.encode,decrytpedString)
	if status then
		return result
	else
		ZYD.Error("while trying to encode string args[decrytpedString]","ZYD.Base64:encode",false)
		return false
	end
end

function ZYD.Base64:toImage(encryptedString,format)
	if format == nil then
		format = "png"
	end
	local a = ZYD.Base64:decode(encryptedString)
	if a then
		io.open("image."..format, "wb"):write(a):close()
	end
end