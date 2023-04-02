local args = {...}

local scan = dofile("scanner.lua")
local parse = dofile("parser.lua")

local tokens = {}

function serialize_list (list,newlines,subNewline)
    local str = "("
    for _,i in ipairs(list) do
        if type(i) == "table" then
            str = str .. serialize_list(i,subNewline,subNewline)
        else
            str = str .. tostring(i)
        end
        str = str .. (newlines and "\n" or " ")
    end
    if #list > 0 then
        str = string.sub(str,1,#str-1)
    end
    str = str .. ")"
    return str
end

local nl = string.char(10)
function serialize_table (tabl, indent)
    indent = indent and (indent.."  ") or ""
    local str = ''
    str = str .. indent.."{"..nl
    for key, value in pairs (tabl) do
        local pr = (type(key)=="string") and ('["'..key..'"]=') or ""
        if type (value) == "table" then
            str = str..pr..serialize_table (value, indent)
        elseif type (value) == "string" then
            str = str..indent..pr..'"'..tostring(value)..'",'..nl
        else
            str = str..indent..pr..tostring(value)..','..nl
        end
    end
    str = str .. indent.."},"..nl
    return str
end

local function addToTokens(t)
    while #t > 0 do
        table.insert(tokens,table.remove(t,1))
    end
end

for _,i in ipairs(args) do
    local file = io.open(i,"rb")
    addToTokens(scan(i,file:read("*a")))
    file:close()
end
print(parse(tokens))