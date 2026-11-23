// Work system: the bottle. It gets more solid with every quarter.
// When it is full, it tells the score board and starts empty again.
integer WORK = 7010;
integer FULL = 7011;
integer quarters;

show()
{
    llSetAlpha(0.2 + 0.2 * quarters, ALL_SIDES);   // 0.2 almost clear, 1.0 solid
}

default
{
    state_entry()
    {
        llSetColor(<0.2, 0.6, 1.0>, ALL_SIDES);   // light blue
        show();
    }

    link_message(integer sender, integer number, string message, key id)
    {
        if (number != WORK) return;
        ++quarters;
        show();
        llOwnerSay("Bottle " + (string)(quarters * 25) + "% full.");
        if (quarters == 4)
        {
            quarters = 0;
            llMessageLinked(LINK_ALL_OTHERS, FULL, "", NULL_KEY);
            show();
        }
    }
}
