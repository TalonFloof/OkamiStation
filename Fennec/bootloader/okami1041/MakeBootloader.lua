local function check(success,_,num)
    if not success then
        io.stderr:write("Failed to run command (status code: "..num..")!\n")
        os.exit(num)
    end
end

check(os.execute("../../../okamisdk/okas Bootstrap.s Bootloader.o"))
check(os.execute("../../../okamisdk/okrotool reloc -noalign 0x80010000 0 Bootloader.o"))
check(os.execute("../../../okamisdk/okrotool dump Bootloader.o Bootloader.bin"))
local file = io.open("Bootloader.bin","rb")
local data = file:read("*all")
file:close()
local outfile = io.open("Bootloader.bin","wb")
outfile:write("OkmiBRcd")
outfile:write("FENNEC"..string.rep("\0",10))
outfile:write(string.pack("<I4I4", 2, math.ceil(#data / 512)))
outfile:write(string.rep("\0",512))
outfile:write(string.rep("\0",480)) -- Padding
outfile:write(data)
if (#data % 512) > 0 then
    outfile:write(string.rep("\0",512-(#data%512)))
end
outfile:close()