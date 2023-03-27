return function(tokens)
    local cursor = 1

    local operators = {
        -- 0: None, 1: Left, 2: Right
        {">",5,1,false,gtOp},
        {"<",5,1,false,ltOp},
        {">",5,1,false,gtOp},
        {"<",5,1,false,ltOp},
        {"<>",5,1,false,neOp},
        {"=",5,1,false,eqOp},
        {"xor",4,1,false,xorOp},
        {"or",4,1,false,orOp},
        {"-",4,1,false,subOp},
        {"+",4,1,false,addOp},
        {"*",3,1,false,mulOp},
        {"/",3,1,false,divOp},
        {"div",3,1,false,divOp},
        {"mod",3,1,false,modOp},
        {"and",3,1,false,andOp},
        {"shl",3,1,false,shlOp},
        {"shr",3,1,false,shrOp},
        {"not",2,1,true,notOp},
        {"_",1,2,true,negateOp}, -- Symbol is actually: -
        {"(",0,0,false,nil},
        {")",0,0,false,nil}
    }

    local function parseExpression(Start,End)
        local cur = Start
        while cur < End do

        end
    end
    while cursor < #tokens do
        --
    end
end