default
{
    state_entry()
    {
        integer i;
        for (i = -2; i < 4; ++i)
            llOwnerSay(llGetInventoryName(INVENTORY_ALL, i) + (string)llGetInventoryNumber(i) + (string)llGetInventoryKey("n"));
        llOwnerSay((string)llGetInventoryType("o") + (string)llGetInventoryType(""));
        llGetNotecardLine("n", 0);
        llGetNotecardLine("n", 1);
        llGetNotecardLine("n", 99);
        llGetNotecardLine("n", -5);
        llGetNotecardLine("o", 0);
        llGetNumberOfNotecardLines("n");
        llGetNumberOfNotecardLines("");
        llRezObject("o", llGetPos() + <0, 0, 10>, <1, 2, 3>, <0, 0, 0, 0>, -1);
        llRezAtRoot("o", <1000, 1000, 1000>, ZERO_VECTOR, ZERO_ROTATION, 0);
        llRezObject("n", ZERO_VECTOR, ZERO_VECTOR, ZERO_ROTATION, 0);
        llGiveInventory(llGetOwner(), "n");
        llGiveInventory(NULL_KEY, "o");
        llGiveInventory("c0ffee00-0000-4000-8000-000000000003", "n");
        llGiveInventory("c0ffee00-0000-4000-8000-000000000003", "o");
        llRemoveInventory("o");
        llRemoveInventory("nothing");
    }
    dataserver(key id, string data)
    {
        llOwnerSay((string)id + data);
        llGetNotecardLine("n", (integer)llStringLength(data));
    }
    object_rez(key id) { llOwnerSay("rezzed " + (string)id); }
    changed(integer c) { llOwnerSay(llGetInventoryName(INVENTORY_NOTECARD, 0)); }
}
