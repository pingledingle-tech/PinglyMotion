Branch: fix/in-program-links
Status: done
Pull request: #35
Files:
- src/aboutdialog.ui
- src/updatenotificationdialog.ui
- src/releaseinfo.cpp
- src/pinglymotionwindow.cpp

## What

The four links inside the program (About dialog, update dialog, update check address, AI help link) pointed to the old repository. They now point to this one. Only the repository name in the address changes. Until the first release here, the update check finds no release and stays silent.
