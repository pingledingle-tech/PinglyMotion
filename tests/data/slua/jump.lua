-- Converted from LSL by PinglyMotion: SLua level 1, "LSL style".
-- Not tested in Second Life. Read the converter's warnings before you use it.

local function state_entry()
    local i = 0
    -- @again (a label for jump; Lua has no jump)
    i += 1
    if i < 3 then
        -- jump again: this jump cannot be converted automatically, rewrite it by hand
    end
    ll.OwnerSay(tostring(i))
end

-- LSL starts the default state with state_entry
state_entry()
