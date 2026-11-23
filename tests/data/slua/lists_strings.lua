-- Converted from LSL by PinglyMotion: SLua level 1, "LSL style".
-- Not tested in Second Life. Read the converter's warnings before you use it.

-- LSL counts positions from 0, SLua's ll functions from 1; negative positions count from the end in both
local function lsl_index(i)
    if i < 0 then
        return i
    end
    return i + 1
end

-- SLua gives nil for "not found" and counts from 1; the script expects -1 and counting from 0
local function lsl_found(i)
    if i == nil then
        return -1
    end
    return i - 1
end

-- LSL's list + list makes a new list
local function lsl_list_join(a, b)
    local result = table.clone(a)
    table.move(b, 1, #b, #result + 1, result)
    return result
end

local names = {"Ann", "Bo"}
local greeting = "Hi"

local function state_entry()
    names = lsl_list_join(names, {"Cy"})
    names = lsl_list_join(names, {"Di", 4, 2.5})
    local i = lsl_found(ll.ListFindList(names, {"Bo"}))
    if i ~= -1 then
        ll.OwnerSay(ll.List2String(names, lsl_index(i)) .. " is number " .. tostring(i + 1))
    end
    if -1 - lsl_found(ll.SubStringIndex(greeting, "i")) ~= 0 then
        ll.OwnerSay("has an i")
    end
    local first = ll.GetSubString(greeting, 1, 1)
    local last = ll.GetSubString(greeting, -1, -1)
    names = ll.DeleteSubList(names, lsl_index(i), lsl_index(i + 1))
    ll.OwnerSay(first .. last .. tostring(ll.GetListLength(names)))
    ll.OwnerSay(ll.DumpList2String(names, ", ") .. ll.DumpList2String(names, ""))
    if #names == #{} then
        ll.OwnerSay("empty")
    end
    greeting ..= "!"
end

-- LSL starts the default state with state_entry
state_entry()
