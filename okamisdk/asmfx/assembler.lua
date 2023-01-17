local function getdirectory(p)
	for i = #p, 1, -1 do
		if p:sub(i,i) == "/" then
			return p:sub(1,i)
		end
	end

	return "./"
end

local asm = {}

local function lerror(line, message)
	print(string.format("asm: %s:%d: %s", line.filename, line.linenumber, message))
end

local constants = {}
local labels = {}

local hexlookup = {
    ["0"] = 0,
    ["1"] = 1,
    ["2"] = 2,
    ["3"] = 3,
    ["4"] = 4,
    ["5"] = 5,
    ["6"] = 6,
    ["7"] = 7,
    ["8"] = 8,
    ["9"] = 9,
    ["A"] = 10,
    ["B"] = 11,
    ["C"] = 12,
    ["D"] = 13,
    ["E"] = 14,
    ["F"] = 15,

    ["a"] = 10,
    ["b"] = 11,
    ["c"] = 12,
    ["d"] = 13,
    ["e"] = 14,
    ["f"] = 15
}

local base

local function match(isa, format, formattok, linetok)
	local place = 1

	-- state:
	--  0 = text match
	--  1 = get format type
	--  2 = get operand name
	--  3 = match operand

	local state = 0

	-- operandtypes:
	--  0 = register
	--  1 = decimal number
	--  2 = hex number
	--  3 = octal number
	--  4 = constant

	local operandtype = 0
	local operandname = true

	local operandvalue

	local neg = false

	local i = 1

	if not linetok then
		return false
	end

	local fc = formattok:sub(i,i)

	while #fc > 0 do
		if state == 0 then -- text match
			if fc == "^" then
				state = 1
			else
				local lc = linetok:sub(place, place)

				if fc ~= lc then
					return false
				end

				place = place + 1
			end

			i = i + 1

			fc = formattok:sub(i,i)
		elseif state == 1 then -- get format type
			if fc == "r" then
				operandtype = 0
			elseif fc == "c" then
				operandtype = 6 -- control register
			elseif fc == "n" then
				operandtype = 1

				neg = false

				if linetok:sub(place, place) == "-" then
					neg = true
					place = place + 1
				end

				if linetok:sub(place, place+1) == "0x" then
					operandtype = 2
					place = place + 2
				elseif linetok:sub(place, place) == "0" then
					operandtype = 3
					place = place + 1
				elseif not tonumber(linetok:sub(place, place)) then
					operandtype = 4
				end
			else
				error("weird operand type in ISA format")
			end

			state = 2

			i = i + 1

			fc = formattok:sub(i,i)
		elseif state == 2 then -- get operand name
			operandname = fc

			i = i + 1

			local operandinfo = format.operandinfo[fc]

			if not operandinfo then
				error("yikes, no operandinfo")
			end

			fc = formattok:sub(i,i)

			local lc = linetok:sub(place, place)

			if (operandtype == 0) or (operandtype == 6) then -- match register name
				local rn = ""

				while #lc > 0 do
					if lc == fc then
						break
					end

					rn = rn..lc

					place = place + 1

					lc = linetok:sub(place, place)
				end

				local chktab

				if operandtype == 0 then
					chktab = isa.registers
				else
					chktab = isa.controlregisters
				end

				if chktab[rn] then
					operandvalue = chktab[rn]
				else
					return false
				end
			elseif (operandtype >= 1) and (operandtype <= 4) then -- match number
				operandvalue = 0

				if operandtype ~= 4 then
					while #lc > 0 do
						if lc == fc then
							break
						end

						if operandtype == 1 then
							local c = tonumber(lc)

							if not c then
								return false
							end

							operandvalue = operandvalue*10 + c
						elseif operandtype == 2 then
							local c = hexlookup[lc]

							if not c then
								return false
							end

							operandvalue = operandvalue*16 + c
						elseif operandvalue == 3 then
							local c = tonumber(lc)

							if not c then
								return false
							end

							if c >= 8 then
								return false
							end

							operandvalue = operandvalue*8 + c
						end

						place = place + 1

						lc = linetok:sub(place, place)
					end
				else
					local cn = ""

					while #lc > 0 do
						if lc == fc then
							break
						end

						cn = cn..lc

						place = place + 1

						lc = linetok:sub(place, place)
					end

					operandvalue = constants[cn]

					if not operandvalue then
						operandvalue = cn
					end
				end

				if type(operandvalue) == "number" then
					if neg then
						operandvalue = -operandvalue
					end

					local mask = operandinfo.mask or 0

					if band(operandvalue, mask) ~= 0 then
						return false
					end

					local max = operandinfo.max or math.huge

					if operandvalue > max then
						print(operandinfo.bits)
						return false
					end
				end
			end

			state = 0
		end
	end

	if state == 0 then
		if fc ~= linetok:sub(place, place) then
			return false
		end
	end

	return operandname, operandvalue
end

function asm.match(isa, line)
	-- matches a line to an instruction mnemonic
	-- returns false if there is no match
	-- otherwise it returns a table with the name and value of the operands

	local formats = isa.formats

	local lbias = 0

	local coperand

	if isa.conditions and isa.conditions[line.tokens[1]] then
		coperand = isa.conditions[line.tokens[1]]
		lbias = 1
	elseif isa.conditions then
		coperand = 0
	end

	for i = 1, #formats do
		local format = formats[i]

		local found = true

		local operands = {}

		if coperand then
			operands["c"] = coperand
		end

		for j = 1, #format.tokens do
			local field, value, rept = match(isa, format, format.tokens[j], line.tokens[j+lbias])

			if not field then
				operands = nil
				break
			end

			if value then
				operands[field] = value
			end
		end

		if operands then
			return format, operands
		end
	end

	return false
end

function asm.tokenize(lines, place, codestring, filename)
	local len = #codestring

	local comment = false

	local instring = false

	local inchar = false

	local char = 0

	local isl

	local escape = false

	local linenow = {}
	linenow.linenumber = 1
	linenow.filename = filename
	linenow.tokens = {}

	local toknow = ""

	local line = 1

	for i = 1, len do
		local c = codestring:sub(i, i)

		if inchar then
			if c == "'" then
				inchar = false

				toknow = tostring(char)
			else
				char = char*0x100 + string.byte(c)
			end
		elseif instring then
			if c == "\n" then
				escape = false
				line = line + 1
				toknow = toknow .. "\n"
			elseif escape then
				if c == "n" then
					toknow = toknow .. "\n"
				elseif c == "t" then
					toknow = toknow .. "\t"
				elseif c == "0" then
					toknow = toknow .. string.char(0)
				else
					toknow = toknow .. c
				end

				escape = false
			elseif c == '"' then
				instring = false
			elseif c == "\\" then
				escape = true
			else
				toknow = toknow .. c
			end
		elseif c == "\n" then
			comment = false

			line = line + 1

			if #toknow > 0 then
				linenow.tokens[#linenow.tokens + 1] = toknow
				toknow = ""
			end

			if #linenow.tokens > 0 then
				table.insert(lines, place, linenow)
				place = place + 1
			end

			linenow = {}
			linenow.linenumber = line
			linenow.filename = filename
			linenow.tokens = {}
		elseif not comment then
			if c == ";" then
				comment = true
			elseif (c == " ") or (c == "\t") or (c == ",") then
				if #toknow > 0 then
					linenow.tokens[#linenow.tokens + 1] = toknow

					toknow = ""
				end
			elseif c == '"' then
				instring = true
				isl = linenow
			elseif c == "'" then
				inchar = true
				char = 0
			else
				toknow = toknow .. c
			end
		end
	end

	if instring then
		lerror(isl, "unfinished string")
		return false
	end

	if #toknow > 0 then
		linenow.tokens[#linenow.tokens + 1] = toknow
	end

	if #linenow.tokens > 0 then
		table.insert(lines, place, linenow)
	end

	return true
end

function asm.includeiter(lines, base)
	for i = 1, #lines do
		local line = lines[i]

		if not line.included then
			if line.tokens[1] == ".include" then
				line.included = true

				local srcf = io.open(base .. line.tokens[2], "r")

				if not srcf then
					lerror(line, "file not found")
					return false, false
				end

				local ok = asm.tokenize(lines, i + 1, srcf:read("*a"), line.tokens[2])

				if not ok then
					return false, false
				end

				return true, true
			end
		end
	end

	return true, false
end

local function label_t(name, bc, ltype, section, mainlabel)
	local label = {}

	label.name = name
	label.bc = bc
	label.type = ltype
	label.locallabels = {}
	label.section = section
	label.erefs = 0
	label.rrefs = 0
	label.mainlabel = mainlabel

	return label
end

local function section_t(name)
	local section = {}

	section.name = name

	section.data = {}

	section.relocations = {}

	section.rrefs = 0
	section.size = 0

	section.bc = 0
	section.origin = 0
	section.offset = 0

	if name:sub(-3,-1) == "bss" then
		section.bss = true
	end

	labels["_"..name] = label_t("_"..name, 1, "special", section)
	labels["_"..name.."_size"] = label_t("_"..name.."_size", 2, "special", section)
	labels["_"..name.."_end"] = label_t("_"..name.."_end", 3, "special", section)

	return section
end

local entry

local structname
local structoff = 0

function asm.labels(isa, sections, lines, sectionsbyid)
	local currentsection

	local currentlabel

	for i = 1, #lines do
		local line = lines[i]

		local t1 = line.tokens[1]

		if structname then
			if t1 == ".end-struct" then
				constants[structname.."_sizeof"] = structoff

				structname = nil
			else
				local sz = myToNumber(t1)

				if not sz then
					lerror(line, "bad struct element size")
					return false
				end

				local name = line.tokens[2]

				if not name then
					lerror(line, "no name for struct element")
					return false
				end

				constants[structname.."_"..name] = structoff

				structoff = structoff + sz
			end
		elseif (#line.tokens == 1) and (t1:sub(-1,-1) == ":") then
			-- label

			if not currentsection then
				lerror(line, "no section selected")
				return false
			end

			local lname = t1:sub(1,-2)

			if (lname == "") or (lname == ".") then
				lerror(line, "weird label definition, kinda needs a name")
				return false
			end

			if lname:sub(1,1) == "." then
				if not currentlabel then
					lerror(line, "no label defined for this label to be local to")
					return false
				end

				currentlabel.locallabels[lname] = label_t(lname, currentsection.bc, "local", currentsection, currentlabel)
			else
				local l = labels[lname]

				if l and (l.type ~= "extern") then
					lerror(line, "attempt to redefine label")
					return false
				end

				l = label_t(lname, currentsection.bc, "local", currentsection)

				labels[lname] = l
				currentlabel = l
			end
		elseif t1:sub(1,1) == "." then
			-- directive

			if t1 == ".section" then
				-- switch sections

				local sname = line.tokens[2]

				if not sname then
					lerror(line, "no section name specified")
					return false
				end

				if sections[sname] then
					currentsection = sections[sname]
				else
					currentsection = section_t(sname)
					sections[sname] = currentsection
					sectionsbyid[#sectionsbyid+1] = currentsection
				end
			elseif t1 == ".define" then
				-- define a constant label

				local lname = line.tokens[2]

				if not lname then
					lerror(line, "no constant name specified")
					return false
				end

				local lval = line.tokens[3]

				if not lval then
					lerror(line, "no constant value specified")
					return false
				end

				local cv = myToNumber(lval)

				if not cv then
					lerror(line, "bad constant value")
					return false
				end

				constants[lname] = cv
			elseif t1 == ".static" then
				-- include a binary file

				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local path = line.tokens[2]

				if not path then
					lerror(line, "no pathname specified")
					return false
				end

				local file = io.open(base .. "/" .. path, "r")

				if not file then
					lerror(line, "can't open file '"..path.."'")
					return false
				end

				local sc = file:read("*a")

				local size = file:seek("end")

				file:close()

				line.static = sc
				line.staticsize = size

				currentsection.bc = currentsection.bc + size
			elseif t1 == ".struct" then
				-- define a struct

				structname = line.tokens[2]

				if not structname then
					lerror(line, "no struct name specified")
					return false
				end

				structoff = 0
			elseif t1 == ".bytes" then
				-- fill some bytes

				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local howmany = line.tokens[2]

				if not howmany then
					lerror(line, "no bytecount specified")
					return false
				end

				local which = line.tokens[3]

				if not which then
					lerror(line, "no byte specified")
					return false
				end

				howmany = myToNumber(howmany)

				if not howmany then
					lerror(line, "bad bytecount")
					return false
				end

				which = myToNumber(which)

				if not which then
					lerror(line, "bad byte")
					return false
				end

				currentsection.bc = currentsection.bc + howmany
			elseif t1 == ".org" then
				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local origin = line.tokens[2]

				if not origin then
					lerror(line, "no origin specified")
					return false
				end

				origin = myToNumber(origin)

				if not origin then
					lerror(line, "bad origin")
					return false
				end

				if currentsection.bc ~= 0 then
					lerror(line, "warning: strange place to set this section's origin?")
				end

				currentsection.origin = origin
			elseif t1 == ".entry" then
				local label = line.tokens[2]

				if not label then
					lerror(line, "no label specified")
					return false
				end

				if entry then
					lerror(line, "can't define more than one entrypoint")
					return false
				end

				local l = labels[label]

				if not l then
					lerror(line, "no such label")
					return false
				end

				l.entry = true

				entry = l
			elseif t1 == ".global" then
				local label = line.tokens[2]

				if not label then
					lerror(line, "no label specified")
					return false
				end

				local l = labels[label]

				if not l then
					lerror(line, "no such label")
					return false
				end

				l.type = "global"
			elseif t1 == ".weak" then
				local label = line.tokens[2]

				if not label then
					lerror(line, "no label specified")
					return false
				end

				local l = labels[label]

				if not l then
					lerror(line, "no such label")
					return false
				end

				l.type = "weak"
			elseif t1 == ".extern" then
				local label = line.tokens[2]

				if not label then
					lerror(line, "no label specified")
					return false
				end

				local l = labels[label]

				if not l then
					labels[label] = label_t(label, 0, "extern", nil)
				end
			elseif t1 == ".dl" then
				-- put a long

				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local whatlong = line.tokens[2]

				if not whatlong then
					lerror(line, "no long specified")
					return false
				end

				currentsection.bc = currentsection.bc + 4
			elseif t1 == ".di" then
				-- put an int

				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local whatint = line.tokens[2]

				if not whatint then
					lerror(line, "no int specified")
					return false
				end

				if not myToNumber(whatint) then
					whatint = constants[whatint]

					if not whatint then
						lerror(line, "label ints not supported")
						return false
					end
				end

				currentsection.bc = currentsection.bc + 2
			elseif t1 == ".db" then
				-- put a byte

				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local whatbyte = line.tokens[2]

				if not whatbyte then
					lerror(line, "no byte specified")
					return false
				end

				if not myToNumber(whatbyte) then
					whatbyte = constants[whatbyte]

					if not whatbyte then
						lerror(line, "label bytes not supported")
						return false
					end
				end

				currentsection.bc = currentsection.bc + 1
			elseif t1 == ".ds" then
				-- put a string

				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local whatstring = line.tokens[2]

				if not whatstring then
					lerror(line, "no string specified")
					return false
				end

				currentsection.bc = currentsection.bc + #whatstring
			elseif t1 == ".ds$" then
				-- put a constant as a string

				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local whatstring = line.tokens[2]

				if not whatstring then
					lerror(line, "no string specified")
					return false
				end

				local wht = constants[whatstring]

				if not wht then
					lerror(line, "no such constant")
					return false
				end

				line.dsd = tostring(wht)
				line.dsl = #wht

				currentsection.bc = currentsection.bc + line.dsl
			elseif t1 == ".align" then
				-- align to given boundary

				if not currentsection then
					lerror(line, "no section selected")
					return false
				end

				local boundary = line.tokens[2]

				if not boundary then
					lerror(line, "no boundary specified")
					return false
				end

				if not myToNumber(boundary) then
					whatboundary = constants[whatboundary]

					if not whatboundary then
						lerror(line, "label boundaries not supported")
						return false
					end
				end

				local mask = boundary-1

				while band(currentsection.bc, mask) ~= 0 do
					currentsection.bc = currentsection.bc + 1
				end
			else
				lerror(line, "unknown directive")
				return false
			end
		else
			-- mnemonic

			if not currentsection then
				lerror(line, "no section selected")
				return false
			end

			local format, operands = asm.match(isa, line)

			if not format then
				lerror(line, "no matching instruction defined")
				return false
			end

			if currentsection.bss then
				lerror(line, "instructions aren't allowed in bss sections")
				return false
			end

			line.format = format
			line.operands = operands
			line.bc = currentsection.bc

			currentsection.bc = currentsection.bc + format.bytes
		end
	end

	if not currentsection then
		print("no section selected")
		return false
	end

	return true
end

function asm.instr(isa, sections, lines, sectionsbyid)
	local currentsection

	local currentlabel

	local instruct

	for i = 1, #lines do
		local line = lines[i]

		local t1 = line.tokens[1]

		if instruct then
			if t1 == ".end-struct" then
				instruct = false
			end
		elseif (#line.tokens == 1) and (t1:sub(-1,-1) == ":") then
			-- label

			if t1:sub(1,1) ~= "." then
				currentlabel = labels[t1:sub(1,-2)]
			end
		elseif t1:sub(1,1) == "." then
			-- directive

			if t1 == ".section" then
				-- switch sections

				local sname = line.tokens[2]

				currentsection = sections[sname]
			elseif t1 == ".bytes" then
				-- fill some bytes

				local howmany = line.tokens[2]

				local which = line.tokens[3]

				howmany = myToNumber(howmany)

				which = myToNumber(which)

				if not currentsection.bss then
					for i = 1, howmany do
						currentsection.data[currentsection.offset] = which

						currentsection.offset = currentsection.offset + 1
					end
				else
					currentsection.offset = currentsection.offset + howmany
				end
			elseif t1 == ".static" then
				-- include a binary file

				for i = 1, line.staticsize do
					currentsection.data[currentsection.offset] = string.byte(line.static:sub(i,i))

					currentsection.offset = currentsection.offset + 1
				end
			elseif t1 == ".struct" then
				instruct = true
			elseif t1 == ".dl" then
				-- put a long

				local whatlong = line.tokens[2]

				if not currentsection.bss then
					if myToNumber(whatlong) then
						sv32(currentsection.data, currentsection.offset, myToNumber(whatlong))
					else
						local l = labels[whatlong]

						if not l then
							lerror(line, string.format("label '%s' not defined", whatlong))
							return false
						end

						local nt

						if l.section then
							nt = l.bc + l.section.origin
							l.section.rrefs = l.section.rrefs + 1
						else
							nt = 0
						end

						if l.type == "extern" then
							l.erefs = l.erefs + 1
						end

						l.rrefs = l.rrefs + 1

						sv32(currentsection.data, currentsection.offset, nt)

						currentsection.relocations[#currentsection.relocations + 1] = {
							offset=currentsection.offset,
							symbol=l,
							long=true
						}
					end
				end

				currentsection.offset = currentsection.offset + 4
			elseif t1 == ".di" then
				-- put an int

				local whatint = line.tokens[2]

				if not currentsection.bss then
					sv16(currentsection.data, currentsection.offset, myToNumber(whatint))
				end

				currentsection.offset = currentsection.offset + 2
			elseif t1 == ".db" then
				-- put a byte

				local whatbyte = line.tokens[2]

				if not currentsection.bss then
					currentsection.data[currentsection.offset] = myToNumber(whatbyte)
				end

				currentsection.offset = currentsection.offset + 1
			elseif t1 == ".ds" then
				-- put a string

				local whatstring = line.tokens[2]

				if not whatstring then
					lerror(line, "no string specified")
					return false
				end

				local l = #whatstring

				for i = 1, l do
					local c = whatstring:sub(i,i)

					currentsection.data[currentsection.offset] = string.byte(c)

					currentsection.offset = currentsection.offset + 1
				end
			elseif t1 == ".ds$" then
				-- put a constant as a string

				local whatstring = line.tokens[2]

				if not whatstring then
					lerror(line, "no string specified")
					return false
				end

				local l = line.dsl

				for i = 1, l do
					local c = line.dsd:sub(i,i)

					currentsection.data[currentsection.offset] = string.byte(c)

					currentsection.offset = currentsection.offset + 1
				end
			elseif t1 == ".align" then
				-- align to given boundary

				local boundary = line.tokens[2]

				if not boundary then
					lerror(line, "no boundary specified")
					return false
				end

				if not myToNumber(boundary) then
					whatboundary = constants[whatboundary]

					if not whatboundary then
						lerror(line, "label boundaries not supported")
						return false
					end
				end

				local mask = boundary-1

				while band(currentsection.offset, mask) ~= 0 do
					currentsection.data[currentsection.offset] = 0

					currentsection.offset = currentsection.offset + 1
				end
			end
		else
			-- mnemonic

			local format = line.format
			local operands = line.operands

			for k,v in pairs(operands) do
				local oinfo = format.operandinfo[k]

				if type(v) == "string" then
					local l

					if v:sub(1,1) == "." then
						l = currentlabel.locallabels[v]
					else
						l = labels[v]
					end

					if not l then
						lerror(line, string.format("label '%s' not defined", v))
						return false
					end

					if l.type == "extern" then
						l.erefs = l.erefs + 1
					end

					if oinfo.relative then
						if l.section ~= currentsection then
							lerror(line, string.format("label '%s' is in another section, but this is a relative instruction", v))
							return false
						end

						operands[k] = l.bc - currentsection.offset
					else
						currentsection.relocations[#currentsection.relocations + 1] = {
							offset=currentsection.offset,
							symbol=l,
							format=format,
							field=k
						}

						l.rrefs = l.rrefs + 1

						local opc

						if l.section then
							opc = l.bc + l.section.origin
							l.section.rrefs = l.section.rrefs + 1
						else
							opc = 0
						end

						operands[k] = opc
					end
				end

				if oinfo.shift then
					operands[k] = rshift(operands[k], oinfo.shift)
				end

				if oinfo.intswap then
					local i1 = lshift(band(operands[k], 0xFFFF), 16)
					local i2 = band(rshift(operands[k], 16), 0xFFFF)

					operands[k] = bor(i1, i2)
				end

				if oinfo.repeatbits then
					local qt = oinfo.repeatbits

					while qt > 0 do
						operands[k] = bor(lshift(operands[k], oinfo.repeatbitsby), operands[k])

						qt = qt - 1
					end
				end

				if oinfo.intshift then
					-- shift the upper 16 bits right by the given value

					local i1 = band(operands[k], 0xFFFF)
					local i2 = lshift(rshift(operands[k], 16+oinfo.intshift), 16)

					operands[k] = bor(i1, i2)
				end
			end

			local encodestring = format.encodingstring

			local byte = 0

			local bit = 0

			if band(currentsection.offset, isa.alignmask) ~= 0 then
				lerror(line, "instruction is not aligned")
				return false
			end

			for i = #encodestring, 1, -1 do
				local bitc = encodestring:sub(i,i)

				local ebit = band(bit, 7)

				if (bitc == "0") or (bitc == "1") then
					byte = bor(byte, lshift(tonumber(bitc), ebit))
				else
					local ov = operands[bitc]

					byte = bor(byte, lshift(band(ov, 1), ebit))

					operands[bitc] = rshift(ov, 1)
				end

				bit = bit + 1

				if (bit ~= 0) and (bit % 8 == 0) then
					currentsection.data[currentsection.offset] = byte
					currentsection.offset = currentsection.offset + 1

					byte = 0
				end
			end
		end
	end

	return true
end

function asm.assemble(codestring, filename, isa, encoder)
	local lines = {}

	local sections = {}
	local sectionsbyid = {}

	base = getdirectory(filename)

	if not asm.tokenize(lines, 1, codestring, filename) then return false end

	local ok, more = asm.includeiter(lines, base)

	while more do
		ok, more = asm.includeiter(lines, base)
	end

	if not ok then return false end

	--[[
	for line = 1, #lines do
		local l = lines[line]

		for tok = 1, #l.tokens do
			local t = l.tokens[tok]

			print(l.linenumber, l.filename, t)
		end
	end
	]]

	constants["__DATE"] = os.date("%Y-%m-%d %X")

	if not asm.labels(isa, sections, lines, sectionsbyid) then return false end

	if not asm.instr(isa, sections, lines, sectionsbyid) then return false end

	return sections, labels, sectionsbyid
end

return asm