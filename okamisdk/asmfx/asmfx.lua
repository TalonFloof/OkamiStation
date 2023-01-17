local function getdirectory(p)
	for i = #p, 1, -1 do
		if p:sub(i,i) == "/" then
			return p:sub(1,i)
		end
	end

	return "./"
end
local sd = getdirectory(arg[0])

dofile(sd.."misc.lua")

local asm = dofile(sd.."assembler.lua")

-- asmfx.lua [source] [dest]
-- tested under lua 5.1

local function printhelp()
	print("== asmfx.lua ==")
	print("retargetable assembler")
	print("usage: asmfx.lua [source] [dest]")
end

local target = "xr17032"

local format = "xloff"

for k,v in pairs(arg) do
	if v:sub(1,7) == "target=" then
		target = v:sub(8)
		table.remove(arg, k)
	end
end

for k,v in pairs(arg) do
	if v:sub(1,7) == "format=" then
		format = v:sub(8)
		table.remove(arg, k)
	end
end

if (#arg < 2) or (math.floor(#arg/2) ~= #arg/2) then
	print("asm: argument mismatch")
	printhelp()
	os.exit(1)
end

local encoder = dofile(sd.."format-"..format..".lua")

local isa = dofile(sd.."isa-"..target..".lua")

for i = 1, #arg/2 do
	local source = arg[i]
	local dest = arg[#arg/2 + i]

	local srcf = io.open(source, "r")

	if not srcf then
		print(string.format("asm: error opening source file %s", source))
		os.exit(1)
	end

	local sections, symbols, sectionsbyid = asm.assemble(srcf:read("*a"), source, isa, encoder)

	if not sections then
		print("asm: couldn't assemble "..source.."!")
		os.exit(1)
	end

	local binary = encoder.encode(sections, symbols, isa, sectionsbyid)

	if not binary then
		print("asm: couldn't encode "..source.."!")
		os.exit(1)
	else
		destf = io.open(dest, "w")

		if not destf then
			print(string.format("asm: error opening destination file %s", dest))
			os.exit(1)
		end

		destf:write(binary)
		return true
	end
end
