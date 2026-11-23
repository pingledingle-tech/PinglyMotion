// two states, a timer and a touch
integer count;
float interval = 2.5;

default
{
    state_entry()
    {
        llSetText("Touch me", <1.0, 1.0, 1.0>, 1.0);
        llSetTimerEvent(interval);
    }

    touch_start(integer total_number)
    {
        count++;
        if (count >= 3)
            state on;
    }

    timer()
    {
        llOwnerSay("tick " + (string)count);
    }
}

state on
{
    state_entry()
    {
        llOwnerSay("on");
        llSetTimerEvent(0.0);
    }

    state_exit()
    {
        count = 0;
    }

    touch_start(integer n)
    {
        if (llDetectedKey(0) == llGetOwner())
            state default;
    }
}
