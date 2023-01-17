local format = {}

format.flat = true

function format.encode(sections, symbols, isa)
	local binary = ""

	local origin = sections.text.origin

	local i = 0

	local so = {}

	if not sections.text then
		print("asm: format-flat: no 'text' section!")
		return false
	end

	so[1] = sections.text

	for k,v in pairs(sections) do
		if k ~= "text" then
			so[#so + 1] = v
		end
	end

	for k,v in ipairs(so) do
		if (k ~= 1) and (v.origin ~= 0) then
			print("asm: format-flat: only 'text' section is allowed to have a special origin")
			return false
		end

		v.origin = origin

		origin = origin + v.bc
		i = i + 1
	end

	isa.relocate(sections)

	for k,v in ipairs(so) do
		for i = 0, v.bc - 1 do
			if not v.bss then
				binary = binary .. string.char(v.data[i])
			else
				binary = binary .. string.char(0)
			end
		end
	end

	return binary
end

return format