-- Sign of an expression is determined using the first value

return function(tree)
    local ircode = {{},{},{}}
    local savedReg = {}
    local ifCount = 0
    local whileCount = 0
    local strings = {}
    local strCount = 0
    local lastSym = nil
    local lastType = nil
    local function ralloc()
        local i = 0
        while true do
            if not savedReg[i] then
                savedReg[i] = true
                return {"saved",i}
            end
            i = i + 1
        end
    end
    local function rfree(r)
        savedReg[r[2]] = false
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
    local function getProcVars(proc)
        local t = {}
        for _,i in ipairs(proc[3]) do
            table.insert(t,{"var",table.unpack(i,1,#i)})
        end
        for _,i in ipairs(proc[5]) do
            table.insert(t,i)
        end
        return t
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
        local tab = {table.unpack(imports),mod[2]}
        for _,i in ipairs(tab) do
            local val = findProcedure(findModule(i),name)
            if val ~= nil then
                return val
            end
        end
        irgenErr(mod[2],"Undefined Procedure \""..name.."\" (hint: use IMPORT to use procedures from other modules)")
    end
    local function getType(mod,imports,typ)
        local tab = {table.unpack(imports),mod[2]}
        for _,i in ipairs(tab) do
            local m = findModule(i)
            for _,j in ipairs(m[4]) do
                if j[1] == typ then
                    return j[2]
                end
            end
        end
        irgenErr(mod[2],"Undefined Type \""..typ.."\" (hint: use IMPORT to use types from other modules)")
    end
    local function getVarType(mod,imports,loc,var)
        for _,i in ipairs(loc) do
            if i[2] == var then
                local ret = i[3]
                while ret[1] == "customType" do ret = getType(mod,imports,ret[2]) end
                return ret
            end
        end
        local tab = {table.unpack(imports),mod[2]}
        for _,i in ipairs(tab) do
            local m = findModule(i)
            for _,j in ipairs(m[5]) do
                if j[1] == "var" and j[2] == var then
                    local ret = j[3]
                    while ret[1] == "customType" do ret = getType(mod,imports,ret[2]) end
                    return ret
                end
            end
        end
        irgenErr(mod[2],"Undefined Variable \""..var.."\" (hint: use IMPORT to use variables from other modules)")
    end
    local function getLoadType(typ)
        if typ[1] == "numType" then
            if typ[2] == 1 and typ[3] then return "LoadSignedByte"
            elseif typ[2] == 1 and not typ[3] then return "LoadByte"
            elseif typ[2] == 2 and typ[3] then return "LoadSignedHalf"
            elseif typ[2] == 2 and not typ[3] then return "LoadHalf"
            else return "Load" end
        elseif typ[1] == "ptrOf" then
            return "Load"
        end
    end
    local function assertGlobalVar(mod,imports,name)
        local tab = {table.unpack(imports),mod[2]}
        for _,i in ipairs(tab) do
            local m = findModule(i)
            for _,j in ipairs(m[5]) do
                if j[1] == "var" and j[2] == name then
                    return
                end
            end
        end
        irgenErr(mod[2],"Undefined Variable \""..name.."\" (hint: use IMPORT to use variables from other modules)")
    end
    local function getSize(mod,imports,typ)
        if typ[1] == "numType" then
            return typ[2]
        elseif typ[1] == "ptrOf" then
            return 4
        elseif typ[1] == "array" then
            return typ[2]*getSize(mod,imports,typ[3])
        elseif typ[1] == "record" then
            local offset = 0
            for i,j in ipairs(typ) do
                if i ~= 1 then
                    local s = getSize(mod,imports,j[2])
                    if (offset % math.min(s,4)) > 0 then
                        offset = (offset + (math.min(s,4)-(offset % math.min(s,4)))) + s
                    else
                        offset = offset + s
                    end
                end
            end
            return offset
        elseif typ[1] == "customType" then
            return getSize(mod,imports,getType(mod,imports,typ[2]))
        end
        irgenErr(mod[2],"Bad Type Record: "..serialize_list(typ))
    end
    local function getRecOffset(mod,imports,typ,val)
        if typ[1] ~= "record" then
            irgenErr(mod,"Given type \""..typ[1].."\" is not a record!")
        end
        local offset = 0
        for i,j in ipairs(typ) do
            if i ~= 1 then
                if j[1] == val[2] then
                    local s = getSize(mod,imports,j[2])
                    if (offset % math.min(s,4)) > 0 then
                        offset = (offset + (math.min(s,4)-(offset % math.min(s,4))))
                    end
                    return offset
                else
                    local s = getSize(mod,imports,j[2])
                    if (offset % math.min(s,4)) > 0 then
                        offset = (offset + (math.min(s,4)-(offset % math.min(s,4)))) + s
                    else
                        offset = offset + s
                    end
                end
            end
        end
        irgenErr(mod[2],"Undefined Value \""..val[2].."\" in record \""..typ[1].."\"")
    end
    local function evaluate(mod,proc,varSpace,val,reg,getAddr)
        if val[1] == ":=" then
            if val[2][1] == "symbol" and varSpace[val[2][2]] ~= nil then
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                if lastType[1] == "numType" and lastType[2] == 1 then text("StoreByte",r,varSpace[val[2][2]],{"frame"})
                elseif lastType[1] == "numType" and lastType[2] == 2 then text("StoreHalf",r,varSpace[val[2][2]],{"frame"})
                else
                    text("Store",r,varSpace[val[2][2]],{"frame"})
                end
            else
                local r1 = ralloc()
                evaluate(mod,proc,varSpace,val[3],r1)
                local r2 = ralloc()
                evaluate(mod,proc,varSpace,val[2],r2,true)
                rfree(r2)
                rfree(r1)
                if lastType[1] == "numType" and lastType[2] == 1 then text("StoreByte",r1,0,r2)
                elseif lastType[1] == "numType" and lastType[2] == 2 then text("StoreHalf",r1,0,r2)
                else
                    text("Store",r1,0,r2)
                end
            end
        elseif val[1] == "call" then
            if val[2] == "PTROF" then
                evaluate(mod,proc,varSpace,val[3],reg,true)
            elseif val[2] == "LSH" then
                evaluate(mod,proc,varSpace,val[3],reg)
                if val[3][1] == "number" then
                    text("Lsh",reg,val[4])
                else
                    local r = ralloc()
                    evaluate(mod,proc,varSpace,val[4],r)
                    rfree(r)
                    text("Lsh",reg,r)
                end
            elseif val[2] == "RSH" then
                evaluate(mod,proc,varSpace,val[3],reg)
                if val[3][1] == "number" then
                    text("Rsh",reg,val[4])
                else
                    local r = ralloc()
                    evaluate(mod,proc,varSpace,val[4],r)
                    rfree(r)
                    text("Rsh",reg,r)
                end
            elseif val[2] == "ASH" then
                evaluate(mod,proc,varSpace,val[3],reg)
                if val[3][1] == "number" then
                    text("Ash",reg,val[4])
                else
                    local r = ralloc()
                    evaluate(mod,proc,varSpace,val[4],r)
                    rfree(r)
                    text("Ash",reg,r)
                end
            elseif val[2] == "RETURN" then
                evaluate(mod,proc,varSpace,val[3],{"arg",0},true)
                text("Branch",".Lret")
            else
                text("BeginCall",reg,#val-2)
                local args = #val-2
                for i=1,args do
                    evaluate(mod,proc,varSpace,val[2+i],{"arg",i-1})
                end
                if reg then
                    text("EndCall",val[2],args,reg)
                else
                    text("EndCall",val[2],args)
                end
            end
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
            text("LocalLabel",".Lwhile"..whileCount)
            local r = ralloc()
            evaluate(mod,proc,varSpace,val[2],r)
            rfree(r)
            text("BranchIfNotZero",r,".Lwhile"..whileCount.."_after")
            for _,arg in ipairs(val[3]) do
                evaluate(mod,proc,varSpace,arg)
            end
            text("Branch",".Lwhile"..whileCount)
            text("LocalLabel",".Lwhile"..whileCount.."_after")
            whileCount = whileCount + 1
        elseif val[1] == "+" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Add",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Add",reg,r)
            end
        elseif val[1] == "-" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Sub",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Sub",reg,r)
            end
        elseif val[1] == "*" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Mul",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Mul",reg,r)
            end
        elseif val[1] == "/" or val[1] == "DIV" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Div",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Div",reg,r)
            end
        elseif val[1] == "MOD" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Mod",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Mod",reg,r)
            end
        elseif val[1] == "XOR" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Xor",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Xor",reg,r)
            end
        elseif val[1] == "OR" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Or",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Or",reg,r)
            end
        elseif val[1] == "&" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("And",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("And",reg,r)
            end
        elseif val[1] == "_" then
            evaluate(mod,proc,varSpace,val[2],reg)
            text("Negate",reg)
        elseif val[1] == "=" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Eq",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Eq",reg,r)
            end
        elseif val[1] == "#" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Neq",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Neq",reg,r)
            end
        elseif val[1] == ">" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Gt",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Gt",reg,r)
            end
        elseif val[1] == "<" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Lt",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Lt",reg,r)
            end
        elseif val[1] == ">=" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Ge",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Ge",reg,r)
            end
        elseif val[1] == "<=" then
            evaluate(mod,proc,varSpace,val[2],reg)
            if val[3][1] == "number" then
                text("Le",reg,val[3])
            else
                local r = ralloc()
                evaluate(mod,proc,varSpace,val[3],r)
                rfree(r)
                text("Le",reg,r)
            end
        elseif val[1] == "~" then
            evaluate(mod,proc,varSpace,val[2],reg)
            text("Not",reg)
        elseif val[1] == "NOT" then
            evaluate(mod,proc,varSpace,val[2],reg)
            text("Xor",reg,{"number",1})
        elseif val[1] == "." then
            evaluate(mod,proc,varSpace,val[2],reg,true)
            local t = getVarType(mod,mod[3],getProcVars(proc),lastSym[2])
            text("Add",reg,{"number",getRecOffset(mod,mod[3],t,val[3])})
            for _,i in ipairs(t) do
                if type(i) == "table" and i[1] == val[3][2] then
                    lastType = i[2]
                end
            end
            if not getAddr then
                text("Load",reg,0,reg)
            end
        elseif val[1] == "[" then
            evaluate(mod,proc,varSpace,val[2],reg,true)
            if lastType[1] ~= "array" and lastType[1] ~= "ptrOf" then
                irgenErr(mod[2],"Attempted to index a value that wasn't an array or a pointer!")
            end
            local nodeSize = 0
            local nodeType = nil
            if lastType[1] == "ptrOf" then
                nodeSize = getSize(mod,mod[3],lastType[2])
                nodeType = lastType[2]
            else
                nodeSize = getSize(mod,mod[3],lastType[3])
                nodeType = lastType[3]
            end
            local r = ralloc()
            evaluate(mod,proc,varSpace,val[3],r,true)
            if nodeSize > 1 then text("Mul",r,{"number",nodeSize}) end
            rfree(r)
            text("Add",reg,r)
            if not getAddr then
                text(getLoadType(nodeType),reg,0,reg)
            end
        elseif val[1] == "^" then
            evaluate(mod,proc,varSpace,val[2],reg,false)
        elseif val[1] == "number" then
            text("LoadImmediate",reg,val[2])
        elseif val[1] == "string" then
            if not strings[val[2]] then
                rodata("__okameronString"..strCount,val[2])
                string[val[2]] = strCount
                strCount = strCount + 1
            end
            text("LoadAddr","__okameronString"..strings[val[2]])
        elseif val[1] == "symbol" then
            lastSym = val
            if varSpace[val[2]] ~= nil then
                if getAddr then
                    text("Move",{"frame"},reg)
                    text("Add",reg,{"number",varSpace[val[2]]})
                else
                    lastType = getVarType(mod,mod[3],getProcVars(proc),lastSym[2])
                    text(getLoadType(lastType),reg,varSpace[val[2]],{"frame"})
                end
            else
                assertGlobalVar(mod,mod[3],val[2])
                text("LoadAddr",reg,val[2])
                if not getAddr then
                    lastType = getVarType(mod,mod[3],getProcVars(proc),lastSym[2])
                    text(getLoadType(lastType),reg,0,reg)
                end
            end
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
                stackUsage = stackUsage + getSize(mod,mod[3],a[2])
                argUsage = argUsage + 4
            end
            for _,a in ipairs(proc[5]) do
                varSpace[a[2]] = stackUsage
                stackUsage = stackUsage + getSize(mod,mod[3],a[3])
            end
            text("PushVariables",stackUsage-4,argUsage//4)
            for _,a in ipairs(proc[6]) do
                evaluate(mod,proc,varSpace,a)
            end
            text("PopVariables")
            text("PopRet")
            text("Return")
        end
        for _,var in ipairs(mod[5]) do
            if var[1] == "var" then
                bss(var[2],getSize(mod,mod[3],var[3]))
            end
        end
    end
    return ircode
end