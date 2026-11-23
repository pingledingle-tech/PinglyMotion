// events with detected entries, listen and link messages
integer handle;

default
{
    touch_start(integer n)
    {
        integer i;
        for (i = 0; i < n; ++i)
        {
            key who = llDetectedKey(i);
            llRegionSayTo(who, 0, "Hello " + llDetectedName(i));
            if (llDetectedGroup(i))
                llOwnerSay("same group");
        }
        handle = llListen(5, "", llGetOwner(), "");
    }

    listen(integer channel, string name, key id, string message)
    {
        if (message == "off")
            llListenRemove(handle);
        llMessageLinked(LINK_SET, channel, message, id);
    }

    link_message(integer sender, integer num, string str, key id)
    {
        if (id)
            llOwnerSay(str);
    }

    sensor(integer found)
    {
        llOwnerSay((string)llDetectedPos(0));
    }

    on_rez(integer param)
    {
        llResetScript();
    }
}
