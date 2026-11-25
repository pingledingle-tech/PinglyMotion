// idle2 also gets every message from hub and does nothing with them.
default
{
    link_message(integer sender, integer number, string message, key id)
    {
        if (number == 99) llOwnerSay(message);
    }
}
