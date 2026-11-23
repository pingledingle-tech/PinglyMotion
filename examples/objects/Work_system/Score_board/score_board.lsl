// Work system: the score board. It counts the full bottles.
// It turns gold for a moment when a point is scored.
integer FULL = 7011;
integer score;

default
{
    state_entry()
    {
        llSetColor(<1.0, 1.0, 1.0>, ALL_SIDES);
    }

    link_message(integer sender, integer number, string message, key id)
    {
        if (number != FULL) return;
        ++score;
        llSetColor(<1.0, 0.8, 0.0>, ALL_SIDES);   // gold
        llOwnerSay("Score: " + (string)score);
        llSetTimerEvent(1.0);
    }

    timer()
    {
        llSetTimerEvent(0.0);
        llSetColor(<1.0, 1.0, 1.0>, ALL_SIDES);
    }
}
