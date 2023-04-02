return function(tokens)
    local cursor = 1;
    local function shunt()
        local NONE = 0
        local LEFT = 1
        local RIGHT = 2
        local finalInsns = {}
        local opTable = {
            {"(",0,NONE,false},
            {")",0,NONE,false},
            {"+",2,LEFT,false},
            {"-",2,LEFT,false},
            {"-",4,RIGHT,true},
            {"*",1,LEFT,false},
            {"/",1,LEFT,false},
            {"DIV",1,LEFT,false},
            {"MOD",1,LEFT,false},
            {"&",1,LEFT,false},
            {"XOR",2,LEFT,false},
            {"OR",2,LEFT,false},
            {"~",4,RIGHT,true},
            {"=",3,LEFT,false},
            {"#",3,LEFT,false},
            {"<",3,LEFT,false},
            {">",3,LEFT,false},
            {"<=",3,LEFT,false},
            {">=",3,LEFT,false},
        }
        local opStack = {}
        local outStack = {}
        local function popOp() {
            local val = opStack[#opStack]
            table.remove(opStack,#opStack)
            return val
        }
        local function popNum() {
            local val = outStack[#outStack]
            table.remove(outStack,#outStack)
            return val
        }
        while tokens[cursor].type != "semicolon" do
            if tokens[cursor].type == "lparen" then
                table.insert(opStack,{"(",0,NONE,false})
            elseif tokens[cursor].type == "comma" then
                while #opStack > 0 and opStack[#opStack][1] != "(" do
                    local pop = popOp()
                    table.insert(outStack,pop)
                end
            elseif tokens[cursor].type == "rparen" then
                while #opStack > 0 and opStack[#opStack][1] != "(" do
                    local pop = popOp()
                    table.insert(outStack,pop)
                end
                popOp()
            elseif tokens[cursor].type == "identifier" and tokens[cursor+1].type == "lparen" then
                table.insert(opStack,tokens[cursor].txt)
            elseif tokens[cursor].type == "identifier" then
                table.insert(opStack,tokens[cursor].txt)
            elseif tokens[cursor].type == "number" then
                if string.sub(tokens[cursor].txt,#tokens[cursor].txt,#tokens[cursor].txt) == "H" then
                    table.insert(numStack,load("return 0x"..string.sub(tokens[cursor].txt,1,#tokens[cursor].txt-1))())
                else
                    table.insert(numStack,load("return "..string.sub(tokens[cursor].txt,1,#tokens[cursor].txt-1))())
                end
            else
                for _,i in ipairs(opTable) do
                    if i[1] == tokens[cursor].txt then
                        
                    end
                end
            end
            cursor = cursor + 1
        end
    end
end