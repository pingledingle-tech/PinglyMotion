// A trap. A touch asks the wearer's relay to hold two restrictions.
// It says what the relay answered. A relay is the worn part that obeys objects like this.
// Here the relay is the part Relay of this object. A real trap talks to the relay the avatar wears.
integer RELAY = -1812221819;
string EVERYONE = "ffffffff-ffff-ffff-ffff-ffffffffffff";
default
{
  state_entry() { llListen(RELAY, "", NULL_KEY, ""); llOwnerSay("Trap ready."); }
  touch_start(integer n) { llSay(RELAY, "trap," + EVERYONE + ",@sendim=n|@fly=n"); }
  listen(integer channel, string name, key id, string m)
  {
    list p = llParseStringKeepNulls(m, [","], []);
    if (llGetListLength(p) != 4 || llList2String(p, 0) != "trap") return;
    if (llList2String(p, 3) == "ok") llOwnerSay("Caught: " + llList2String(p, 2));
    else llOwnerSay("Refused: " + llList2String(p, 2));
  }
}
