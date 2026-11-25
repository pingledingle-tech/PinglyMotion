// Start-up smoke for the web trial build (docs/DEV.md, "The web trial build").
// Serves the build folder on a local port, opens PinglyMotion.html in headless Chromium
// (Playwright, WebGL 2 through SwiftShader), waits for the program's start line, then takes
// a screenshot and writes every console line and page error to a log.
//
//   node tools/web/smoke.mjs <build folder with PinglyMotion.html> <output folder> [seconds]
//
// Exit code 0: the program printed its start line and nothing aborted. 1 otherwise.
import { createServer } from 'node:http';
import { readFile, mkdir, writeFile } from 'node:fs/promises';
import { extname, join, normalize, resolve, sep } from 'node:path';
import { chromium } from 'playwright';

const [root, outDir, secondsArg] = process.argv.slice(2);
if (!root || !outDir) {
  console.error('usage: node tools/web/smoke.mjs <build folder> <output folder> [seconds]');
  process.exit(2);
}
const seconds = Number(secondsArg || 90);
const base = resolve(root);
const types = {
  '.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript',
  '.wasm': 'application/wasm', '.data': 'application/octet-stream', '.svg': 'image/svg+xml',
};

// only files below the build folder are served
const server = createServer(async (req, res) => {
  const path = normalize(join(base, decodeURIComponent(new URL(req.url, 'http://x').pathname)));
  if (path !== base && !path.startsWith(base + sep)) { res.writeHead(403); res.end(); return; }
  try {
    const body = await readFile(path);
    res.writeHead(200, { 'Content-Type': types[extname(path)] || 'application/octet-stream' });
    res.end(body);
  } catch {
    res.writeHead(404); res.end();
  }
});
await new Promise((ok) => server.listen(0, '127.0.0.1', ok));
const url = `http://127.0.0.1:${server.address().port}/PinglyMotion.html`;

await mkdir(outDir, { recursive: true });
const lines = [];
const note = (text) => { lines.push(`${new Date().toISOString()} ${text}`); };
const browser = await chromium.launch({
  args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'],
});
const page = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
let started = false;
let aborted = false;
page.on('console', (msg) => {
  const text = msg.text();
  note(`console.${msg.type()}: ${text}`);
  if (/PinglyMotion \d/.test(text)) started = true;
  if (/Aborted\(|RuntimeError|abort\(/.test(text)) aborted = true;
});
page.on('pageerror', (err) => { note(`pageerror: ${err.message}`); aborted = true; });
page.on('requestfailed', (req) => note(`requestfailed: ${req.url()} ${req.failure()?.errorText}`));

note(`open ${url}`);
await page.goto(url);
const webgl2 = await page.evaluate(() => !!document.createElement('canvas').getContext('webgl2'));
note(`WebGL 2 available: ${webgl2}`);
const end = Date.now() + seconds * 1000;
while (!started && !aborted && Date.now() < end) await page.waitForTimeout(500);
// give the window time to draw after the start line
if (started && !aborted) await page.waitForTimeout(15000);
await page.screenshot({ path: join(outDir, 'screenshot.png') });
note(`started: ${started}, aborted: ${aborted}`);
await writeFile(join(outDir, 'console.log'), lines.join('\n') + '\n');
console.log(lines.join('\n'));
await browser.close();
server.close();
process.exit(started && !aborted ? 0 : 1);
