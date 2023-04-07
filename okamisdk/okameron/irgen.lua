-- Sign of an expression is determined using the first value

return function(tree)
    local ircode = {{},{},{}}
    local savedReg = {}
    local ifCount = 0
    local whileCount = 0
    local strings = {}
    local function ralloc()
        local i = 0
        while true do
            if not savedReg[i] then
                return {"saved",i}
            end
            i = i + 1
        end
    end
    
    local function text(...)
        table.insert(ircode[1],table.pack(...))
    end

    local function rodata(name,typ,dat)
        table.insert(ircode[2],{name,typ,dat})
    end

    local function bss(name,size)
        table.insert(ircode[3],{name,size})
    end

    local function irgenErr(modname,err)
        io.stderr:write("\x1b[1;31mModule("..modname..") - "..err.."\x1b[0m\n")
        os.exit(4)
    end

    local function findModule(name)
        for _,i in ipairs(tree) do
            if i[1] == "module" and i[2] == name then
                return i
            end
        end
        irgenErr(name,"<-- Attempted to find this nonexistant module")
    end
    local function findProcedure(mod,name)
        for _,i in ipairs(mod[6]) do
            if i[1] == "procedure" and i[2] == name then
                return i
            end
        end
        for _,i in ipairs(mod[5]) do
            if i[1] == "extern" and i[2] then
                for j=3,#i do
                    if i[j][1] == name then
                        return {"extern",table.unpack(i[j])}
                    end
                end
            end
        end
        return nil
    end
    local function getProcedure(mod,imports,name)
        for _,i in ipairs(imports) do
            local val = findProcedure(findModule(i),name)
            if val ~= nil then
                return 
            end
        end
        irgenErr(mod,"Undefined Procedure \""..name.."\" (hint: use IMPORT to use procedures from other modules)")
    end
    local function getType(mod,imports,typ)
        for _,i in ipairs(imports) do
            local mod = findModule(i)
            for _,j in ipairs(mod[4]) do
                if j[1] == typ[2] then
                    return j[2]
                end
            end
        end
        irgenErr(mod,"Undefined Type \""..typ[2].."\" (hint: use IMPORT to use types from other modules)")
    end
    local function getSize(mod,imports,typ)
        if typ[1] == "numType" then
            return typ[2]
        elseif typ[1] == "ptrOf" then
            return 4
        elseif typ[1] == "array" then
            return typ[2]*getSize(typ[3])
        elseif typ[1] == "record" then

        else
            return getSize(mod,imports,getType(mod,imports,typ[2]))
        end
    end
    local function evaluate(mod,proc,varSpace,val,reg)
        if val[1] == ":=" then
            local r1 = ralloc()
            evaluate(mod,proc,varSpace,val[3],r1)
            local r2 = ralloc()
            evaluate(mod,proc,varSpace,val[2],r2)
            rfree(r2)
            rfree(r1)
            text("Store",r2,0,r1)
        elseif val[1] == "call" then
            local args = #val-2
            for i=1,args do
                evaluate(mod,proc,varSpace,val[2+i],{"arg",i})
            end
            text("Call",val[2])
        elseif val[1] == "if" then
            for i=1,#val do
                if val[i][1] ~= "else" then
                    local r = ralloc()
                    evaluate(mod,proc,varSpace,val[i][2],r)
                    text("BranchIfNotZero",r,".Lif"..ifCount.."_"..i)
                    rfree(r)
                end
            end
            if val[#val][1] == "else" then
                text("Branch",".Lif"..ifCount.."_"..(#val))
            else
                text("Branch",".Lif"..ifCount.."_after")
            end
            for i=1,#val do
                text("LocalLabel",".Lif"..ifCount.."_"..i)
                evaluate(mod,proc,varSpace,val[i][3])
                text("Branch",".Lif"..ifCount.."_after")
            end
            text("LocalLabel",".Lif"..ifCount.."_after")
            ifCount = ifCount + 1
        elseif val[1] == "while" then
            
        elseif val[1] == "+" then
            for i,arg in ipairs(val) do
                if i == 2 then
                    evaluate(mod,proc,varSpace,arg,reg)
                elseif i ~= 1 then
                    if arg[1] == "number" then
                        text("Add",reg,arg)
                    else
                        local r = ralloc()
                        evaluate(mod,proc,varSpace,arg,r)
                        rfree(r)
                        text("Add",reg,r)
                    end
                end
            end
        elseif val[1] == "-" then

        elseif val[1] == "*" then

        elseif val[1] == "/" or val[1] == "DIV" then

        elseif val[1] == "MOD" then

        elseif val[1] == "XOR" then

        elseif val[1] == "OR" then

        elseif val[1] == "&" then

        elseif val[1] == "_" then

        elseif val[1] == "=" then

        elseif val[1] == "#" then

        elseif val[1] == ">" then

        elseif val[1] == "<" then

        elseif val[1] == ">=" then

        elseif val[1] == "<=" then

        elseif val[1] == "~" then

        elseif val[1] == "NOT" then

        elseif val[1] == "number" then
            text("LoadImmediate",val[2])
        end
    end

    for _,mod in ipairs(tree) do
        for _,proc in ipairs(mod[6]) do
            text("DefSymbol",proc[2])
            text("PushRet") -- Includes Saved Registers
            local varSpace = {}
            local stackUsage = 4
            local argUsage = 0
            for _,a in ipairs(proc[3]) do
                varSpace[a[1]] = stackUsage
                stackUsage = stackUsage + getSize(mod[2],mod[3],a[2])
                argUsage = argUsage + 4
            end
            for _,a in ipairs(proc[5]) do
                varSpace[a[1]] = stackUsage
                stackUsage = stackUsage + getSize(mod[2],mod[3],a[2])
            end
            text("PushVariables",stackUsage-4,argUsage//4)
            for _,a in ipairs(proc[6]) do
                evaluate(mod,proc,varSpace,a)
            end
            text("PopVariables")
            text("PopRet")
            text("Return")
        end
    end
    print(serialize_list(ircode,true,false))
    return ircode
end