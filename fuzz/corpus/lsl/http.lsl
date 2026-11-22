default
{
    state_entry()
    {
        llHTTPRequest("https://example.com/", [], "");
        llHTTPRequest("http://example.com/x", [HTTP_METHOD, "POST", HTTP_MIMETYPE, "application/json", HTTP_BODY_MAXLENGTH, 7,
                      HTTP_CUSTOM_HEADER, "X-A", "b", HTTP_VERBOSE_THROTTLE, FALSE, HTTP_USER_AGENT, "t/1"], "{}");
        llHTTPRequest("ftp://example.com/", [], "");
        llHTTPRequest("https://example.com/", [HTTP_BODY_MAXLENGTH], "");
        llHTTPRequest("https://example.com/", [HTTP_METHOD, 1, 99], "");
        integer i;
        for (i = 0; i < 30; ++i) llHTTPRequest("https://e.example/" + (string)i, [HTTP_BODY_MAXLENGTH, i], "");
    }
    http_response(key id, integer status, list meta, string body)
    {
        llOwnerSay((string)status + llList2CSV(meta) + body);
        if (status == 500) llHTTPRequest("https://example.com/", [HTTP_BODY_MAXLENGTH, 16384], body);
    }
    touch_start(integer n) { llResetScript(); }
}
