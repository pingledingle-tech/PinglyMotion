-- Converted from LSL by PinglyMotion: SLua level 1, "LSL style".
-- Not tested in Second Life. Read the converter's warnings before you use it.

local end_lsl = 1

local twice, helper, describe

function twice(x)
    return helper(x) * 2
end

function helper(x)
    if x < 0 then
        return 0
    end
    return x
end

function describe(on, who)
    if on == 1 and who ~= NULL_KEY then
        return "on for " .. tostring(who)
    end
    return "off"
end

local function state_entry()
    local flag = 1
    flag = if flag == 0 then 1 else 0
    ll.OwnerSay(describe(flag, ll.GetOwner()) .. tostring(twice(end_lsl)))
    while true do
        if twice(3) == 6 then
            return
        end
    end
end

-- LSL starts the default state with state_entry
state_entry()
