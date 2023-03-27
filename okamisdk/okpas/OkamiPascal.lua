local args = {...}

local nl = string.char(10) -- newline
function serialize_list (tabl, indent)
    indent = indent and (indent.."  ") or ""
    local str = ''
    str = str .. indent.."{"..nl
    for key, value in pairs (tabl) do
        local pr = (type(key)=="string") and ('["'..key..'"]=') or ""
        if type (value) == "table" then
            str = str..pr..serialize_list (value, indent)
        elseif type (value) == "string" then
            str = str..indent..pr..'"'..tostring(value)..'",'..nl
        else
            str = str..indent..pr..tostring(value)..','..nl
        end
    end
    str = str .. indent.."},"..nl
    return str
end

local infile = io.open(args[1],"r")
local sourceCode = infile:read("*a")
infile:close()

local lexer = require("Lexer")

print(serialize_list(lexer(args[1],sourceCode)))