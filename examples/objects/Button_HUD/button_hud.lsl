// Button HUD: the simplest HUD. Touch it, pick a button, and it changes colour.
// Written for PinglyMotion's Script Lab, from the HUD part of examples/scriptlab/hud_cuffs_hud.lsl.
integer DIALOG = -7001;   // the channel the dialog answers on

default
{
    state_entry()
    {
        llListen(DIALOG, "", llGetOwner(), "");
        llSetColor(<1.0, 1.0, 1.0>, ALL_SIDES);   // white
        llOwnerSay("Touch me to pick a colour.");
    }

    touch_start(integer count)
    {
        llDialog(llDetectedKey(0), "Pick a colour", ["Red", "Green"], DIALOG);
    }

    listen(integer channel, string name, key id, string message)
    {
        if (message == "Red") llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES);
        else if (message == "Green") llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES);
        llOwnerSay("Now " + llToLower(message) + ".");
    }
}
