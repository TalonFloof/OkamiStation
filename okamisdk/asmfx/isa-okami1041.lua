local isa = {}

isa.name = "okami1041"

isa.bits = 32

isa.alignmask = 0x3

local formats = {}

isa.formats = formats

isa.registers = {
	["zero"] = 0,
	
	["a0"]   = 1,
	["a1"]   = 2,
	["a2"]   = 3,
	["a3"]   = 4,
	["a4"]   = 5,
	["a5"]   = 6,
	["a6"]   = 7,
	["a7"]   = 8,
	
	["t0"]   = 9,
	["t1"]   = 10,
	["t2"]   = 11,
	["t3"]   = 12,
	["t4"]   = 13,
	["t5"]   = 14,
	["t6"]   = 15,
	["t7"]   = 16,
	
	["s0"]   = 17,
	["s1"]   = 18,
	["s2"]   = 19,
	["s3"]   = 20,
	["s4"]  = 21,
	["s5"]  = 22,
	["s6"]  = 23,
	["s7"]  = 24,
	["s8"]  = 25,
	["s9"]  = 26,

    ["gp"] = 27,
	["tp"] = 28,
	["fp"] = 29,
	["sp"] = 30,
	["ra"] = 31
}

isa.controlregisters = {

}

function isa.relocate(sections)
    for k,v in pairs(sections) do
		local data = v.data

		for i,r in ipairs(v.relocations) do
			--[[local nval = r.symbol.bc + r.symbol.section.origin

			if r.long then
				-- nothing
			elseif r.field == "s" then
				sv32(data, r.offset + 2, nval)
			elseif r.field == "d" then
				if r.format.operandinfo.TT == 8 then
					sv32(data, r.offset + 3, nval)
				elseif r.format.operandinfo.TT == 16 then
					sv32(data, r.offset + 4, nval)
				else
					sv32(data, r.offset + 6, nval)
				end
			else
				error("weird relocation")
			end]]
			for x,y in pairs(r) do
			    print(x,y)
			end
			print("")
		end
	end

	return true
end

local function addFormat(operandinfo, encodingstring, formatstring)
	local format = {}

	local tokens = {}

	format.operandinfo = operandinfo

	format.tokens = tokens

	local tok = ""

	for i = 1, #formatstring do
		local c = formatstring:sub(i,i)

		if c == " " then
			if #tok > 0 then
				tokens[#tokens + 1] = tok
			end

			tok = ""
		else
			tok = tok..c
		end
	end

	if #tok > 0 then
		tokens[#tokens + 1] = tok
	end

	format.bits = #encodingstring

	if band(format.bits, 7) ~= 0 then
		error("format bits isn't multiple of 8")
	end

	format.bytes = format.bits/8

	format.encodingstring = encodingstring

	local encoding = {}

	format.encoding = encoding

	local field = 0
	local fbits = 0

	local fn
	local fnbits = 0

	for i = 1, #encodingstring do
		local c = encodingstring:sub(i,i)

		if (c == "0") or (c == "1") then
			if fn then
				local et = {}
				et.bits = fnbits
				et.field = fn
				et.operand = operandinfo[fn]

				encoding[#encoding + 1] = et

				fn = nil
				fnbits = 0
			end

			fbits = fbits + 1

			field = field * 2 + tonumber(c)
		else
			if fbits > 0 then
				local et = {}
				et.bits = fbits
				et.field = field

				encoding[#encoding + 1] = et

				fbits = 0
				field = 0
			end

			if not fn then
				fn = c
				fnbits = 1
			elseif fn ~= c then
				local et = {}
				et.bits = fnbits
				et.field = fn
				et.operand = operandinfo[fn]

				encoding[#encoding + 1] = et

				fn = c
				fnbits = 1
			else
				fnbits = fnbits + 1
			end
		end
	end

	if fbits > 0 then
		local et = {}
		et.bits = fbits
		et.field = field

		encoding[#encoding + 1] = et
	elseif fnbits > 0 then
		local et = {}
		et.bits = fnbits
		et.field = fn
		et.operand = operandinfo[fn]

		encoding[#encoding + 1] = et
	end

	for i = 1, #encoding do
		local e = encoding[i]

		if type(e.field) ~= "number" then
			local q = e.operand

			if not q then
				q = {}
				operandinfo[e.field] = q
				e.operand = q
			end

			q.bits = (q.bits or 0) + e.bits

			local shift = q.shift or 0

			if operandinfo[e.field].intshift then
				q.max = 0xFFFFFFFF
			elseif q.bits == 32 then
				q.max = 0xFFFFFFFF
			else
				q.max = lshift(lshift(1, q.bits)-1, shift)
			end

			if q.max < 0 then
				q.max = 0xFFFFFFFF
			end
		end
	end

	formats[#formats + 1] = format
end

-- INSTRUCTIONS

addFormat(
	{},
	"000000bbbbbaaaaaddddd00000000000", -- add rd, rs1, rs2
	"add ^rd ^ra ^rb"
)
addFormat(
	{},
	"000001bbbbbaaaaaddddd00000000000", -- sub rd, rs1, rs2
	"sub ^rd ^ra ^rb"
)
addFormat(
	{},
	"000010bbbbbaaaaaddddd00000000000", -- and rd, rs1, rs2
	"and ^rd ^ra ^rb"
)
addFormat(
	{},
	"000011bbbbbaaaaaddddd00000000000", -- or rd, rs1, rs2
	"or ^rd ^ra ^rb"
)
addFormat(
	{},
	"000100bbbbbaaaaaddddd00000000000", -- xor rd, rs1, rs2
	"xor ^rd ^ra ^rb"
)
addFormat(
	{},
	"000101bbbbbaaaaaddddd00000000000", -- sll rd, rs1, rs2
	"sll ^rd ^ra ^rb"
)
addFormat(
	{},
	"000110bbbbbaaaaaddddd00000000000", -- srl rd, rs1, rs2
	"srl ^rd ^ra ^rb"
)
addFormat(
	{},
	"000110bbbbbaaaaaddddd10000000000", -- sra rd, rs1, rs2
	"sra ^rd ^ra ^rb"
)
addFormat(
	{},
	"000111bbbbbaaaaaddddd00000000000", -- slt rd, rs1, rs2
	"slt ^rd ^ra ^rb"
)
addFormat(
	{},
	"001000bbbbbaaaaaddddd00000000000", -- sltu rd, rs1, rs2
	"sltu ^rd ^ra ^rb"
)
addFormat(
	{},
	"001001bbbbbaaaaauuuuuddddd000000", -- mul rd, ru, rs1, rs2
	"mul ^rd ^ru ^ra ^rb"
)
addFormat(
	{},
	"001001bbbbbaaaaauuuuuddddd000001", -- mulu rd, ru, rs1, rs2
	"mulu ^rd ^ru ^ra ^rb"
)
addFormat(
	{},
	"001010bbbbbaaaaauuuuuddddd000000", -- divu rd, ru, rs1, rs2
	"div ^rd ^ru ^ra ^rb"
)
addFormat(
	{},
	"001010bbbbbaaaaauuuuuddddd000001", -- divu rd, ru, rs1, rs2
	"divu ^rd ^ru ^ra ^rb"
)

addFormat(
	{},
	"010000sssssdddddiiiiiiiiiiiiiiii", -- addi rd, rs, const
	"addi ^rd ^rs ^ni"
)
addFormat(
	{},
	"010001sssssdddddiiiiiiiiiiiiiiii", -- andi rd, rs, const
	"andi ^rd ^rs ^ni"
)
addFormat(
	{},
	"010010sssssdddddiiiiiiiiiiiiiiii", -- ori rd, rs, const
	"ori ^rd ^rs ^ni"
)
addFormat(
	{},
	"010011sssssdddddiiiiiiiiiiiiiiii", -- xori rd, rs, const
	"xori ^rd ^rs ^ni"
)
addFormat(
	{},
	"010100sssssdddddiiiiiiiiiiiiiiii", -- slli rd, rs, const
	"slli ^rd ^rs ^ni"
)
addFormat(
	{},
	"010101sssssddddd00000000000iiiii", -- srli rd, rs, const
	"srli ^rd ^rs ^ni"
)
addFormat(
	{},
	"010101sssssddddd10000000000iiiii", -- srai rd, rs, const
	"srai ^rd ^rs ^ni"
)
addFormat(
	{},
	"010110sssssdddddiiiiiiiiiiiiiiii", -- slti rd, rs, const
	"slti ^rd ^rs ^ni"
)
addFormat(
	{},
	"010111sssssdddddiiiiiiiiiiiiiiii", -- sltiu rd, rs, const
	"sltiu ^rd ^rs ^ni"
)
addFormat(
	{},
	"01011000000dddddiiiiiiiiiiiiiiii", -- lui rd, const
	"lui ^rd ^ni"
)
addFormat(
	{},
	"01011100000dddddiiiiiiiiiiiiiiii", -- aupc rd, const
	"aupc ^rd ^ni"
)

addFormat(
	{
	    ["j"] = {
			mask=0x3,
			shift=2,
			relative=true
		}
	},
	"100000dddddjjjjjjjjjjjjjjjjjjjjj", -- bl rd, offset
	"bl ^rd ^nj"
)
addFormat(
	{},
	"100001dddddsssss0000000000000000", -- blr rd, rs
	"blr ^rd ^rs"
)
addFormat(
	{
	    ["j"] = {
			mask=0x3,
			shift=2,
			relative=true
		}
	},
	"100010bbbbbaaaaajjjjjjjjjjjjjjjj", -- beq rs1, rs2, offset
	"beq ^ra ^rb ^nj"
)
addFormat(
	{
	    ["j"] = {
			mask=0x3,
			shift=2,
			relative=true
		}
	},
	"100011bbbbbaaaaajjjjjjjjjjjjjjjj", -- bne rs1, rs2, offset
	"bne ^ra ^rb ^nj"
)
addFormat(
	{
	    ["j"] = {
			mask=0x3,
			shift=2,
			relative=true
		}
	},
	"100100bbbbbaaaaajjjjjjjjjjjjjjjj", -- bge rs1, rs2, offset
	"bge ^ra ^rb ^nj"
)
addFormat(
	{
	    ["j"] = {
			mask=0x3,
			shift=2,
			relative=true
		}
	},
	"100101bbbbbaaaaajjjjjjjjjjjjjjjj", -- blt rs1, rs2, offset
	"blt ^ra ^rb ^nj"
)
addFormat(
	{
	    ["j"] = {
			mask=0x3,
			shift=2,
			relative=true
		}
	},
	"100110bbbbbaaaaajjjjjjjjjjjjjjjj", -- bgeu rs1, rs2, offset
	"bgeu ^ra ^rb ^nj"
)
addFormat(
	{
	    ["j"] = {
			mask=0x3,
			shift=2,
			relative=true
		}
	},
	"100111bbbbbaaaaajjjjjjjjjjjjjjjj", -- bltu rs1, rs2, offset
	"bltu ^ra ^rb ^nj"
)

addFormat(
	{},
	"110000dddddsssssiiiiiiiiiiiiiiii", -- lb rd, const(rs)
	"lb ^rd ^ni(^rs)"
)
addFormat(
	{},
	"110001dddddsssssiiiiiiiiiiiiiiii", -- lbu rd, const(rs)
	"lbu ^rd ^ni(^rs)"
)
addFormat(
	{},
	"110010dddddsssssiiiiiiiiiiiiiiii", -- lh rd, const(rs)
	"lh ^rd ^ni(^rs)"
)
addFormat(
	{},
	"110011dddddsssssiiiiiiiiiiiiiiii", -- lhb rd, const(rs)
	"lhb ^rd ^ni(^rs)"
)
addFormat(
	{},
	"110100dddddsssssiiiiiiiiiiiiiiii", -- lw rd, const(rs)
	"lw ^rd ^ni(^rs)"
)
addFormat(
	{},
	"110101dddddsssssiiiiiiiiiiiiiiii", -- sb rd, const(rs)
	"sb ^rd ^ni(^rs)"
)
addFormat(
	{},
	"110110dddddsssssiiiiiiiiiiiiiiii", -- sh rd, const(rs)
	"sh ^rd ^ni(^rs)"
)
addFormat(
	{},
	"110111dddddsssssiiiiiiiiiiiiiiii", -- sw rd, const(rs)
	"sw ^rd ^ni(^rs)"
)

addFormat(
	{},
	"111100ddddd00000iiiiiiiiiiiiiiii", -- mfex rd, regnum
	"mfex ^rd ^ni"
)
addFormat(
	{},
	"111101ddddd00000iiiiiiiiiiiiiiii", -- mtex rd, regnum
	"mtex ^rs ^ni"
)
addFormat(
	{},
	"111110iiiiiiiiiiiiiiiiiiiiiiiiii", -- kcall code
	"kcall ^ni"
)
addFormat(
	{},
	"11111100000000000000000000000000", -- rft
	"rft"
)

-- PSEUDO INSTRUCTIONS
addFormat(
	{
	    ["j"] = {
			mask=0x3,
			shift=2,
			relative=true
		}
	},
	"10000000000jjjjjjjjjjjjjjjjjjjjj", -- bl zero, offset
	"b ^nj"
)
addFormat(
	{},
	"10000100000sssss0000000000000000", -- blr zero, rs
	"br ^rs"
)
addFormat(
    {},
    "01000000000dddddiiiiiiiiiiiiiiii", -- addi rd, zero, const
    "li ^rd ^ni"
)
addFormat(
	{
	    ["i"] = {
			intswap=true,
		},
		["d"] = {
			repeatbits=2,
			repeatbitsby=5,
		}
	},
	"01011000000dddddiiiiiiiiiiiiiiii01001000000dddddiiiiiiiiiiiiiiii", -- lui rd, (const >>> 16); ori rd, zero, const
	"la ^rd ^ni"
)

return isa
