// Small setup: an example of our own for the Script Lab tab Improve a setup.
// hub sends three link messages to every script in the object (LINK_SET).
// Only the script in the prim User needs them.
// Try: change LINK_SET to 2 (the prim User), save, and read Before and after.
default
{
    state_entry()
    {
        integer i;
        for (i = 1; i <= 3; ++i) llMessageLinked(LINK_SET, i, "go", "");
    }
}
