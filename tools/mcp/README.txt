PinglyMotion with an AI assistant
=================================

This folder lets an AI assistant on your PC pose, play, look at and save
animations in PinglyMotion:

  PinglyMotion.mcpb     the extension for the Claude app
  pinglymotion_mcp.js   the helper for ChatGPT and other AI tools (it needs
                        commands.json and ASSISTANT_GUIDE.md next to it)
  README.txt            this file

The same steps are in PinglyMotion: click "AI Assistant off" in the status
bar at the bottom of the window. The first time it asks "Which AI do you
use?". After that it opens a small menu: Turn on / Turn off, who is
connected, and "Connect another AI...".

First: unzip the PinglyMotion download (right-click the zip > Extract All).
Do not use the files from inside the zip.


1. Claude (the Claude app for Windows or Mac)
---------------------------------------------

Python and Node are not needed: the Claude app brings what it needs.

1. In PinglyMotion, click the AI Assistant button and pick Claude. Click
   "Show the file": this folder opens with PinglyMotion.mcpb selected.
2. In the Claude app, open Settings > Desktop app and click "Manage" next
   to Extensions.
3. Click "Advanced settings", then "Install Extension...", and pick
   PinglyMotion.mcpb in this folder. Follow the prompts to finish.
   Older layout of the Claude app: Settings > Extensions >
   Advanced settings > Install Extension...
   In some versions the installed extension is listed under
   Settings > Plugins.
   If your Claude account belongs to a company or team, the administrator
   may have blocked your own extensions.
4. In PinglyMotion, click "Turn on AI Assistant" (or Turn on in the
   button's menu). It is off each time PinglyMotion starts.
5. In Claude, start a new chat and ask, for example:
   Raise the avatar's right arm and show me a picture.
   Claude may ask for permission to use PinglyMotion. Allow it.

Update: install the new PinglyMotion.mcpb from a new PinglyMotion download
the same way.

Anthropic's help page (local MCP servers and extensions in Claude):
https://support.claude.com/en/articles/10949351-getting-started-with-local-mcp-servers-on-claude-desktop


2. ChatGPT (the ChatGPT desktop app) - NOT YET TESTED
-----------------------------------------------------

ChatGPT's "Add a custom MCP server" form only takes a web address or a
tunnel, so it cannot be used: PinglyMotion only works on your own PC.
The ChatGPT desktop app also reads a settings file, config.toml, in the
folder .codex in your user folder (or in the folder named by the
environment variable CODEX_HOME).

You need Node: https://nodejs.org (the LTS version). PinglyMotion's
ChatGPT steps say whether Node is installed.

The easy way:
1. In PinglyMotion, click the AI Assistant button and pick ChatGPT.
2. Click "Connect ChatGPT". It makes a backup of config.toml first
   (config.toml.backup-<date>-<time> next to it), then adds PinglyMotion
   and changes nothing else in the file.
3. Restart the ChatGPT app.
4. In PinglyMotion, click "Turn on AI Assistant".
5. Ask ChatGPT, for example: Raise the avatar's right arm and show me a
   picture.

By hand: add these lines at the end of config.toml, with the real place of
pinglymotion_mcp.js on your PC (single quotes, so the backslashes stay as
they are):

  [mcp_servers.pinglymotion]
  command = "node"
  args = ['C:\path\to\PinglyMotion\mcp\pinglymotion_mcp.js']

OpenAI's page about MCP servers (see "Configure with config.toml"):
https://developers.openai.com/codex/mcp


3. Other or local AI tools - NOT TESTED BY US
---------------------------------------------

Most AI tools that can start a local MCP server (stdio) need a command and
a file:

  Command:   node
  Argument:  the full path of pinglymotion_mcp.js in this folder

You need Node: https://nodejs.org (the LTS version). In PinglyMotion, pick
Other: "Copy the command" puts the whole line on the clipboard, and "Show
the file" opens this folder.

Did it work, or not? Tell us what you tried and what worked:
https://github.com/pingledingle-tech/PinglyMotion/issues/414


How it's kept safe
------------------

- It only works on your own PC. Nothing on the network or the internet can
  reach it.
- It is off each time PinglyMotion starts.
- There is a new secret key each time you turn it on.
- Requests from web pages are refused.
- Turn off locks every AI out and deletes the key.
- Nothing secret is stored in these files.

It has been through a security review and is tested automatically on every
change:
https://github.com/pingledingle-tech/PinglyMotion/blob/main/docs/SECURITY_AUDIT_21NOV.md

The AI can only open and save animations in the PinglyMotion folder in your
Documents folder. A picture of the 3D view is sent to the AI when it asks
for a screenshot.

If it does not work: check that PinglyMotion is running and the AI
Assistant button is yellow or green, then try again. Write down at which
step and what you saw.

More: docs/CONTROL.md in the repository, section "Set up an AI assistant".
