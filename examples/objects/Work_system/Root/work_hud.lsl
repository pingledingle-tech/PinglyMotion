// Work system, the HUD in the root prim. Touch it and press "Work".
// Each press fills the bottle a quarter. A full bottle scores a point.
// The HUD talks to the bottle and the score board with link messages.
integer DIALOG = -7001;   // the channel the dialog answers on
integer WORK = 7010;      // HUD to bottle: one more quarter
integer FULL = 7011;      // bottle to score board: the bottle was full

default
{
    state_entry()
    {
        llListen(DIALOG, "", llGetOwner(), "");
        llOwnerSay("Touch me and press Work.");
    }

    touch_start(integer count)
    {
        llDialog(llDetectedKey(0), "Work system", ["Work"], DIALOG);
    }

    listen(integer channel, string name, key id, string message)
    {
        if (message == "Work") llMessageLinked(LINK_ALL_OTHERS, WORK, "", NULL_KEY);
    }
}
