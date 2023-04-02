return function(tokens)
    local cursor = 1;
    local tree = {}
    local cur
    local function parseErr(file,line,col,err)
        io.stderr:write("\x1b[1;31m"..file.."("..line..":"..col..") "..err.."\x1b[0m\n")
        os.exit(3)
    end
    local function shunt(ending)
        local NONE = 0
        local LEFT = 1
        local RIGHT = 2
        local finalInsns = {}
        local opTable = {
            {".",0,LEFT,false},
            {"]",0,NONE,false},
            {"(",0,NONE,false},
            {")",0,NONE,false},
            {"+",2,LEFT,false},
            {"-",2,LEFT,false},
            {"_",4,RIGHT,true,function(x) return 0 - x end}, -- Unary -
            {"^",4,LEFT,true,function(x) parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Attempt to decast immediate value") end},
            {"*",1,LEFT,false},
            {"/",1,LEFT,false},
            {"DIV",1,LEFT,false},
            {"MOD",1,LEFT,false},
            {"&",1,LEFT,false},
            {"XOR",2,LEFT,false},
            {"OR",2,LEFT,false},
            {"~",4,RIGHT,true,function(x) return ~x end},
            {"=",3,LEFT,false},
            {"#",3,LEFT,false},
            {"<",3,LEFT,false},
            {">",3,LEFT,false},
            {"<=",3,LEFT,false},
            {">=",3,LEFT,false},
            {":=",10,LEFT,false},
        }
        local opStack = {}
        local outStack = {}
        local lastOp = {"X",0,NONE,false}
        local function getOp(c)
            for _,i in ipairs(opTable) do
                if i[1] == c then
                    return i
                end
            end
            return nil
        end
        local function popOp()
            local val = opStack[#opStack]
            table.remove(opStack,#opStack)
            return val
        end
        local function popNum()
            local val = outStack[#outStack]
            table.remove(outStack,#outStack)
            return val
        end
        local function parenSkip(c)
            local val = c
            val = val + 1
            while tokens[val].type ~= "rparen" do
                if tokens[val].type == "lparen" then
                    parenSkip(val)
                else
                    val = val + 1
                end
            end
            val = val + 1
            return val
        end
        local function addOp(op)
            if op then
                if lastOp then
                    if lastOp[1] == "X" or lastOp[1] ~= ")" then
                        if op[1] == "-" then op=getOp("_") end
                    end
                end
                while #opStack > 0 and opStack[#opStack][1] ~= "(" do
                    local po1 = op[2]
                    local po2 = opStack[#opStack][2]
                    local operator = nil
                    if opStack[#opStack][4] then operator = popOp()
                    elseif po2 > po2 then operator = popOp()
                    elseif po2 == po1 and op[3] == LEFT then operator = popOp()
                    else break end
                    if operator[4] and type(outStack[#outStack]) == "number" then
                        local val = popNum()
                        table.insert(outStack,operator[5](val))
                    else
                        table.insert(outStack,operator)
                    end
                end
                table.insert(opStack,op)
                lastOp = op
            end
        end
        while tokens[cursor].type ~= (ending or "semicolon") do
            if tokens[cursor].type == "lparen" then
                table.insert(opStack,{"(",0,NONE,false})
            elseif tokens[cursor].type == "comma" then
                while #opStack > 0 and opStack[#opStack][1] ~= "(" do
                    local pop = popOp()
                    table.insert(outStack,pop)
                end
            elseif tokens[cursor].type == "rbracket" then
                while #opStack > 0 and opStack[#opStack][1] ~= "[" do
                    local pop = popOp()
                    table.insert(outStack,pop)
                end
                if #opStack == 0 then
                    parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Freestanding \"]\" in expression")
                end
                table.insert(outStack,popOp())
            elseif tokens[cursor].type == "rparen" then
                while #opStack > 0 and opStack[#opStack][1] ~= "(" do
                    local pop = popOp()
                    table.insert(outStack,pop)
                end
                if #opStack == 0 then
                    parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Freestanding \")\" in expression")
                end
                popOp()
            elseif tokens[cursor].type == "lbracket" and tokens[cursor-1].type == "identifier" then
                addOp({"[",0,NONE,false})
            elseif tokens[cursor].type == "identifier" and tokens[cursor+1].type == "lparen" then
                -- Count Arguments
                local c = cursor+2
                local argCount = 0
                while tokens[c].type ~= "rparen" do
                    if argCount == 0 then argCount = 1 end
                    if tokens[c].type == "lparen" then
                        c = parenSkip(c)
                    end
                    if tokens[c].type == "comma" and tokens[c+1].type ~= "rparen" then argCount = argCount + 1 end
                    c = c + 1
                end
                table.insert(opStack,{"FN",tokens[cursor].txt,argCount})
                inFunc = true
            elseif tokens[cursor].type == "identifier" and not getOp(tokens[cursor].txt) then
                table.insert(outStack,tokens[cursor].txt)
            elseif tokens[cursor].type == "number" then
                if string.sub(tokens[cursor].txt,#tokens[cursor].txt,#tokens[cursor].txt) == "H" then
                    table.insert(outStack,load("return 0x"..string.sub(tokens[cursor].txt,1,#tokens[cursor].txt-1))())
                else
                    table.insert(outStack,tonumber(tokens[cursor].txt))
                end
            elseif tokens[cursor].type == "string" then
                table.insert(outStack,{"STR",tokens[cursor].txt})
            else
                local op = getOp(tokens[cursor].txt)
                addOp(op)
            end
            cursor = cursor + 1
        end
        while #opStack > 0 do
            if opStack[#opStack][1] ~= "(" then
                local val = popOp()
                table.insert(outStack,val)
            else popOp() end
        end
        local tempStack = {}
        for _,i in ipairs(outStack) do
            if type(i) == "number" then
                table.insert(tempStack,{"number",i})
            elseif type(i) == "string" then
                table.insert(tempStack,{"symbol",i})                
            elseif type(i) == "table" then
                if i[1] == "STR" then
                    table.insert(tempStack,{"string",i[2]})
                elseif i[1] == "FN" then
                    local arguments = {}
                    local ind = (#tempStack-i[3])+1
                    for x=1,i[3] do table.insert(arguments,table.remove(tempStack,ind)) end
                    table.insert(arguments,1,i[2])
                    table.insert(arguments,1,"call")
                    table.insert(tempStack,arguments)
                else
                    local arguments = {}
                    local n1 = table.remove(tempStack,#tempStack)
                    local n2 = nil
                    if i[4] then
                        table.insert(tempStack,{i[1],n1})
                    else
                        n2 = table.remove(tempStack,#tempStack)
                        table.insert(tempStack,{i[1],n2,n1})
                    end
                end
            end
        end
        return table.remove(tempStack,1)
    end
    local function expectToken(type)
        if tokens[cursor].type ~= type then
            parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Expected \""..type.."\" token, got \""..tokens[cursor].type.."\" token")
        end
    end
    local function parseProcedure(t)
        local tab = t
        if tab == 0 then

        end
        return tab
    end
    local function parseType()
        if tokens[cursor].type == "identifier" then
            if tokens[cursor].txt == "CHAR" then
                return {"numType",1,true}
            elseif tokens[cursor].txt == "BYTE" then
                return {"numType",1,false}
            elseif tokens[cursor].txt == "SHORT" then
                return {"numType",2,true}
            elseif tokens[cursor].txt == "USHORT" then
                return {"numType",2,false}
            elseif tokens[cursor].txt == "INT" or tokens[cursor].txt == "LONG" or tokens[cursor].txt == "PTR" then
                return {"numType",4,true}
            elseif tokens[cursor].txt == "UINT" or tokens[cursor].txt == "ULONG" or tokens[cursor].txt == "UPTR" then
                return {"numType",4,false}
            else
                return {"customType",tokens[cursor].txt}
            end
        elseif tokens[cursor].type == "arrayKw" then
            cursor = cursor + 1
            expectToken("number")
            local size = tonumber(tokens[cursor].txt)
            cursor = cursor + 1
            expectToken("ofKw")
            cursor = cursor + 1
            return {"array",size,parseType()}
        elseif tokens[cursor].type == "pointerKw" then
            cursor = cursor + 1
            expectToken("toKw")
            cursor = cursor + 1
            return {"ptrOf",parseType()}
        end
    end
    local function parseTypeSection()

    end
    local function parseVariable(isfn)
        local out = {}
        local idents = {}
        while tokens[cursor].type ~= "colon" do
            expectToken("identifier")
            table.insert(idents,tokens[cursor].txt)
            cursor = cursor + 1
            if tokens[cursor].type ~= "colon" then
                expectToken("comma")
                cursor = cursor + 1
            end
        end
        cursor = cursor + 1
        local typ = parseType()
        cursor = cursor + 1
        expectToken("semicolon")
        cursor = cursor + 1
        for _,i in ipairs(idents) do
            table.insert(out,{i,typ})
        end
        return out
    end
    while cursor < #tokens+1 do
        expectToken("moduleKw")
        cursor = cursor + 1
        expectToken("identifier")
        local modName = tokens[cursor].txt
        cursor = cursor + 1
        expectToken("semicolon")
        cursor = cursor + 1
        local module = {"module",modName,{},{},{},{}}
        while not (tokens[cursor].type == "endKw" and tokens[cursor+1].txt == modName and tokens[cursor+2].type == "dot") do
            if tokens[cursor].type == "importKw" then
                cursor = cursor + 1
                while tokens[cursor].type ~= "semicolon" do
                    expectToken("identifier")
                    table.insert(module[3],tokens[cursor].txt)
                    cursor = cursor + 1
                    if tokens[cursor].type ~= "comma" and tokens[cursor].type ~= "semicolon" then
                        parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Unexpected Token: \""..tokens[cursor].type.."\"")
                    end
                    if tokens[cursor].type ~= "semicolon" then cursor = cursor + 1 end
                end
                cursor = cursor + 1
            elseif tokens[cursor].type == "typeKw" then
                parseTypeSection()
            elseif tokens[cursor].type == "procedureKw" then
                cursor = cursor + 1
                expectToken("identifier")
                local procName = tokens[cursor].txt
                cursor = cursor + 1
                local args = {}
                if tokens[cursor].type == "lparen" then
                    cursor = cursor + 1
                    while tokens[cursor].type ~= "rparen" do
                        for _,i in ipairs(parseVariable()) do
                            table.insert(args,i)
                        end
                    end
                    cursor = cursor + 1
                end
                local ret = false
                if tokens[cursor].type == "colon" then
                    cursor = cursor + 1
                    ret = parseType()
                    cursor = cursor + 1
                end
                expectToken("semicolon")
                cursor = cursor + 1
                local vars = {}
                if tokens[cursor].type == "varKw" then
                    cursor = cursor + 1
                    while tokens[cursor].type ~= "beginKw" do
                        for _,i in ipairs(parseVariable()) do
                            table.insert(vars,i)
                        end
                    end
                end
                expectToken("beginKw")
                cursor = cursor + 1
                local code = {}
                while not (tokens[cursor].type == "endKw" and tokens[cursor+1].txt == procName and tokens[cursor+2].type == "semicolon") do
                    if tokens[cursor].type == "identifier" then
                        table.insert(code,shunt())
                    else
                        parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Unexpected Token: \""..tokens[cursor].type.."\"")
                    end
                    cursor = cursor + 1
                end
                cursor = cursor + 3
                table.insert(module[6],{"procedure",procName,args,ret,vars,code})
            else
                parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Unexpected Token: \""..tokens[cursor].type.."\"")
            end
        end
        table.insert(tree,module)
        cursor = cursor + 3
    end
    print(serialize_table(tree))
end