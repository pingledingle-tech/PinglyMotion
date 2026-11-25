// idle gets every message from hub and does nothing with them.
// It also keeps a long list in memory all the time, and a variable and a function it never uses.
list names = ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
              "k", "l", "m", "n", "o", "p", "q", "r", "s", "t"];
integer spare;

noop()
{
}

default
{
    link_message(integer sender, integer number, string message, key id)
    {
        if (number == 99) llOwnerSay(llList2String(names, 0));
    }
}
