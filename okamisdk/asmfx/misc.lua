lshift, rshift, tohex, arshift, band, bxor, bor, bnot, bror, brol = bit.lshift, bit.rshift, bit.tohex, bit.arshift, bit.band, bit.bxor, bit.bor, bit.bnot, bit.ror, bit.rol

function bnor(a,b)
    return bnot(bor(a,b))
end

function bnand(a,b)
    return bnot(band(a,b))
end

function reverse(l)
  local m = {}
  for i = #l, 1, -1 do table.insert(m, l[i]) end
  return m
end

function TableConcat(t1,t2)
    for i=1,#t2 do
        t1[#t1+1] = t2[i]
    end
    return t1
end

function readBinFile(file)
	local out = {}
	local f = love.filesystem.read(file)
	for b in f:gmatch(".") do
		out[#out+1] = string.byte(b)
	end
	return out
end

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

-- supports hex and octal
function myToNumber(str)
    local num = 0

    local neg

    if str:sub(1,1) == "-" then
        neg = true
        str = str:sub(2)
    end

    if str:sub(1,2) == "0x" then
        if #str < 3 then
            return nil
        end

        for i = 3, #str do
            local c = hexlookup[str:sub(i,i)]

            if not c then
                return nil
            end

            num = num*16 + c
        end
    elseif str:sub(1,1) == "0" then
        for i = 2, #str do
            local c = tonumber(str:sub(i,i))

            if not c then
                return nil
            end

            if c >= 8 then
                return nil
            end

            num = num*8 + c
        end
    else
        if neg then
            return -tonumber(str)
        else
            return tonumber(str)
        end
    end

    if neg then
        num = -num
    end

    return num
end

function toInt32(byte4, byte3, byte2, byte1)
    return lshift(byte1, 24) + lshift(byte2, 16) + lshift(byte3, 8) + byte4
end

function toInt16(byte2, byte1)
    return lshift(byte1, 8) + byte2
end

function splitInt32(n) 
    return band(rshift(n, 24), 0xFF), band(rshift(n, 16), 0xFF), band(rshift(n, 8), 0xFF), band(n, 0xFF)
end

function splitInt16(n)
    return band(rshift(n, 8), 0xFF), band(n, 0xFF)
end

function splitInt24(n) 
    return band(rshift(n, 16), 0xFF), band(rshift(n, 8), 0xFF), band(n, 0xFF)
end

function trim(s)
  return s:match("^%s*(.-)%s*$")
end

function struct(stuff)
    local s = {}
    s.o = {}
    s.sz = {}
    local offset = 0
    for k,v in ipairs(stuff) do
        local size = v[1]
        local name = v[2]
        s.o[name] = offset
        s.sz[name] = size
        offset = offset + size
    end
    function s.size()
        return offset
    end
    return s
end

function BitNOT(n)
    local p,c=1,0
    for i = 0, 31 do
        local r=n%2
        if r<1 then c=c+p end
        n,p=(n-r)/2,p*2
    end
    return c
end

function tc(n) -- two's complement
	n = tonumber(n)

	if n < 0 then
		n = BitNOT(math.abs(n))+1
	end

	return n
end

function explode(d,p)
	local t, ll
	t={}
	ll=0
	if(#p == 1) then return {p} end
		while true do
			l=string.find(p,d,ll,true) -- find the next d in the string
			if l~=nil then -- if "not not" found then..
				table.insert(t, string.sub(p,ll,l-1)) -- Save it in our array.
				ll=l+1 -- save just after where we found it for searching next time.
			else
				table.insert(t, string.sub(p,ll)) -- Save what's left in our array.
				break -- Break at end, as it should be, according to the lua manual.
			end
		end
	return t
end

function tokenize(str)
	return explode(" ",str)
end

function lineate(str)
	return explode("\n",str)
end

function struct(stuff)
    local s = {}
    s.o = {}
    s.sz = {}
    local offset = 0
    for k,v in ipairs(stuff) do
        local size = v[1]
        local name = v[2]
        s.o[name] = offset
        s.sz[name] = size
        offset = offset + size
    end
    function s.size()
        return offset
    end
    return s
end

function cast(struct, tab, offset)
    local s = {}

    s.s = struct
    s.t = tab
    s.o = offset or 0

    function s.ss(n, str)
        for i = 0, s.s.sz[n]-1 do
            if str:sub(i+1,i+1) then
                s.t[s.s.o[n] + i + s.o] = str:sub(i+1,i+1):byte()
            else
                s.t[s.s.o[n] + i + s.o] = 0
            end
        end
    end

    function s.sv(n, val)
        if s.s.sz[n] == 4 then
            local b1,b2,b3,b4 = splitInt32(val)
            s.t[s.s.o[n] + s.o] = b4
            s.t[s.s.o[n] + s.o + 1] = b3
            s.t[s.s.o[n] + s.o + 2] = b2
            s.t[s.s.o[n] + s.o + 3] = b1
        elseif s.s.sz[n] == 2 then
            local b1,b2 = splitInt16(val)
            s.t[s.s.o[n] + s.o] = b2
            s.t[s.s.o[n] + s.o + 1] = b1
        elseif s.s.sz[n] == 1 then
            s.t[s.s.o[n] + s.o] = val
        else
            error("no support for vals size "..tostring(s.s.sz[n]))
        end
    end

    function s.st(n, tab, ux)
        ux = ux or 1
        if ux == 1 then
            for i = 0, #tab do
                s.t[s.s.o[n] + s.o + i] = tab[i]
            end
        elseif ux == 4 then
            for i = 0, #tab do
                local b = i*4
                local b1,b2,b3,b4 = splitInt32(tab[i] or 0)
                s.t[s.s.o[n] + s.o + b] = b4
                s.t[s.s.o[n] + s.o + b + 1] = b3
                s.t[s.s.o[n] + s.o + b + 2] = b2
                s.t[s.s.o[n] + s.o + b + 3] = b1
            end
        else
            error("no support for vals size "..tostring(ux))
        end
    end

    function s.gs(n)
        local str = ""
        for i = 0, s.s.sz[n]-1 do
            local ch = s.t[s.s.o[n] + i + s.o] or 0
            if ch == 0 then break end
            str = str .. string.char(ch)
        end
        return str
    end

    function s.gv(n)
        local v = 0
        for i = s.s.sz[n]-1, 0, -1 do
            v = v*0x100 + (s.t[s.s.o[n] + i + s.o] or 0)
        end
        return v
    end

    function s.gc()
        return s.t
    end

    function s.gt(n, ux)
        local t = {}
        ux = ux or 1
        if ux == 1 then
            for i = s.s.sz[n]-1, 0, -1 do
                t[i] = (s.t[s.s.o[n] + i + s.o] or 0)
            end
        else
            for i = 0, (s.s.sz[n]/ux)-1 do
                local v = 0
                for j = ux-1, 0, -1 do
                    v = (v * 0x100) + (s.t[s.s.o[n] + (i*4) + j + s.o] or 0)
                end
                t[i] = v
            end
        end
        return t
    end

    return s
end

function gv16(tab, offset)
    return lshift(tab[offset + 1], 8) + tab[offset]
end

function gv32(tab, offset)
    return lshift(tab[offset + 3], 24) + lshift(tab[offset + 2], 16) + lshift(tab[offset + 1], 8) + tab[offset]
end

function sv16(tab, offset, value)
    local b1,b2 = splitInt16(value)

    tab[offset] = b2
    tab[offset + 1] = b1
end

function sv32(tab, offset, value)
    local b1,b2,b3,b4 = splitInt32(value)

    tab[offset] = b4
    tab[offset + 1] = b3
    tab[offset + 2] = b2
    tab[offset + 3] = b1
end