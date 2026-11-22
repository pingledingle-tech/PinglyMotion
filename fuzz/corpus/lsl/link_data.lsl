// Script Lab step 9a seed: link messages between copies of one script and Linkset Data
integer n;
default
{
    state_entry()
    {
        llLinksetDataWrite("count", (string)llGetLinkNumber());
        llMessageLinked(LINK_ALL_OTHERS, 1, "ping", "");
    }
    link_message(integer sender, integer num, string str, key id)
    {
        if (++n < 5) llMessageLinked(sender, num + 1, str + "!", id);
        llLinksetDataWrite(str, (string)num);
    }
    linkset_data(integer action, string name, string value)
    {
        if (action == LINKSETDATA_RESET) llOwnerSay("reset");
        else if (llGetListLength(llLinksetDataListKeys(0, 0)) > 3) llLinksetDataReset();
    }
    touch_start(integer t)
    {
        llMessageLinked(LINK_SET, 0, llLinksetDataRead("count"), NULL_KEY);
        llLinksetDataDelete("count");
        llOwnerSay((string)llLinksetDataAvailable() + " " + (string)llLinksetDataCountKeys());
    }
}
