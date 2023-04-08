return function(ir,asm)
    io.stdout:write(asm)
    io.stdout:write(".text\n")
    
    local savedRegs = -1
    local varSpace = 0
    local cursor = 1
    local callDepth = 0

    local function getSavedRegs(start)
        local i = start
        local highest = -1
        while ir[1][i][1] ~= "Return" do
            for _,val in ipairs(ir[1][i]) do
                 if type(val) == "table" and val[1] == "saved" and val[2] > highest then
                    highest = val[2]
                 end
            end
            i = i + 1
        end
        return highest+1
    end
    local function getReg(t)
        if t[1] == "saved" then
            return "s"..t[2]
        elseif t[1] == "arg" then
            return "a"..t[2]
        elseif t[1] == "frame" then
            return "fp"
        end
    end
    local function loadImm(r,i)
        if i & 0xFFFF == 0 then
            io.stdout:write("    lui "..r..", "..i.."\n")
        elseif i & 0xFFFF0000 == 0 then
            io.stdout:write("    li "..r..", "..i.."\n")
        else
            io.stdout:write("    la "..r..", "..i.."\n")
        end
    end
    local ops = {
        ["DefSymbol"]=function(name)
            io.stdout:write(".global "..name..":\n")
            savedRegs = getSavedRegs(cursor)
        end,
        ["LocalLabel"]=function(name)
            io.stdout:write(name..":\n")
        end,
        ["PushRet"]=function()
            io.stdout:write("    addi sp, sp, -"..(8+(savedRegs*4)).."\n")
            for i=savedRegs,1,-1 do
                io.stdout:write("    sw s"..(i-1)..", "..((i*4)+8).."(sp)\n")
            end
            io.stdout:write("    sw ra, 8(sp)\n")
            io.stdout:write("    sw fp, 4(sp)\n")
        end,
        ["PushVariables"]=function(space,args)
            varSpace = space
            io.stdout:write("    addi sp, sp, -"..varSpace.."\n")
            for i=args,1,-1 do
                io.stdout:write("    sw a"..(i-1)..", "..(i*4).."(sp)\n")
            end
            io.stdout:write("    mv sp, fp\n")
        end,
        ["PopVariables"]=function()
            io.stdout:write(".Lret:\n")
            io.stdout:write("    addi sp, sp, "..varSpace.."\n")
        end,
        ["PopRet"]=function()
            for i=savedRegs,1,-1 do
                io.stdout:write("    lw s"..(i-1)..", "..((i*4)+8).."(sp)\n")
            end
            io.stdout:write("    lw ra, 8(sp)\n")
            io.stdout:write("    lw fp, 4(sp)\n")
            io.stdout:write("    addi sp, sp, 8\n")
        end,
        ["Return"]=function()
            io.stdout:write("    br ra\n")
        end,
        ["BeginCall"]=function(r,argCount)
            callDepth = callDepth + 1
            if callDepth > 1 then
                if argCount > 0 and r ~= {"arg",0} then
                    io.stdout:write("    addi sp, sp, -"..(argCount*4).."\n")
                    for i=1,argCount do
                        io.stdout:write("    sw a"..(i-1)..", "..(argCount*4).."(sp)\n")
                    end
                end
            end
        end,
        ["EndCall"]=function(name,argCount,r)
            io.stdout:write("    bl "..name.."\n")
            if r ~= nil then
                io.stdout:write("    mv a0, "..getReg(r).."\n")
            end
            callDepth = callDepth - 1
            if callDepth > 1 then
                if argCount > 0 and r ~= {"arg",0} then
                    for i=1,argCount do
                        io.stdout:write("    lw a"..(i-1)..", "..(argCount*4).."(sp)\n")
                    end
                    io.stdout:write("    addi sp, sp, "..(argCount*4).."\n")
                end
            end
        end,
        ["Move"]=function(r1,r2)
            io.stdout:write("    mv "..getReg(r1)..", "..getReg(r2).."\n")
        end,
        ["Add"]=function(r1,r2)
            if r2[1] == "number" then
                if r2[2] < 32768 then
                    io.stdout:write("    addi "..getReg(r1)..", "..getReg(r1)..", "..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    add "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    add "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Sub"]=function(r1,r2)
            if r2[1] == "number" then
                if r2[2] <= 32768 then
                    io.stdout:write("    addi "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    sub "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    sub "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Mul"]=function(r1,r2,sign)
            if r2[1] == "number" then
                loadImm("t0",r2[2])
                io.stdout:write("    "..(sign and "mul" or "mulu").." "..getReg(r1)..", zero, "..getReg(r1)..", t0\n")
            else
                io.stdout:write("    "..(sign and "mul" or "mulu").." "..getReg(r1)..", zero, "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Div"]=function(r1,r2,sign)
            if r2[1] == "number" then
                loadImm("t0",r2[2])
                io.stdout:write("    "..(sign and "div" or "divu").." "..getReg(r1)..", zero, "..getReg(r1)..", t0\n")
            else
                io.stdout:write("    "..(sign and "div" or "divu").." "..getReg(r1)..", zero, "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Mod"]=function(r1,r2)
            if r2[1] == "number" then
                loadImm("t0",r2[2])
                io.stdout:write("    div zero, "..getReg(r1)..", "..getReg(r1)..", t0\n")
            else
                io.stdout:write("    div zero, "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["And"]=function(r1,r2)
            if r2[1] == "number" then
                if r2[2] < 65536 then
                    io.stdout:write("    andi "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    and "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    and "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Or"]=function(r1,r2)
            if r2[1] == "number" then
                if r2[2] < 65536 then
                    io.stdout:write("    ori "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    or "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    or "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Xor"]=function(r1,r2)
            if r2[1] == "number" then
                if r2[2] < 65536 then
                    io.stdout:write("    xori "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    xor "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    xor "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Lsh"]=function(r1,r2)
            if r2[1] == "number" then
                if r2[2] > 31 then
                    io.stdout:write("    lui "..getReg(r1)..", 0\n")
                elseif r2[2] < 65536 then
                    io.stdout:write("    slli "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    sll "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    sll "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Rsh"]=function(r1,r2)
            if r2[1] == "number" then
                if r2[2] > 31 then
                    io.stdout:write("    lui "..getReg(r1)..", 0\n")
                elseif r2[2] < 65536 then
                    io.stdout:write("    srli "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    srl "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    srl "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Ash"]=function(r1,r2)
            if r2[1] == "number" then
                if r2[2] > 31 then
                    io.stdout:write("    lui "..getReg(r1)..", 0\n")
                elseif r2[2] < 65536 then
                    io.stdout:write("    srai "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    sra "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    sra "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Negate"]=function(r)
            io.stdout:write("    sub "..getReg(r)..", zero, "..getReg(r))
        end,
        ["Not"]=function(r)
            io.stdout:write("    addi t0, zero, -1\n")
            io.stdout:write("    xor "..getReg(r)..", "..getReg(r)..", t0\n")
        end,
        ["Eq"]=function(r1,r2)
            ops["Sub"](r1,r2)
            io.stdout:write("    sltiu "..getReg(r1)..", "..getReg(r1)..", 1\n")
        end,
        ["Neq"]=function(r1,r2)
            ops["Sub"](r1,r2)
            io.stdout:write("    sltu "..getReg(r1)..", zero, "..getReg(r1).."\n")
        end,
        ["Lt"]=function(r1,r2,sign)
            if r2[1] == "number" then
                if r2[2] < 65536 then
                    io.stdout:write("    "..(sign and "slti" or "sltiu").." "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    "..(sign and "slt" or "sltu").." "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    "..(sign and "slt" or "sltu").." "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
        end,
        ["Gt"]=function(r1,r2,sign)
            if r2[1] == "number" then
                loadImm("t0",r2[2])
                io.stdout:write("    "..(sign and "slt" or "sltu").." "..getReg(r1)..", t0, "..getReg(r1).."\n")
            else
                io.stdout:write("    "..(sign and "slt" or "sltu").." "..getReg(r1)..", "..getReg(r2)..", "..getReg(r1).."\n")
            end
        end,
        ["Le"]=function(r1,r2,sign)
            if r2[1] == "number" then
                loadImm("t0",r2[2])
                io.stdout:write("    "..(sign and "slt" or "sltu").." "..getReg(r1)..", t0, "..getReg(r1).."\n")
            else
                io.stdout:write("    "..(sign and "slt" or "sltu").." "..getReg(r1)..", "..getReg(r2)..", "..getReg(r1).."\n")
            end
            io.stdout:write("    xori "..getReg(r1)..", "..getReg(r1)..", 1\n")
        end,
        ["Ge"]=function(r1,r2,sign)
            if r2[1] == "number" then
                if r2[2] < 65536 then
                    io.stdout:write("    "..(sign and "slti" or "sltiu").." "..getReg(r1)..", "..getReg(r1)..", -"..r2[2].."\n")
                else
                    loadImm("t0",r2[2])
                    io.stdout:write("    "..(sign and "slt" or "sltu").." "..getReg(r1)..", "..getReg(r1)..", t0\n")
                end
            else
                io.stdout:write("    "..(sign and "slt" or "sltu").." "..getReg(r1)..", "..getReg(r1)..", "..getReg(r2).."\n")
            end
            io.stdout:write("    xori "..getReg(r1)..", "..getReg(r1)..", 1\n")
        end,
        ["StoreByte"]=function(d,offset,s)
            io.stdout:write("    sb "..getReg(d)..", "..offset.."("..getReg(s)..")\n")
        end,
        ["StoreHalf"]=function(d,offset,s)
            io.stdout:write("    sh "..getReg(d)..", "..offset.."("..getReg(s)..")\n")
        end,
        ["Store"]=function(d,offset,s)
            io.stdout:write("    sw "..getReg(d)..", "..offset.."("..getReg(s)..")\n")
        end,
        ["LoadByte"]=function(d,offset,s)
            io.stdout:write("    lbu "..getReg(d)..", "..offset.."("..getReg(s)..")\n")
        end,
        ["LoadHalf"]=function(d,offset,s)
            io.stdout:write("    lhu "..getReg(d)..", "..offset.."("..getReg(s)..")\n")
        end,
        ["Load"]=function(d,offset,s)
            io.stdout:write("    lw "..getReg(d)..", "..offset.."("..getReg(s)..")\n")
        end,
        ["LoadAddr"]=function(r,val)
            io.stdout:write("    la "..getReg(r)..", "..val.."\n")
        end,
        ["LoadImmediate"]=function(r,val)
            loadImm(getReg(r),val)
        end,
        ["Branch"]=function(l)
            io.stdout:write("    b "..l.."\n")
        end,
        ["BranchIfNotZero"]=function(r,l)
            io.stdout:write("    bne "..getReg(r)..", zero, "..l.."\n")
        end,
    }
    while cursor < #ir[1] do
        local insn = ir[1][cursor]
        if not ops[insn[1]] then
            error("Unknown IR Instruction: "..insn[1])
        else
            ops[insn[1]](table.unpack(insn,2))
        end
        cursor = cursor + 1
    end
    io.stdout:write(".rodata\n")
    for _,i in ipairs(ir[2]) do
        if i[2] == "string" then
            io.stdout:write(i[1]..": .string \""..i[3].."\"\n")
        elseif i[2] == "set" then
            io.stdout:write(i[1]..":\n")
            for _,j in ipairs(i[3]) do
                io.stdout:write("    .word "..j[2].."\n")
            end
        end
    end
    io.stdout:write(".bss\n")
    for _,i in ipairs(ir[3]) do
        io.stdout:write(i[1]..": .resb "..i[2].."\n")
    end
end