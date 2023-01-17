local format = {}

format.name = "xloff"

local XLOFFMAGIC = 0x99584F46

local xloffheader_s = struct {
	{4, "Magic"},
	{4, "SymbolTableOffset"},
	{4, "SymbolCount"},
	{4, "StringTableOffset"},
	{4, "StringTableSize"},
	{4, "TargetArchitecture"},
	{4, "EntrySymbol"},
	{4, "Flags"},
	{4, "Timestamp"},
	{4, "SectionTableOffset"},
	{4, "SectionCount"},
	{4, "ImportTableOffset"},
	{4, "ImportCount"},
	{4, "HeadLength"},
}

local sectionheader_s = struct {
	{4, "NameOffset"},
	{4, "DataOffset"},
	{4, "DataSize"},
	{4, "VirtualAddress"},
	{4, "RelocTableOffset"},
	{4, "RelocCount"},
	{4, "Flags"}
}

local symbol_s = struct {
	{4, "NameOffset"},
	{4, "Value"},
	{2, "SectionIndex"},
	{1, "Type"},
	{1, "Flags"}
}

local import_s = struct {
	{4, "NameOffset"},
	{4, "ExpectedTimestamp"},
	{4, "ExpectedBase"},
	{4, "FixupTableOffset"},
	{4, "FixupCount"}
}

local reloc_s = struct {
	{4, "Offset"},
	{4, "SymbolIndex"},
	{2, "RelocType"},
	{2, "SectionIndex"}
}

local isas = {
	["xr17032"] = {
		archid = 1,
		align = 4,
	},
	["fox32"] = {
		archid = 2,
		align = 1,
	},
	["okami1041"] = {
	    archid = 3,
	    align = 4,
	},
}

local XLOFFSYMTYPE_GLOBAL  = 1
local XLOFFSYMTYPE_LOCAL   = 2
local XLOFFSYMTYPE_EXTERN  = 3
local XLOFFSYMTYPE_SPECIAL = 4

local XLOFFSECTIONFLAG_BSS   = 1
local XLOFFSECTIONFLAG_DEBUG = 2
local XLOFFSECTIONFLAG_TEXT  = 4
local XLOFFSECTIONFLAG_MAP   = 8
local XLOFFSECTIONFLAG_READONLY = 16

local symbolnames = {
	["global"]  = XLOFFSYMTYPE_GLOBAL,
	["local"]   = XLOFFSYMTYPE_LOCAL,
	["extern"]  = XLOFFSYMTYPE_EXTERN,
	["special"] = XLOFFSYMTYPE_SPECIAL,
}

function format.encode(sections, symbols, isa, sectionsbyid)
	local arch = isas[isa.name]

	if not arch then
		print("asm: format-xloff: XLOFF encoding doesn't support '"..isa.name.."' yet")
		return false
	end

	local isectiontable = {}
	local sectioncount = 0

	local data = {}

	local headertab = {}

	local header = cast(xloffheader_s, headertab)

	local binary = ""

	local function addTab(tab, size)
		if size == 0 then return end

		for i = 0, size-1 do
			binary = binary .. string.char(tab[i])
		end
	end

	local stringtab = {}
	local stringtaboff = 0

	local function addString(str)
		local off = stringtaboff

		for i = 1, #str do
			stringtab[stringtaboff] = string.byte(str:sub(i,i))
			stringtaboff = stringtaboff + 1
		end

		stringtab[stringtaboff] = 0
		stringtaboff = stringtaboff + 1

		return off
	end

	for k,v in ipairs(sectionsbyid) do
		v.id = sectioncount
		isectiontable[sectioncount] = v

		sectioncount = sectioncount + 1

		local flags = 0

		local name = v.name

		if name:sub(1,2) == "ro" then
			flags = bor(flags, XLOFFSECTIONFLAG_READONLY)
		end

		if name:sub(-4,-1) == "text" then
			flags = bor(flags, XLOFFSECTIONFLAG_TEXT)
			flags = bor(flags, XLOFFSECTIONFLAG_MAP)
		elseif name:sub(-4,-1) == "data" then
			flags = bor(flags, XLOFFSECTIONFLAG_MAP)
		elseif name:sub(-5,-1) == "debug" then
			flags = bor(flags, XLOFFSECTIONFLAG_DEBUG)
		elseif v.bss then
			flags = bor(flags, XLOFFSECTIONFLAG_BSS)
			flags = bor(flags, XLOFFSECTIONFLAG_MAP)
		end

		v.flags = flags

		v.reloctable = {}
		v.reloctableoff = 0
		v.reloctableindex = 0

		v.header = {}
		v.sheader = cast(sectionheader_s, v.header)

		while band(v.bc, 3) ~= 0 do
			if not v.bss then
				v.data[v.bc] = 0
			end

			v.bc = v.bc + 1
		end

		v.nameoff = addString(name)
	end

	local symtab = {}
	local symtaboff = 0
	local symtabindex = 0

	local function addSymbol(symbol)
		local off = symtabindex

		local nameoff = 0xFFFFFFFF

		if symbol.name then
			if (symbol.type ~= "local") then
				nameoff = addString(symbol.name)
			end
		end

		local typid = symbolnames[symbol.type]

		if not typid then
			print("asm: format-xloff: XLOFF encoding doesn't support symbols of type '"..symbol.type.."'")
			return false
		end

		local sym = cast(symbol_s, symtab, symtaboff)

		local sid

		if symbol.section then
			sid = symbol.section.id
		else
			sid = -1
		end

		sym.sv("NameOffset", nameoff)
		sym.sv("Value", symbol.bc)
		sym.sv("SectionIndex", sid)
		sym.sv("Type", typid)
		sym.sv("Flags", 0)

		symtabindex = symtabindex + 1
		symtaboff = symtaboff + symbol_s.size()

		return off
	end

	local entrySymbol = 0xFFFFFFFF

	for k,v in pairs(symbols) do
		if (v.type ~= "extern") or (v.erefs > 0) then
			for k2, v2 in pairs(v.locallabels) do
				if v2.rrefs > 0 then
					v2.index = addSymbol(v2)

					if not v2.index then return false end
				end
			end

			if (v.type ~= "local") or (v.rrefs > 0) then
				v.index = addSymbol(v)

				if not v.index then return false end

				if v.entry then
					entrySymbol = v.index
				end
			end
		end
	end

	while band(stringtaboff, 3) ~= 0 do
		stringtab[stringtaboff] = 0
		stringtaboff = stringtaboff + 1
	end

	local filoff = xloffheader_s.size()
	local symtabfiloff = filoff

	filoff = filoff + symtaboff
	local stringtabfiloff = filoff

	filoff = filoff + stringtaboff

	local function addRelocation(section, symbol, offset, rtype)
		local off = section.reloctableindex

		local reloc = cast(reloc_s, section.reloctable, section.reloctableoff)

		if not symbol.index then
			print("asm: format-xloff: strange relocation")
			return false
		end

		reloc.sv("Offset", offset)
		reloc.sv("SymbolIndex", symbol.index)
		reloc.sv("RelocType", rtype)
		reloc.sv("SectionIndex", section.id)

		section.reloctableindex = section.reloctableindex + 1
		section.reloctableoff = section.reloctableoff + reloc_s.size()

		return off
	end

	-- add all the relocations

	for i = 0, sectioncount-1 do
		local section = isectiontable[i]

		for i,r in ipairs(section.relocations) do
			local rt = isa.reloctype(format, r)

			if not rt then return false end

			if not addRelocation(section, r.symbol, r.offset, rt) then return false end
		end

		local shdr = section.sheader

		shdr.sv("RelocTableOffset", filoff)
		shdr.sv("RelocCount", section.reloctableindex)
		shdr.sv("VirtualAddress", section.origin)
		shdr.sv("NameOffset", section.nameoff)
		shdr.sv("Flags", section.flags)

		filoff = filoff + section.reloctableoff
	end

	local sectionhdrfiloff = filoff
	filoff = filoff + sectionheader_s.size()*sectioncount

	local headlength = filoff

	for i = 0, sectioncount-1 do
		local section = isectiontable[i]

		local shdr = section.sheader

		if section.bss then
			shdr.sv("DataOffset", 0)
		else
			shdr.sv("DataOffset", filoff)
		end

		shdr.sv("DataSize", section.bc)

		if not section.bss then
			filoff = filoff + section.bc
		end
	end

	for i = 0, xloffheader_s.size()-1 do
		headertab[i] = 0
	end

	header.sv("Magic", XLOFFMAGIC)

	header.sv("SymbolTableOffset", symtabfiloff)
	header.sv("SymbolCount", symtabindex)

	header.sv("StringTableOffset", stringtabfiloff)
	header.sv("StringTableSize", stringtaboff)

	header.sv("TargetArchitecture", arch.archid)

	header.sv("EntrySymbol", entrySymbol)

	header.sv("Flags", 0)

	header.sv("Timestamp", 0)

	header.sv("SectionTableOffset", sectionhdrfiloff)
	header.sv("SectionCount", sectioncount)

	header.sv("ImportTableOffset", 0)
	header.sv("ImportCount", 0)

	header.sv("HeadLength", headlength)

	addTab(headertab, xloffheader_s.size())
	addTab(symtab, symtaboff)
	addTab(stringtab, stringtaboff)

	for i = 0, sectioncount-1 do
		local section = isectiontable[i]

		addTab(section.reloctable, section.reloctableoff)
	end

	for i = 0, sectioncount-1 do
		local section = isectiontable[i]

		addTab(section.header, sectionheader_s.size())
	end

	for i = 0, sectioncount-1 do
		local section = isectiontable[i]

		if not section.bss then
			addTab(section.data, section.bc)
		end
	end

	return binary
end

return format
