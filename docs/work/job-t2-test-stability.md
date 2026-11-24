Branch: job/t2-test-stability
Status: active
Pull request: #470
Files:
- src/scriptlab/labwatcher.*
- tests/tst_labwatcher.cpp
- tests/tst_labrecipe.cpp
- tests/tst_scriptlabeffects.cpp
- tests/tst_control.cpp
- tests/tst_auroralink.cpp
- tests/CMakeLists.txt
- tests/scriptlabpanel/**
- tests/scriptlabbeginner/**
- tests/scriptlabplayer/**
- tests/scriptlabwindow/**
- tests/librarypanel/tst_librarypanel.cpp
- tests/easingtiles/tst_easingtiles.cpp
- tests/paneldocks/tst_paneldocks.cpp
- tests/dialogfit/tst_dialogfit.cpp
- tests/repeat/**

## What

Test stability (T2): the Script Lab folder watcher on Windows and fixed waits in the tests.

1. Find why `labeditor` (and before it `labeffects`) fails now and then on the Windows runner, and fix the cause.
2. The folder watcher (`LabWatcher`) sets its watch up again and checks the files itself when Windows drops a watched folder ("FindNextChangeNotification failed ... Access is denied").
3. Tests that use the watcher do not depend on the Windows change notification: they call the loading directly or wait for the signal with a ceiling.
4. Fixed waits (`QTest::qWait` with a fixed time) become waits until the thing has happened, with a ceiling. Same assertions, only different waiting.

Proof: the affected tests run many times on the Windows runner (how and the result are in the pull request report).
