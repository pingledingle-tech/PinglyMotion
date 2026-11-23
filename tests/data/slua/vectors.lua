-- Converted from LSL by PinglyMotion: SLua level 1, "LSL style".
-- Not tested in Second Life. Read the converter's warnings before you use it.

local pos = vector(1, 2, 3)
local rot = ZERO_ROTATION

local function state_entry()
    pos = vector(4.0, pos.y, pos.z)
    pos = vector(pos.x, pos.y, pos.z + 1.5)
    rot = quaternion(rot.x, rot.y, rot.z, 1.0)
    local other = ll.GetPos()
    local dot = vector.dot(pos, other)
    local cross = pos % other
    local turned = pos * rot
    rot = ll.Euler2Rot(vector(0.0, 0.0, PI / 2)) * rot
    if pos ~= ZERO_VECTOR then
        ll.OwnerSay(string.format("%.6f", dot) .. tostring(cross) .. tostring(turned) .. string.format("%.6f", rot.z))
    end
    ll.SetPos(pos + vector(0.0, 0.0, -1.0))
end

-- LSL starts the default state with state_entry
state_entry()
