// Timed lock: touch it and it locks for 5 seconds, then opens by itself.
// Built from examples/scriptlab/states_timer.lsl (two states and a timer)
// and examples/scriptlab/rlv_lock.lsl (the RLV lock).
integer SECONDS = 5;   // change this number to lock for longer
integer left;

default
{
    state_entry()
    {
        llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES);   // green: free
        llOwnerSay("Free. Touch me to lock for " + (string)SECONDS + " seconds.");
    }

    touch_start(integer count)
    {
        state locked;
    }
}

state locked
{
    state_entry()
    {
        left = SECONDS;
        llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES);   // red: locked
        llOwnerSay("@detach=n");
        llOwnerSay("Locked for " + (string)SECONDS + " seconds.");
        llSetTimerEvent(1.0);
    }

    timer()
    {
        --left;
        if (left > 1) llOwnerSay((string)left + " seconds left.");
        else if (left == 1) llOwnerSay("1 second left.");
        else state default;
    }

    state_exit()
    {
        llSetTimerEvent(0.0);
        llOwnerSay("@detach=y");
    }
}
