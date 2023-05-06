return function(fileName,module)
    local keywords = {
        ["ARRAY"] = "arrayKw",
        ["ASM"] = "asmKw",
        ["BEGIN"] = "beginKw",
        ["CASE"] = false,
        ["CONST"] = "constKw",
        ["DIV"] = "divKw",
        ["DO"] = "doKw",
        ["ELSE"] = "elseKw",
        ["ELSIF"] = "elsifKw",
        ["EXTERN"] = "externKw",
        ["END"] = "endKw",
        ["EXIT"] = false,
        ["IF"] = "ifKw",
        ["IMPORT"] = "importKw",
        ["IN"] = false,
        ["IS"] = false,
        ["LOOP"] = false,
        ["MOD"] = "modKw",
        ["MODULE"] = "moduleKw",
        ["NIL"] = "nilKw",
        ["NOT"] = "notKw",
        ["OF"] = "ofKw",
        ["OR"] = "orKw",
        ["POINTER"] = "pointerKw",
        ["PROCEDURE"] = "procedureKw",
        ["RECORD"] = "recordKw",
        ["REPEAT"] = false,
        --["RETURN"] = false,
        ["THEN"] = "thenKw",
        ["TO"] = "toKw",
        ["TYPE"] = "typeKw",
        ["UNTIL"] = false,
        ["VAR"] = "varKw",
        ["WHILE"] = "whileKw",
        ["WITH"] = false,
        ["XOR"] = "xorKw",
    }
    local symbols = {
        ["+"] = "plus",
        ["-"] = "minus",
        ["*"] = "mul",
        ["*|"] = "umul",
        ["/"] = "div",
        ["/|"] = "udiv",
        ["~"] = "not",
        ["&"] = "and",
        ["."] = "dot",
        [","] = "comma",
        [";"] = "semicolon",
        ["|"] = "pipe",
        ["("] = "lparen",
        ["["] = "lbracket",
        ["{"] = "lbrace",
        [":="] = "assign",
        ["^"] = "caret",
        ["="] = "eq",
        ["#"] = "neq",
        ["<"] = "lt",
        [">"] = "gt",
        ["<="] = "leq",
        [">="] = "geq",
        ["<|"] = "ult",
        [">|"] = "ugt",
        ["<|="] = "uleq",
        [">|="] = "ugeq",
        [".."] = "dots",
        [":"] = "colon",
        [")"] = "rparen",
        ["]"] = "rbracket",
        ["}"] = "rbrace",
    }
    local tokens = {}
    local lineStart = 1
    local line = 1
    local cursor = 1
    local cursorStart = 1
    local function addToken(t)
        table.insert(tokens,{type=t,file=fileName,line=line,col=cursorStart-lineStart,txt=module:sub(cursorStart,cursor)})
        cursor = cursor + 1
        cursorStart = cursor
    end
    local function isAlpha(c)
        return (c >= "A" and c <= "Z") or (c >= "a" and c <= "z") or c == "_"
    end
    local function isDigit(c)
        return c >= "0" and c <= "9"
    end
    while cursor < #module+1 do
        if module:sub(cursorStart,cursor) == "\n" or module:sub(cursorStart,cursor) == "\r" then
            cursor = cursor + 1
            cursorStart = cursor
            line = line + 1
            lineStart = cursorStart
        elseif module:sub(cursorStart,cursor) == " " or module:sub(cursorStart,cursor) == "\t" then
            cursor = cursor + 1
            cursorStart = cursor
        elseif module:sub(cursorStart,cursor+1) == "(*" then
            cursor = cursor + 2
            while module:sub(cursor,cursor+1) ~= "*)" and cursor < #module+1 do
                if module:sub(cursor,cursor) == "\n" or module:sub(cursor,cursor) == "\r" then
                    line = line + 1
                    lineStart = cursor+1
                end
                cursor = cursor + 1
            end
            cursor = cursor + 2
            cursorStart = cursor
        elseif isAlpha(module:sub(cursor,cursor)) then
            while isAlpha(module:sub(cursor,cursor)) or isDigit(module:sub(cursor,cursor)) and cursor < #module+1 do cursor = cursor + 1 end
            cursor = cursor - 1
            if keywords[module:sub(cursorStart,cursor)] then
                addToken(keywords[module:sub(cursorStart,cursor)])
            else
                addToken("identifier")
            end
        elseif module:sub(cursor,cursor) == "\"" then
            cursor = cursor + 1
            while not (module:sub(cursor,cursor) == "\"" and module:sub(cursor-1,cursor-1) ~= "\\") and cursor < #module+1 do cursor = cursor + 1 end
            addToken("string")
        elseif isDigit(module:sub(cursor,cursor)) then
            while isAlpha(module:sub(cursor,cursor)) or isDigit(module:sub(cursor,cursor)) and cursor < #module+1 do cursor = cursor + 1 end
            cursor = cursor - 1
            addToken("number")
    elseif symbols[module:sub(cursorStart,cursor)] ~= nil then
            while symbols[module:sub(cursorStart,cursor)] ~= nil and cursor < #module+1 do cursor = cursor + 1 end
            cursor = cursor - 1
            addToken(symbols[module:sub(cursorStart,cursor)])
        else
            io.stderr:write("\x1b[1;31m"..fileName.."("..line..":"..(cursorStart-lineStart)..") Unknown Token!\x1b[0m\n")
            os.exit(2)
        end
    end
    return tokens
end