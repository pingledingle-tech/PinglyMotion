-- Converted from LSL by PinglyMotion: SLua level 1, "LSL style".
-- Not tested in Second Life. Read the converter's warnings before you use it.

local handle = 0

LLEvents:on("touch_start", function(evts)
    local n = #evts
    local i = 0
    i = 0
    while i < n do
        local who = evts[i + 1]:getKey()
        ll.RegionSayTo(who, 0, "Hello " .. evts[i + 1]:getName())
        if evts[i + 1]:getGroup() then
            ll.OwnerSay("same group")
        end
        i += 1
    end
    handle = ll.Listen(5, "", ll.GetOwner(), "")
end)

LLEvents:on("listen", function(channel, name, id, message)
    if message == "off" then
        ll.ListenRemove(handle)
    end
    ll.MessageLinked(LINK_SET, channel, message, id)
end)

LLEvents:on("link_message", function(sender, num, str, id)
    if id.istruthy then
        ll.OwnerSay(str)
    end
end)

LLEvents:on("sensor", function(evts)
    local found = #evts
    ll.OwnerSay(tostring(evts[1]:getPos()))
end)

LLEvents:on("on_rez", function(param)
    ll.ResetScript()
end)
