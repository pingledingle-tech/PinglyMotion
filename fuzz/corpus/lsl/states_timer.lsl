// Two states, a timer and a counter.
integer count = 0;
list colours = ["red", "green", "blue"];

default
{
    state_entry()
    {
        llOwnerSay("Starting, " + (string)llGetListLength(colours) + " colours.");
        llSetTimerEvent(1.0);
    }

    timer()
    {
        llOwnerSay("Tick " + (string)count + ": " + llList2String(colours, count));
        ++count;
        if (count >= llGetListLength(colours)) state done;
    }
}

state done
{
    state_entry()
    {
        llSetTimerEvent(0.0);
        llOwnerSay("Done after " + (string)count + " ticks.");
    }
}
