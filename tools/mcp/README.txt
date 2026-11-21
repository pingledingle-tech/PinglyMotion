PinglyMotion MCP server
====================

With this an AI assistant (for example Claude Code or Claude Desktop) can pose,
play, take screenshots of and save animations in PinglyMotion.

You need Python 3 (https://www.python.org). Nothing else has to be installed.


Easiest for the Claude app: see "START HERE - Claude.txt" next to the program (unzip first, then
in the Claude app: Settings > Extensions > Advanced settings > Install Extension... and pick PinglyMotion.mcpb). No Python is needed for that. Then do step 1 below. The rest of
this file is the manual way, which still needs Python.


1. Turn on remote control in PinglyMotion
--------------------------------------

Options > Allow remote control (or start PinglyMotion with --control).
It is off by default and has to be on while the AI works.


2. Register the MCP server once
-------------------------------

Claude Code - in a command window (replace the path with where you unpacked PinglyMotion):

    claude mcp add pinglymotion -- python "C:\PinglyMotion\mcp\pinglymotion_mcp.py"

Claude app (Claude Desktop) - Settings > Developer > Edit Config, and add this
to claude_desktop_config.json, then quit and restart the app:

    {
      "mcpServers": {
        "pinglymotion": {
          "command": "python",
          "args": ["C:\\PinglyMotion\\mcp\\pinglymotion_mcp.py"]
        }
      }
    }

On Linux and macOS use python3 and the path to this folder.


3. Updating
-----------

Unpack a new PinglyMotion zip over the old folder. The MCP server in this folder is
updated at the same time, so it always matches the program. Restart the AI tool
afterwards. Program and MCP server must be the same version: a new MCP server
refuses to talk to an old PinglyMotion.

If you unpack the new zip into ANOTHER folder, the AI still uses the old MCP
server from the folder you registered (new tools are missing or say "Unknown
tool"). Copy this mcp folder over the old one, or register the new path:

    claude mcp remove pinglymotion
    claude mcp add pinglymotion -- python "C:\NewFolder\mcp\pinglymotion_mcp.py"

Claude Code only finds the server when you start claude in the folder where you
ran "claude mcp add". Add --scope user to "claude mcp add" to use it everywhere.


Good to know
------------

- The AI can only open and save animations in the PinglyMotion folder in your
  Documents folder (for example C:\Users\<name>\Documents\PinglyMotion).
- It will not replace other files, and it will not throw away unsaved changes.
- Before it sends the key, the MCP server checks that it really talks to
  PinglyMotion.
- The AI gets a short guide when it connects: how Second Life treats an
  animation, the joint names, which way each turn goes and how to work. It reads
  it with the tool "read_guide" (also an MCP resource, pinglymotion://guide).
  The guide is the file ASSISTANT_GUIDE.md in this folder.

Step-by-step guide with more details:
https://github.com/pingledingle-tech/PinglyMotion/blob/main/docs/CONTROL.md#set-up-an-ai-assistant
