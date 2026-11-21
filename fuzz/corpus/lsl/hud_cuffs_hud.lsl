// Script Lab example (step 5): a HUD that asks with a dialog and tells the cuffs on a channel.
// Run it together with hud_cuffs_cuffs.lsl in one lsl_run with "objects" (the HUD first),
// then touch the HUD and answer its dialog: lsl_event touch, then lsl_event dialog with button "Lock".
integer DIALOG = -7001;
integer CUFFS = -7002;

default
{
    state_entry()
    {
        llListen(DIALOG, "", llGetOwner(), "");
        llListen(CUFFS, "", NULL_KEY, "");
    }

    touch_start(integer n)
    {
        llDialog(llDetectedKey(0), "Cuffs", ["Lock", "Unlock"], DIALOG);
    }

    listen(integer channel, string name, key id, string message)
    {
        if (channel == DIALOG) llRegionSay(CUFFS, llToLower(message));
        else llOwnerSay("Cuffs: " + message);
    }
}
