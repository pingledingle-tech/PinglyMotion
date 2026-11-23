// Gear set: the collar. When locked, the whole set cannot be taken off.
// Green when free, red when locked. The HUD in the root prim tells it which.
// RLV: detach=n stops you taking the object off (from examples/scriptlab/rlv_lock.lsl).
integer GEAR = 7002;      // HUD to pieces
integer ANSWER = 7003;    // pieces to HUD

default
{
    state_entry()
    {
        llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES);   // green: free
    }

    link_message(integer sender, integer number, string message, key id)
    {
        if (number != GEAR) return;
        if (message == "lock")
        {
            llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES);   // red: locked
            llOwnerSay("@detach=n");
            llMessageLinked(LINK_ROOT, ANSWER, "locked", NULL_KEY);
        }
        else if (message == "free")
        {
            llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES);
            llOwnerSay("@detach=y");
            llMessageLinked(LINK_ROOT, ANSWER, "free", NULL_KEY);
        }
    }
}
