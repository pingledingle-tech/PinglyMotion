// The HUD: a click shows a menu, a line on channel -77 from the owner goes to the cuff.
default
{
  state_entry() { llListen(-77, "", llGetOwner(), ""); llOwnerSay("Ready."); }
  touch_start(integer n) { llDialog(llDetectedKey(0), "Cuffs", ["Lock", "Free"], -77); }
  listen(integer c, string name, key id, string m) { llMessageLinked(LINK_ALL_OTHERS, 1, m, NULL_KEY); }
}
