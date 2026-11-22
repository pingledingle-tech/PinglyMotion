// Listens on channel 7 and starts or stops an animation on command.
// lsl_run inputs: [{"at":1,"channel":7,"text":"dance"},{"at":3,"channel":7,"text":"stop"}]
string anim = "dance1";

default
{
    state_entry()
    {
        llListen(7, "", llGetOwner(), "");
        llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION);
    }

    run_time_permissions(integer perms)
    {
        if (perms & PERMISSION_TRIGGER_ANIMATION) llOwnerSay("Ready. Say dance or stop on channel 7.");
    }

    listen(integer channel, string name, key id, string message)
    {
        message = llToLower(llStringTrim(message, STRING_TRIM));
        if (message == "dance") llStartAnimation(anim);
        else if (message == "stop") llStopAnimation(anim);
        else llOwnerSay("Unknown command: " + message);
    }
}
