// The cuff: red and locked on "Lock", green and free on anything else.
default
{
  state_entry() { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); }
  link_message(integer s, integer n, string m, key id)
  {
    if (m == "Lock") { llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES); llOwnerSay("@detach=n"); llOwnerSay("@sendim=n"); llOwnerSay("Locked."); }
    else { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); llOwnerSay("@detach=y"); llOwnerSay("@sendim=y"); llOwnerSay("Free."); }
  }
}
