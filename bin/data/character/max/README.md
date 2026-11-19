# Max avatar

`maxine.pmm` is the female "Maxine" variant of the open source Max avatar
(https://github.com/RuthAndRoth/Max), shown by PinglyMotion when
**Options > Body > Max** is chosen. Its fingers and face are weighted to the
Bento bones, so hand poses and facial expressions can be seen while animating.

It is made from the Collada file by `tools/make_max_mesh.py`:

    python3 tools/make_max_mesh.py "<Max repo>/Contrib/Ada Radius/MaxineV1Draft1.1.dae"

Licence: CC BY 4.0, see `LICENSE.txt`. Keep that notice with the file.

Max is still a draft upstream. When a newer export is published, run the
script on it and check the tests (`maxBodyLoads`, `maxFingerBends`,
`maxJawOpens`).
