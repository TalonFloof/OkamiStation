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
        "var",
        "while",
        "with"
    }

    local function isNumber(c)
        return c >= "0" and c <= "9"
    end

    local function isAlpha(c)
        return (c:lower() >= "a" and c:lower() <= "z") or c == "_"
    end

    local function isAlphanum(c)
        return isAlpha(c) or isNumber(c)
    end

    local function addToken(tokenType)
        table.insert(tokens,{type=tokenType,line=curLine,col=startOffset-lineStart,txt=data:sub(startOffset,curOffset)})
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
        elseif isAlpha(data:sub(curOffset,curOffset)) then
            while isAlphanum(data:sub(curOffset,curOffset)) do curOffset = curOffset + 1 end
            curOffset = curOffset - 1
            local addedToken = false
            for _, i in ipairs(reservedWords) do
                if data:sub(startOffset,curOffset) == i then
                    addToken("keyword")
                    addedToken = true
                    break
                end
            end
            if not addedToken then
                addToken("identifier")
            end
        elseif 
    end
end