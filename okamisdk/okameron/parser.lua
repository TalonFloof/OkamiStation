return function(tokens)
    local cursor = 1;
    local tree = {}
    local asmCode = ""
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
            {".",0,NONE,false},
            {":=",0,NONE,false},
            {"]",0,NONE,false},
            {"(",0,NONE,false},
            {")",0,NONE,false},
            {"+",8,LEFT,false,function(x,y) return x + y end},
            {"-",8,LEFT,false,function(x,y) return x - y end},
            {"_",10,RIGHT,true,function(x) return 0 - x end}, -- Unary -
            {"^",0,NONE,true,function(x) parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Attempt to decast immediate value") end},
            {"**",9,LEFT,false,function(x,y) return x * y end},
            {"*",9,LEFT,false,function(x,y) return x * y end},
            {"//",9,LEFT,false,function(x,y) return x // y end},
            {"/",9,LEFT,false,function(x,y) return x // y end},
            {"DIV",9,LEFT,false,function(x,y) return x // y end},
            {"MOD",9,LEFT,false,function(x,y) return x % y end},
            {"&",9,LEFT,false,function(x,y) return x & y end},
            {"XOR",8,LEFT,false,function(x,y) return x ~ y end},
            {"OR",8,LEFT,false,function(x,y) return x | y end},
            {"~",10,RIGHT,true,function(x) return ~x end},
            {"=",7,LEFT,false,function(x,y) return x == y end},
            {"#",7,LEFT,false,function(x,y) return x ~= y end},
            {"<",7,LEFT,false,function(x,y) return x < y end},
            {">",7,LEFT,false,function(x,y) return x > y end},
            {"<=",7,LEFT,false,function(x,y) return x <= y end},
            {">=",7,LEFT,false,function(x,y) return x >= y end},
            {"<<",7,LEFT,false,function(x,y) return x < y end},
            {">>",7,LEFT,false,function(x,y) return x > y end},
            {"<<=",7,LEFT,false,function(x,y) return x <= y end},
            {">>=",7,LEFT,false,function(x,y) return x >= y end},
        }
        local opStack = {}
        local outStack = {}
        local lastOp = nil
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
            local level = 1
            while level > 0 do
                if tokens[val].type == "lparen" then
                    level = level + 1
                elseif tokens[val].type == "rparen" then
                    level = level - 1
                end
                val = val + 1
            end
            val = val - 1
            return val
        end
        local function addOp(op)
            if op then
                if lastOp then
                    if op[1] == "-" then op=getOp("_")
                    elseif op[1] ~= "(" and op[1] ~= ":=" and not op[4] then
                        parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Illegal use of binary operator \""..op[1].."\"")
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
                    table.insert(outStack,operator)
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
            elseif tokens[cursor].type == "dot" then
                lastOp = {".",0,NONE,false}
            elseif tokens[cursor].type == "caret" then
                table.insert(outStack,{"^",0,NONE,true})
            elseif tokens[cursor].type == "assign" then
                while #opStack > 0  do
                    local pop = popOp()
                    table.insert(outStack,pop)
                end
                addOp({":=",0,NONE,false})
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
                if lastOp ~= nil and lastOp[1] == "." then
                    table.insert(outStack,lastOp)
                end
                lastOp = nil
            elseif tokens[cursor].type == "number" then
                if string.sub(tokens[cursor].txt,#tokens[cursor].txt,#tokens[cursor].txt) == "H" then
                    table.insert(outStack,load("return 0x"..string.sub(tokens[cursor].txt,1,#tokens[cursor].txt-1))())
                else
                    table.insert(outStack,tonumber(tokens[cursor].txt))
                end
                lastOp = nil
            elseif tokens[cursor].type == "string" then
                table.insert(outStack,{"STR",tokens[cursor].txt})
                lastOp = nil
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
                        if n1[1] == "number" then
                            table.insert(tempStack,{"number",i[5](n1[2])})
                        else
                            table.insert(tempStack,{i[1],n1})
                        end
                    else
                        n2 = table.remove(tempStack,#tempStack)
                        if n1[1] == "number" and n2[1] == "number" then
                            table.insert(tempStack,{"number",i[5](n1[2],n2[2])})
                        else
                            table.insert(tempStack,{i[1],n2,n1})
                        end
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
    local function parseType()
        if tokens[cursor].type == "identifier" then
            if tokens[cursor].txt == "CHAR" then
                return {"numType",1}
            elseif tokens[cursor].txt == "SHORT" then
                return {"numType",2}
            elseif tokens[cursor].txt == "INT" or tokens[cursor].txt == "LONG" or tokens[cursor].txt == "PTR" then
                return {"numType",4}
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
    local function parseVariable()
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
    local function parseTypeSection()
        local out = {}
        while tokens[cursor].type == "identifier" do
            local name = tokens[cursor].txt
            cursor = cursor + 1
            expectToken("eq")
            cursor = cursor + 1
            local t = false
            if tokens[cursor].type == "recordKw" then
                t = {"record"}
                cursor = cursor + 1
                while tokens[cursor].type ~= "endKw" do
                    for _,i in ipairs(parseVariable()) do
                        table.insert(t,i)
                    end
                end
                cursor = cursor + 1
            else
                t = parseType()
                cursor = cursor + 1
            end
            expectToken("semicolon")
            cursor = cursor + 1
            table.insert(out,{name,t})
        end
        return out
    end
    local function parseConstSection()
        local out = {}
        while tokens[cursor].type == "identifier" do
            local name = tokens[cursor].txt
            cursor = cursor + 1
            expectToken("eq")
            cursor = cursor + 1
            if tokens[cursor].type == "lbrace" then --Set
                cursor = cursor + 1
                local vals = {}
                while tokens[cursor].type ~= "rbrace" do
                    table.insert(vals,shunt("comma"))
                    cursor = cursor + 1
                end
                cursor = cursor + 1
                expectToken("semicolon")
                cursor = cursor + 1
                table.insert(vals,1,"set")
                table.insert(out,{"const",name,vals})
            elseif tokens[cursor].type == "number" then
                table.insert(out,{"const",name,shunt("semicolon")})
                cursor = cursor + 1
            else
                parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Invalid Assignment for Constant")
            end
        end
        return out
    end
    local function parseCode(delim)
        local code = {}
        local d = delim or {["endKw"]=true}
        while not d[tokens[cursor].type] do
            if tokens[cursor].type == "identifier" then
                table.insert(code,shunt())
                cursor = cursor + 1
            elseif tokens[cursor].type == "whileKw" then
                cursor = cursor + 1
                local expr = shunt("doKw")
                cursor = cursor + 1
                local c = parseCode()
                table.insert(code,{"while",expr,c})
            elseif tokens[cursor].type == "ifKw" then
                cursor = cursor + 1
                local expr = shunt("thenKw")
                cursor = cursor + 1
                local c = parseCode({["endKw"]=true,["elseKw"]=true,["elsifKw"]=true})
                local tab = {"if",{expr,c}}
                while tokens[cursor].type ~= "endKw" do
                    if tokens[cursor].type == "elsifKw" then
                        cursor = cursor + 1
                        expr = shunt("thenKw")
                        c = parseCode({["endKw"]=true,["elseKw"]=true,["elsifKw"]=true})
                        table.insert(tab,{"elseif",expr,c})
                    elseif tokens[cursor].type == "elseKw" then
                        c = parseCode({["endKw"]=true})
                        table.insert(tab,{"else",c})
                        break
                    end
                end
                cursor = cursor + 1
                expectToken("semicolon")
                cursor = cursor + 1
            else
                parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Unexpected Token: \""..tokens[cursor].type.."\"")
            end
        end
        if delim == nil then
            cursor = cursor + 1
            expectToken("semicolon")
            cursor = cursor + 1
        end
        return code
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
        while not (tokens[cursor].type == "endKw" and tokens[cursor+1].type == "dot") do
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
            elseif tokens[cursor].type == "asmKw" then
                cursor = cursor + 1
                expectToken("string")
                local file = io.open((tokens[cursor].txt):sub(2,#tokens[cursor].txt-1),"rb")
                asmCode = asmCode .. file:read("*a")
                file:close()
                cursor = cursor + 1
                expectToken("semicolon")
                cursor = cursor + 1
            elseif tokens[cursor].type == "externKw" then
                cursor = cursor + 1
                local funcs = false
                if tokens[cursor].type == "procedureKw" then
                    funcs = true
                    cursor = cursor + 1
                end
                local out = {"extern",funcs}
                while tokens[cursor].type == "identifier" do
                    for _,i in ipairs(parseVariable()) do
                        table.insert(out,i)
                    end
                end
                table.insert(module[5],out)
            elseif tokens[cursor].type == "typeKw" then
                cursor = cursor + 1
                module[4] = parseTypeSection()
            elseif tokens[cursor].type == "varKw" then
                cursor = cursor + 1
                while tokens[cursor].type == "identifier" do
                    for _,i in ipairs(parseVariable()) do
                        local tab = {"var",table.unpack(i)}
                        table.insert(module[5],tab)
                    end
                end
            elseif tokens[cursor].type == "constKw" then
                cursor = cursor + 1
                local t = parseConstSection()
                for _,i in ipairs(t) do
                    table.insert(module[5],i)
                end
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
                            table.insert(vars,{"var",table.unpack(i)})
                        end
                    end
                end
                expectToken("beginKw")
                cursor = cursor + 1
                local code = parseCode()
                table.insert(module[6],{"procedure",procName,args,ret,vars,code})
            else
                parseErr(tokens[cursor].file,tokens[cursor].line,tokens[cursor].col,"Unexpected Token: \""..tokens[cursor].type.."\"")
            end
        end
        table.insert(tree,module)
        cursor = cursor + 2
    end
    return tree, asmCode
end