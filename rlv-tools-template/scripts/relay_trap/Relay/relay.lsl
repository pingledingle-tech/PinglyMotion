// A small relay for the test. It says yes to every command, until the owner says "refuse" on channel -78.
// Then it says no (ko) and holds nothing. "accept" turns yes on again. "release" lets go of everything.
// It answers with llSay, because chat inside one object needs it; a real relay uses llRegionSayTo.
// Written for this template in our own words from the open relay specification.
integer RELAY = -1812221819;
integer yes = TRUE;
default
{
  state_entry() { llListen(RELAY, "", NULL_KEY, ""); llListen(-78, "", llGetOwner(), ""); llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); }
  listen(integer channel, string name, key id, string m)
  {
    if (channel == -78)
    {
      if (m == "release") { llOwnerSay("@clear"); llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); }
      else yes = (m != "refuse");
      return;
    }
    list p = llParseStringKeepNulls(m, [","], []);
    if (llGetListLength(p) != 3) return;
    list cmds = llParseString2List(llList2String(p, 2), ["|"], []);
    integer i;
    for (i = 0; i < llGetListLength(cmds); ++i)
    {
      string c = llList2String(cmds, i);
      string a = "ko";
      if (c == "!release") { llOwnerSay("@clear"); llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); a = "ok"; }
      else if (llGetSubString(c, 0, 0) == "@" && yes) { llOwnerSay(c); llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES); a = "ok"; }
      llSay(RELAY, llList2String(p, 0) + "," + (string)id + "," + c + "," + a);
    }
  }
}
