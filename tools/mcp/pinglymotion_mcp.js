#!/usr/bin/env node
/*
 * PinglyMotion MCP server (Node version)
 *
 * The MCP server, written for Node so that Claude Desktop, which ships its own Node,
 * can run it without Python (issue #7). It is the only MCP server; the old Python one was removed. It talks to
 * PinglyMotion's local control interface (see docs/CONTROL.md), so PinglyMotion must
 * be running with the AI Assistant on: click the AI Assistant button in its status bar
 * and turn it on, or start it with `pinglymotion --control`.
 *
 * Uses only Node's built-in modules (Node 16 or newer). Speaks MCP over
 * stdin/stdout, so it is started by the AI tool, not by hand.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

"use strict";

const crypto = require("crypto");
const fs = require("fs");
const http = require("http");
const os = require("os");
const path = require("path");
const readline = require("readline");

const SERVER_NAME = "pinglymotion";
const SERVER_VERSION = "0.1.0";
const SUPPORTED_PROTOCOLS = ["2025-06-18", "2025-03-26", "2024-11-05"];

// Text that goes to the AI tool together with the tool list.
// Short on purpose: the long teaching text is the assistant guide, read with the read_guide tool.
const INSTRUCTIONS =
  "PinglyMotion is a Second Life animation editor that you control with these tools. " +
  "First call read_guide: it has the Second Life rules, the joint names, which way each turn goes and the work rhythm. " +
  "Then call state and joints. Frame 1 is the reference pose and is protected: pose from frame 2. " +
  "Use screenshot to look at the result, and set_camera to see another side (never turn the hip). " +
  "Save with save_file as .dingle (working file) or .bvh. Files are only in the PinglyMotion folder shown in state.";

// The assistant guide (docs/ASSISTANT_GUIDE.md). The Windows zip and the extension carry a copy next
// to this file; in the source tree it is in docs/. It is a tool (read_guide) and an MCP resource.
const GUIDE_NAME = "ASSISTANT_GUIDE.md";
const GUIDE_URI = "pinglymotion://guide";
const GUIDE_TOOL = {
  name: "read_guide",
  description: "Returns the assistant guide for PinglyMotion: what Second Life does with an animation, the skeleton " +
    "and joint names, which way each turn goes (calibration table), typical poses as numbers, the work " +
    "rhythm, known mistakes and how to talk to a beginner. Read it first. Works even when PinglyMotion " +
    "is not running.",
  inputSchema: { type: "object", properties: {}, required: [] },
};

const NOT_RUNNING =
  "PinglyMotion is not running with the AI Assistant on. Start PinglyMotion, click the AI Assistant " +
  "button in its status bar and turn it on (or start PinglyMotion with --control), then try again.";
const UNREACHABLE =
  "Could not reach PinglyMotion. It may have been closed, or the AI Assistant was turned off. " +
  "Start it again and turn the AI Assistant on (the button in its status bar).";

class PinglyMotionError extends Error {}

function connectionFile() {
  const override = process.env.PINGLYMOTION_CONTROL_FILE;
  if (override) return override;
  let base;
  if (process.platform === "win32") {
    base = process.env.LOCALAPPDATA || path.join(os.homedir(), "AppData", "Local");
  } else if (process.platform === "darwin") {
    base = path.join(os.homedir(), "Library", "Application Support");
  } else {
    base = process.env.XDG_DATA_HOME || path.join(os.homedir(), ".local", "share");
  }
  return path.join(base, "PinglyMotion", "control.json");
}

// PinglyMotion only listens on this machine. The token must never go anywhere else, so the
// URL from control.json is checked, and requests use their own agent: never a proxy from
// HTTP_PROXY and the like (newer Node versions can send the global agent through one).
const LOCAL_URL = /^http:\/\/(127\.0\.0\.1|localhost):([0-9]{1,5})$/;
const AGENT = new http.Agent({ keepAlive: false });

// Name of the AI tool that started this server (from MCP "initialize"), shown in
// PinglyMotion's status bar. Printable ASCII only, so it fits in an HTTP header.
let clientName = "";
// Only tests change this (seconds, at least 0.05)
const HEARTBEAT_SECONDS = Math.max(0.05, Number(process.env.PINGLYMOTION_MCP_HEARTBEAT_SECONDS) || 10);

function setClientName(clientInfo) {
  if (!clientInfo || typeof clientInfo !== "object") return;
  const name = clientInfo.title || clientInfo.name || "";
  if (typeof name === "string") {
    clientName = Array.from(name).filter((c) => c >= " " && c <= "~").join("").trim().slice(0, 40);
  }
}

/* One plain HTTP request to 127.0.0.1. Resolves to {status, json} (json is undefined when the
   answer is not JSON); rejects with UNREACHABLE when nothing answers. */
function httpRequest(url, method, requestPath, headers, body, timeoutSeconds) {
  return new Promise((resolve, reject) => {
    const match = LOCAL_URL.exec(url);
    const options = {
      host: match[1],
      port: Number(match[2]),
      path: requestPath,
      method: method,
      headers: Object.assign({}, headers),
      agent: AGENT,
      timeout: timeoutSeconds * 1000,
    };
    if (body !== undefined) options.headers["Content-Length"] = Buffer.byteLength(body);
    const req = http.request(options, (response) => {
      const chunks = [];
      response.on("data", (chunk) => chunks.push(chunk));
      response.on("end", () => {
        let json;
        try {
          json = JSON.parse(Buffer.concat(chunks).toString("utf8"));
        } catch (error) {
          json = undefined;
        }
        resolve({ status: response.statusCode, json: json });
      });
      response.on("error", () => reject(new PinglyMotionError(UNREACHABLE)));
    });
    req.on("timeout", () => req.destroy(new Error("timeout")));
    req.on("error", () => reject(new PinglyMotionError(UNREACHABLE)));
    if (body !== undefined) req.write(body);
    req.end();
  });
}

function headers(token) {
  const result = { Authorization: "Bearer " + token, "Content-Type": "application/json" };
  if (clientName) result["X-PinglyMotion-Client"] = clientName;
  return result;
}

/* Before the key is sent, the program on the port must show that it knows it.

   PinglyMotion answers a random nonce with HMAC-SHA256(key, "pinglymotion-hello:" + nonce).
   After a crash control.json stays behind, and another program could take the port and
   pretend to be PinglyMotion (issue #15); it cannot give the right answer. */
async function checkItIsPinglyMotion(url, token) {
  const nonce = crypto.randomBytes(32).toString("hex");
  const expected = crypto.createHmac("sha256", token).update("pinglymotion-hello:" + nonce).digest("hex");
  const answer = await httpRequest(url, "GET", "/api/hello?nonce=" + nonce, {}, undefined, 5);
  const proof = answer.status === 200 && answer.json && typeof answer.json === "object" ? answer.json.proof : undefined;
  const ok = typeof proof === "string" && proof.length === expected.length &&
    crypto.timingSafeEqual(Buffer.from(proof), Buffer.from(expected));
  if (!ok) {
    throw new PinglyMotionError(
      "The program on PinglyMotion's port did not prove it is PinglyMotion, so nothing was sent. " +
      "PinglyMotion may have crashed or be an old version. Start PinglyMotion again with remote control on.");
  }
}

/* One request to PinglyMotion; resolves to the decoded JSON answer. */
async function request(requestPath, body, timeoutSeconds) {
  let info;
  try {
    info = JSON.parse(fs.readFileSync(connectionFile(), "utf8"));
  } catch (error) {
    throw new PinglyMotionError(NOT_RUNNING);
  }
  const url = info && typeof info === "object" ? info.url : undefined;
  const token = info && typeof info === "object" ? info.token : undefined;
  if (typeof url !== "string" || !LOCAL_URL.test(url) || typeof token !== "string") {
    throw new PinglyMotionError("The PinglyMotion connection file is not valid. Restart PinglyMotion with remote control on.");
  }

  await checkItIsPinglyMotion(url, token);

  const data = body === undefined ? undefined : JSON.stringify(body);
  const answer = await httpRequest(url, body === undefined ? "GET" : "POST", requestPath, headers(token), data,
    timeoutSeconds || 30);
  if (answer.json === undefined) {
    throw new PinglyMotionError("PinglyMotion answered with HTTP " + answer.status + ".");
  }
  return answer.json;
}

/* Tells PinglyMotion every few seconds that the assistant is still connected (issue #43). */
let heartbeat = null;
function startHeartbeat() {
  if (heartbeat !== null) return;
  const beat = () => request("/api/heartbeat", {}, 5).catch(() => {});  // not running: try again later
  beat();
  heartbeat = setInterval(beat, HEARTBEAT_SECONDS * 1000);
  heartbeat.unref();  // never keeps the server running after the AI tool has gone
}

function bundledCommands() {
  return JSON.parse(fs.readFileSync(path.join(__dirname, "commands.json"), "utf8")).commands;
}

/* The commands of the running PinglyMotion; the bundled list when it is not running. */
async function commandList() {
  try {
    const answer = await request("/api/commands", undefined, 5);
    if (answer && answer.ok && Array.isArray(answer.commands)) return answer.commands;
  } catch (error) {
    if (!(error instanceof PinglyMotionError)) throw error;
  }
  return bundledCommands();
}

function guideText() {
  for (const file of [path.join(__dirname, GUIDE_NAME), path.join(__dirname, "..", "..", "docs", GUIDE_NAME)]) {
    try {
      return fs.readFileSync(file, "utf8");
    } catch (error) {
      continue;
    }
  }
  throw new PinglyMotionError("The assistant guide (" + GUIDE_NAME + ") was not found next to the MCP server. " +
    "Update PinglyMotion: unpack the new zip over the old folder.");
}

async function toolList() {
  const tools = (await commandList()).map((command) => ({
    name: command.name,
    description: command.description,
    inputSchema: {
      type: "object",
      properties: command.parameters || {},
      required: command.required || [],
    },
  }));
  tools.push(GUIDE_TOOL);
  return tools;
}

async function callTool(name, args) {
  if (name === GUIDE_TOOL.name) return { content: [{ type: "text", text: guideText() }] };
  const known = new Set(bundledCommands().map((command) => command.name));
  if (!known.has(name)) throw new PinglyMotionError("Unknown tool: " + name);

  const body = Object.assign({}, args && typeof args === "object" && !Array.isArray(args) ? args : {});
  body.command = name;
  const answer = await request("/api", body);

  if (!answer || !answer.ok) {
    return { content: [{ type: "text", text: (answer && answer.error) || "The command failed." }], isError: true };
  }

  if (name === "screenshot") {
    return { content: [
      { type: "image", data: answer.png_base64, mimeType: answer.mime_type || "image/png" },
      { type: "text", text: "Screenshot of the 3D view (" + (answer.width || 0) + "x" + (answer.height || 0) + ")." },
    ] };
  }

  delete answer.ok;
  return { content: [{ type: "text", text: JSON.stringify(answer, null, 1) }] };
}

function reply(id, result, error) {
  const message = { jsonrpc: "2.0", id: id };
  if (error !== undefined) message.error = error;
  else message.result = result;
  process.stdout.write(JSON.stringify(message) + "\n");
}

async function handle(message) {
  const method = message.method;
  const id = message.id;
  const params = message.params && typeof message.params === "object" ? message.params : {};

  // notifications have no id and get no answer
  if (id === undefined || id === null) return;

  if (method === "initialize") {
    setClientName(params.clientInfo);
    startHeartbeat();
    const wanted = params.protocolVersion;
    reply(id, {
      protocolVersion: SUPPORTED_PROTOCOLS.includes(wanted) ? wanted : SUPPORTED_PROTOCOLS[0],
      capabilities: { tools: { listChanged: false }, resources: { listChanged: false } },
      serverInfo: { name: SERVER_NAME, version: SERVER_VERSION },
      instructions: INSTRUCTIONS,
    });
  } else if (method === "ping") {
    reply(id, {});
  } else if (method === "tools/list") {
    reply(id, { tools: await toolList() });
  } else if (method === "resources/list") {
    reply(id, { resources: [{
      uri: GUIDE_URI, name: "PinglyMotion assistant guide", mimeType: "text/markdown",
      description: "How to animate in PinglyMotion: Second Life rules, joints, calibration table, work rhythm." }] });
  } else if (method === "resources/read") {
    if (params.uri !== GUIDE_URI) {
      reply(id, undefined, { code: -32002, message: "Unknown resource: " + params.uri });
    } else {
      try {
        reply(id, { contents: [{ uri: GUIDE_URI, mimeType: "text/markdown", text: guideText() }] });
      } catch (error) {
        if (!(error instanceof PinglyMotionError)) throw error;
        reply(id, undefined, { code: -32002, message: error.message });
      }
    }
  } else if (method === "tools/call") {
    try {
      reply(id, await callTool(typeof params.name === "string" ? params.name : "", params.arguments));
    } catch (error) {
      if (!(error instanceof PinglyMotionError)) throw error;
      reply(id, { content: [{ type: "text", text: error.message }], isError: true });
    }
  } else {
    reply(id, undefined, { code: -32601, message: "Method not found: " + method });
  }
}

function main() {
  // Only these flags stop the server. Any other argument is ignored: a launcher (Claude Desktop's
  // built-in Node) may pass its own arguments, such as the path of this file.
  const argv = process.argv.slice(1);
  if (argv.includes("--version") || argv.includes("-V")) {
    console.log(SERVER_NAME + " " + SERVER_VERSION);
    return;
  }
  if (argv.includes("--help") || argv.includes("-h")) {
    console.log("PinglyMotion MCP server " + SERVER_VERSION + ". An AI tool starts it and talks MCP over stdin/stdout; " +
      "see docs/CONTROL.md.");
    return;
  }
  // Messages are handled one after the other, in the order they came
  let queue = Promise.resolve();
  const lines = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
  lines.on("line", (line) => {
    line = line.trim();
    if (!line) return;
    let message;
    try {
      message = JSON.parse(line);
    } catch (error) {
      reply(null, undefined, { code: -32700, message: "Parse error" });
      return;
    }
    if (!message || typeof message !== "object" || Array.isArray(message)) {
      reply(null, undefined, { code: -32600, message: "Invalid request" });
      return;
    }
    queue = queue.then(() => handle(message)).catch((error) => {  // keep running whatever happens
      if (message.id !== undefined && message.id !== null) {
        reply(message.id, undefined, { code: -32603, message: "Internal error: " + (error && error.message) });
      }
    });
  });
}

// Always start, also when the file is not the main module: Claude Desktop loads it through its own
// launcher, so `require.main === module` is false there and the server never answered (initialize
// timed out). A test that only wants the functions sets PINGLYMOTION_MCP_LIBRARY=1 before require().
if (process.env.PINGLYMOTION_MCP_LIBRARY !== "1") {
  main();
}

module.exports = { connectionFile, setClientName, request, commandList, bundledCommands, PinglyMotionError };
