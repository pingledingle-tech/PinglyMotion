-- Converted from LSL by PinglyMotion: SLua level 1, "LSL style".
-- Not tested in Second Life. Read the converter's warnings before you use it.

-- LSL's integer division cuts toward zero
local function lsl_div(a, b)
    local q = a / b
    if q < 0 then
        return math.ceil(q)
    end
    return math.floor(q)
end

-- LSL's (integer) cast cuts toward zero
local function lsl_int(x)
    if x < 0 then
        return math.ceil(x)
    end
    return math.floor(x)
end

local function state_entry()
    local a = 7
    local b = -2
    local f = 7.0
    local q = lsl_div(a, b)
    local r = math.fmod(a, b)
    local g = f / 2
    a = lsl_div(a, 2)
    a = lsl_int(a * 1.5)
    local bits = bit32.bor(bit32.band(a, 255), bit32.lshift(b, 2))
    local inv = -1 - bits
    local t = lsl_int(g)
    local n = 12
    local h = tonumber(ll.GetObjectDesc()) or 0
    local s = string.format("%.6f", f)
    b = (function()
        local lsl_previous = a
        a += 1
        return lsl_previous
    end)() + (function()
        b -= 1
        return b
    end)()
    if a ~= 0 and b == 0 or bits ~= 0 then
        ll.OwnerSay(s .. tostring(q) .. tostring(r) .. tostring(t) .. tostring(n) .. string.format("%.6f", h) .. tostring(inv))
    end
    repeat
        local step = 1
        a -= step
    until not (a > 0)
end

-- LSL starts the default state with state_entry
state_entry()
