// First colour: the smallest example. It works with what the program has now.
// This folder is the prim "Box 0". A prop named "Box 0" shows its colour
// while "Show effects in 3D view" is ticked.
default
{
    state_entry()
    {
        llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES);   // red, green, blue from 0 to 1
        llOwnerSay("Box 0 is red now.");
    }
}
