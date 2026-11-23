// integer and float math, casts and bits
default
{
    state_entry()
    {
        integer a = 7;
        integer b = -2;
        float f = 7.0;
        integer q = a / b;
        integer r = a % b;
        float g = f / 2;
        a /= 2;
        a *= 1.5;
        integer bits = (a & 0xFF) | (b << 2);
        integer inv = ~bits;
        integer t = (integer)g;
        integer n = (integer)"12abc";
        float h = (float)llGetObjectDesc();
        string s = (string)f;
        b = a++ + --b;
        if (a && !b || bits)
            llOwnerSay(s + (string)q + (string)r + (string)t + (string)n + (string)h + (string)inv);
        do
        {
            integer step = 1;
            a -= step;
        }
        while (a > 0);
    }
}
