// A simple RLV relay. It lets other objects, such as a chair or a trap, send RLV
// commands to your viewer. Written for PinglyMotion from the open relay
// specification, in our own words. This one says yes to every object: real relays ask first.
//
// A message on the relay channel looks like this:   name,target,commands
//   name      any short word the sender picks, sent back in every answer
//   target    the relay's key, or ffffffff-ffff-ffff-ffff-ffffffffffff for every relay
//   commands  one or more commands with | between them: @detach=n|@sendchat=n
// For every command the relay answers   name,relay key,command,ok   (or ko for no).
// Two meta commands: !version (answers the version) and !release (lets go of everything).
integer RELAY = -1812221819;
string EVERYONE = "ffffffff-ffff-ffff-ffff-ffffffffffff";
integer holding;   // TRUE while a sender holds a restriction

show()
{
    if (holding) llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES);   // red: holding
    else llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES);           // green: free
}

default
{
    state_entry()
    {
        llListen(RELAY, "", NULL_KEY, "");
        holding = FALSE;
        show();
        llOwnerSay("Relay ready on channel " + (string)RELAY + ".");
    }

    listen(integer channel, string name, key id, string message)
    {
        list parts = llParseStringKeepNulls(message, [","], []);
        if (llGetListLength(parts) != 3) return;
        string ident = llList2String(parts, 0);
        string target = llList2String(parts, 1);
        if (target != (string)llGetKey() && target != EVERYONE) return;
        list commands = llParseString2List(llList2String(parts, 2), ["|"], []);
        integer i;
        for (i = 0; i < llGetListLength(commands); ++i)
        {
            string command = llList2String(commands, i);
            string answer = "ko";
            if (command == "!version") answer = "1100";
            else if (command == "!release")
            {
                llOwnerSay("@clear");
                holding = FALSE;
                answer = "ok";
            }
            else if (llGetSubString(command, 0, 0) == "@")
            {
                llOwnerSay(command);
                if (llSubStringIndex(command, "=n") > 0 || llSubStringIndex(command, "=add") > 0) holding = TRUE;
                answer = "ok";
            }
            llRegionSayTo(id, RELAY, ident + "," + (string)id + "," + command + "," + answer);
        }
        show();
    }
}
