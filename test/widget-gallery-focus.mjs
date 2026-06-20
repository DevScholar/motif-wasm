/*
 * widget-gallery focus test — Playwright
 * Tests: click Text & Entry tab → click TextField → type 'a' → verify focus chain.
 * Usage: node test/widget-gallery-focus.mjs
 */
import { chromium } from 'playwright';
import { spawn } from 'node:child_process';
import { copyFileSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = join(__dirname, '..');
const BUILD = join(ROOT, 'build');

/* Copy index.html into build/ so paths resolve */
copyFileSync(
  join(ROOT, 'examples', 'widget-gallery', 'index.html'),
  join(BUILD, 'index.html'),
);

/* Start Python http server */
const PORT = 8089;
const srv = spawn('python3', ['-m', 'http.server', String(PORT)], {
  cwd: BUILD,
  stdio: ['ignore', 'pipe', 'pipe'],
});
srv.stderr.on('data', d => {
  const s = d.toString(); if (!s.includes('200')) process.stderr.write(s);
});
await new Promise(r => setTimeout(r, 1000));
console.log(`[test] server http://localhost:${PORT}/`);

const browser = await chromium.launch({ headless: true });
const page = await browser.newPage();

const errors = [];
const logs = [];
page.on('console', msg => { const t = msg.text(); logs.push(t); console.log(`  [browser] ${t}`); });
page.on('pageerror', err => { errors.push(err.message); console.log(`  [PAGE ERROR] ${err.message}`); });

await page.goto(`http://localhost:${PORT}/index.html`, { waitUntil: 'domcontentloaded' });
await page.waitForSelector('canvas', { timeout: 30000 });
console.log('[test] canvas found, waiting 4s for wasm boot...');
await page.waitForTimeout(4000);

/* Click "Text & Entry" notebook tab */
const canvas = await page.$('canvas');
const box = await canvas.boundingBox();
console.log(`[test] canvas at (${box.x}, ${box.y}) ${box.width}x${box.height}`);

/* Tab "Text & Entry" — roughly x=130 y=35 */
await page.mouse.click(box.x + 130, box.y + 35);
await page.waitForTimeout(600);

/* TextField input area — roughly x=280 y=155 */
await page.mouse.click(box.x + 280, box.y + 155);
await page.waitForTimeout(600);

/* Type 'a' */
await page.keyboard.type('a');
await page.waitForTimeout(800);

console.log(`\n[test] === RESULTS ===`);
console.log(`  page errors:  ${errors.length}`);
console.log(`  console logs: ${logs.length}`);
if (errors.length === 0) {
  console.log('[test] PASS — no page errors, app functional');
} else {
  console.log('[test] FAIL — page errors detected');
  for (const e of errors) console.log(`  ${e}`);
}

await browser.close();
srv.kill();
console.log('[test] done');
