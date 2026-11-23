-- Converted from LSL by PinglyMotion: SLua level 1, "LSL style".
-- Not tested in Second Life. Read the converter's warnings before you use it.

local count = 0
local interval = 2.5

-- LSL states: one table of event handlers per state; state() changes the state the LSL way
local states = {}
local currentState = nil

local function state(name)
    local new = states[name]
    if new == currentState then
        return
    end
    if currentState ~= nil then
        if currentState.state_exit ~= nil then
            currentState.state_exit()
        end
        for event, handler in pairs(currentState) do
            if event ~= "state_entry" and event ~= "state_exit" then
                LLEvents:off(event, handler)
            end
        end
    end
    currentState = new
    for event, handler in pairs(new) do
        if event ~= "state_entry" and event ~= "state_exit" then
            LLEvents:on(event, handler)
        end
    end
    if new.state_entry ~= nil then
        new.state_entry()
    end
end

states.default = {
    state_entry = function()
        ll.SetText("Touch me", vector(1.0, 1.0, 1.0), 1.0)
        llcompat.SetTimerEvent(interval)
    end,
    touch_start = function(evts)
        local total_number = #evts
        count += 1
        if count >= 3 then
            state("on")
            return
        end
    end,
    timer = function()
        ll.OwnerSay("tick " .. tostring(count))
    end,
}

states.on = {
    state_entry = function()
        ll.OwnerSay("on")
        llcompat.SetTimerEvent(0.0)
    end,
    state_exit = function()
        count = 0
    end,
    touch_start = function(evts)
        local n = #evts
        if evts[1]:getKey() == ll.GetOwner() then
            state("default")
            return
        end
    end,
}

-- LSL starts in the default state
state("default")
