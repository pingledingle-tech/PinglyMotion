// Gear set, the HUD in the root prim. Touch it and pick "Lock all" or "Free all".
// It tells every piece with a link message on number 7002, and counts the answers.
// Built from examples/scriptlab/hud_cuffs_hud.lsl: one object, so link messages
// take the place of the chat channel.
integer DIALOG = -7001;   // the channel the dialog answers on
integer GEAR = 7002;      // HUD to pieces
integer ANSWER = 7003;    // pieces to HUD
integer PIECES = 8;
integer answers;

default
{
    state_entry()
    {
        llListen(DIALOG, "", llGetOwner(), "");
        llSetColor(<1.0, 1.0, 1.0>, ALL_SIDES);
        llOwnerSay("HUD ready. Touch me.");
    }

    touch_start(integer count)
    {
        llDialog(llDetectedKey(0), "Gear set", ["Lock all", "Free all"], DIALOG);
    }

    listen(integer channel, string name, key id, string message)
    {
        answers = 0;
        if (message == "Lock all") llMessageLinked(LINK_ALL_OTHERS, GEAR, "lock", NULL_KEY);
        else if (message == "Free all") llMessageLinked(LINK_ALL_OTHERS, GEAR, "free", NULL_KEY);
    }

    link_message(integer sender, integer number, string message, key id)
    {
        if (number != ANSWER) return;
        ++answers;
        if (answers < PIECES) return;
        if (message == "locked") llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES);
        else llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES);
        llOwnerSay("All " + (string)PIECES + " pieces are " + message + ".");
    }
}
