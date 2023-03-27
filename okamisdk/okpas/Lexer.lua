return function(fileName, data)
    local tokens = {}
    local curLine = 1
    local lineStart = 1
    local curOffset = 1
    local startOffset = 1
    local reservedWords = {
        "and",
        "array",
        "begin",
        "case",
        "const",
        "div",
        "do",
        "downto",
        "else",
        "end",
        "file",
        "for",
        "function",
        "goto",
        "implementation",
        "interface",
        "if",
        "in",
        "label",
        "mod",
        "nil",
        "not",
        "of",
        "or",
        "packed",
        "procedure",
        "program",
        "record",
        "repeat",
        "set",
        "then",
        "to",
        "type",
        "until",
        "unit",
        "uses",
        "var",
        "while",
        "with"
    }

    local function isNumber(c)
        return c >= "0" and c <= "9"
    end

    local function isHexNumber(c)
        return (c >= "0" and c <= "9") or (string.lower(c) >= "a" and string.lower(c) <= "f")
    end

    local function isAlpha(c)
        return (c:lower() >= "a" and c:lower() <= "z") or c == "_"
    end

    local function isAlphanum(c)
        return isAlpha(c) or isNumber(c)
    end

    local function isSymbol(c)
        return c == "+" or c == "-" or c == "*" or c == "/" or c == "=" or c == "^" or c == "<" or c == ">" 
               or c == "(" or c == ")" or c == "[" or c == "]" or c == "." or c == "," or c == ":" or c == ";"
               or c == "@"
    end

    local function addToken(tokenType)
        table.insert(tokens,{type=tokenType,file=fileName,line=curLine,col=startOffset-lineStart,txt=data:sub(startOffset,curOffset)})
        startOffset = curOffset + 1
        curOffset = curOffset + 1
    end

    while curOffset < #data do
        if data:sub(curOffset,curOffset) == " " or data:sub(curOffset,curOffset) == "\r" or data:sub(curOffset,curOffset) == "\t" then
            curOffset = curOffset + 1
            startOffset = curOffset
        elseif data:sub(curOffset,curOffset) == "\n" then
            curOffset = curOffset + 1
            startOffset = curOffset
            curLine = curLine + 1
            lineStart = curOffset
        elseif data:sub(curOffset,curOffset) == "{" then -- Comment
            while data:sub(curOffset,curOffset) ~= "}" do
                if data:sub(curOffset,curOffset) == "\n" then
                    curOffset = curOffset + 1
                    curLine = curLine + 1
                    lineStart = curOffset
                else
                    curOffset = curOffset + 1
                end
            end
            curOffset = curOffset + 1
            startOffset = curOffset
        elseif isAlpha(data:sub(curOffset,curOffset)) then
            while isAlphanum(data:sub(curOffset,curOffset)) do curOffset = curOffset + 1 end
            curOffset = curOffset - 1
            local addedToken = false
            for _, i in ipairs(reservedWords) do
                if string.lower(data:sub(startOffset,curOffset)) == string.lower(i) then
                    addToken("keyword")
                    addedToken = true
                    break
                end
            end
            if not addedToken then
                addToken("identifier")
            end
        elseif data:sub(curOffset,curOffset) == "$" then
            curOffset = curOffset + 1
            print(data:sub(curOffset,curOffset))
            while isHexNumber(data:sub(curOffset,curOffset)) do curOffset = curOffset + 1 end
            curOffset = curOffset - 1
            addToken("number")
        elseif isNumber(data:sub(curOffset,curOffset)) then
            while isNumber(data:sub(curOffset,curOffset)) do curOffset = curOffset + 1 end
            curOffset = curOffset - 1
            addToken("number")
        elseif data:sub(curOffset,curOffset) == "'" then
            curOffset = curOffset + 1
            while data:sub(curOffset,curOffset) == "'" and data:sub(curOffset+1,curOffset+1) ~= "'" do curOffset = curOffset + 1 end
            curOffset = curOffset - 1
            addToken("string")
        elseif data:sub(curOffset,curOffset) == "#" then
            curOffset = curOffset + 1
            while isNumber(data:sub(curOffset,curOffset)) do curOffset = curOffset + 1 end
            curOffset = curOffset - 1
            addToken("character")
        elseif isSymbol(data:sub(curOffset,curOffset)) then
            if (data:sub(curOffset,curOffset) == "<" and (data:sub(curOffset+1,curOffset+1) == ">" or data:sub(curOffset+1,curOffset+1) == "=")) or (data:sub(curOffset,curOffset) == ">" and data:sub(curOffset+1,curOffset+1) == "=") then
                curOffset = curOffset + 1
            end
            addToken("symbol")
        else
            print("\x1b[1;31m"..fileName.."("..curLine..":"..(curOffset-lineStart)..") Unknown Symbol: "..data:sub(curOffset,curOffset).."\x1b[0m")
            os.exit(1)
        end
    end
    return tokens
end