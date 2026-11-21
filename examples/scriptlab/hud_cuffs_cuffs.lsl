// Script Lab example (step 5): cuffs that lock with RLV when the HUD (hud_cuffs_hud.lsl) says so.
// Only messages from objects of the same owner count.
integer CUFFS = -7002;

default
{
    state_entry()
    {
        llListen(CUFFS, "", NULL_KEY, "");
    }

    attach(key id)
    {
        if (id) llOwnerSay("Cuffs worn.");
    }

    listen(integer channel, string name, key id, string message)
    {
        if (llGetOwnerKey(id) != llGetOwner()) return;
        if (message == "lock")
        {
            llOwnerSay("@detach=n");
            llRegionSay(CUFFS, "locked");
        }
        else if (message == "unlock")
        {
            llOwnerSay("@detach=y");
            llRegionSay(CUFFS, "unlocked");
        }
    }
}
