// Plays the bundled "wave" animation (examples/wave.dingle) one second after it starts.
// On the Script Lab's Live page, watch this folder and tick "Show effects in 3D view":
// the figure waves. A touch stops it again, as llStopAnimation does.
// lsl_run inputs: [{"at":5,"type":"touch"}]
default
{
    state_entry()
    {
        llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION);
    }

    run_time_permissions(integer perms)
    {
        if (perms & PERMISSION_TRIGGER_ANIMATION) llSetTimerEvent(1.0);
    }

    timer()
    {
        llSetTimerEvent(0.0);
        llStartAnimation("wave");
        llOwnerSay("Waving. Touch me to stop.");
    }

    touch_start(integer count)
    {
        llStopAnimation("wave");
        llOwnerSay("Stopped.");
    }
}
