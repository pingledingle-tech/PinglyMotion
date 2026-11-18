Pingmator MCP server
====================

With this an AI assistant (for example Claude Code or Claude Desktop) can pose,
play, take screenshots of and save animations in Pingmator.

You need Python 3 (https://www.python.org). Nothing else has to be installed.


1. Turn on remote control in Pingmator
--------------------------------------

Options > Allow remote control (or start pingmator with --control).
It is off by default and has to be on while the AI works.


2. Register the MCP server once
-------------------------------

Claude Code - in a command window (replace the path with where you unpacked Pingmator):

    claude mcp add pingmator -- python "C:\Pingmator\mcp\pingmator_mcp.py"

Claude app (Claude Desktop) - Settings > Developer > Edit Config, and add this
to claude_desktop_config.json, then quit and restart the app:

    {
      "mcpServers": {
        "pingmator": {
          "command": "python",
          "args": ["C:\\Pingmator\\mcp\\pingmator_mcp.py"]
        }
      }
    }

On Linux and macOS use python3 and the path to this folder.


3. Updating
-----------

Unpack a new Pingmator zip over the old folder. The MCP server in this folder is
updated at the same time, so it always matches the program. Restart the AI tool
afterwards. Program and MCP server must be the same version: a new MCP server
refuses to talk to an old Pingmator.

If you unpack the new zip into ANOTHER folder, the AI still uses the old MCP
server from the folder you registered (new tools are missing or say "Unknown
tool"). Copy this mcp folder over the old one, or register the new path:

    claude mcp remove pingmator
    claude mcp add pingmator -- python "C:\NewFolder\mcp\pingmator_mcp.py"

Claude Code only finds the server when you start claude in the folder where you
ran "claude mcp add". Add --scope user to "claude mcp add" to use it everywhere.


Good to know
------------

- The AI can only open and save animations in the Pingmator folder in your
  Documents folder (for example C:\Users\<name>\Documents\Pingmator).
- It will not replace other files, and it will not throw away unsaved changes.
- Before it sends the key, the MCP server checks that it really talks to
  Pingmator.

Step-by-step guide with more details:
https://github.com/pingledingle-tech/Pingmator/blob/main/docs/CONTROL.md#set-up-an-ai-assistant
