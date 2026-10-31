#!/usr/bin/env node
/* Zero-flicker contract: release metadata must never change visible copy.
 * Run the REAL browser script in a headless DOM stub with valid, bad,
 * missing, and future release metadata. No browser, network, or Windows build.
 */
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '../..');
const script = fs.readFileSync(path.join(root, 'site/app.js'), 'utf8');
const host = 'https://github.com/masarray/k500/releases/';
const shortExe = 'SonkuPik-k500-setup.exe';
const shortZip = 'SonkuPik-k500-setup.zip';

function anchor(fallback, copy) {
  let text = copy;
  let hidden = false;
  return {
    href: fallback,
    get textContent() { return text; },
    set textContent(_) { throw new Error('Visible copy mutated after paint'); },
    get hidden() { return hidden; },
    set hidden(_) { throw new Error('Visibility mutated after paint'); },
    setAttribute(attr) { if (attr !== 'aria-label') throw new Error('Unexpected DOM mutation: ' + attr); }
  };
}

async function exercise(metadata, expected = {}) {
  const original = 'Download latest stable K500';
  const preferred = anchor('/download/windows-preferred', original);
  const nav = anchor('/download/windows-preferred', 'Download K500');
  const direct = anchor('/download/windows', 'Download EXE');
  const relPage = anchor(host + 'tag/v1.1.3', 'GitHub release');
  const exeChecksum = anchor(host + 'download/v1.1.3/SHA256SUMS.txt', 'Installer checksum');
  const zipChecksum = anchor(host + 'tag/v1.1.3', 'ZIP checksum');
  const selectors = {
    '[data-release-preferred]': [preferred, nav],
    '[data-release-download]': [direct],
    '[data-release-page]': [relPage],
    '[data-release-checksums]': [exeChecksum],
    '[data-zip-checksum]': [zipChecksum]
  };
  const document = {
    readyState: 'complete',
    querySelector: () => null,
    querySelectorAll: (selector) => selectors[selector] || []
  };
  vm.runInNewContext(script, {
    document,
    window: {requestIdleCallback: fn => fn()},
    fetch: async () => metadata === null ? Promise.reject(new Error('API unavailable')) : Response.json(metadata),
    AbortController, URL, Response,
    setTimeout: () => 1, clearTimeout: () => {}
  }, {filename: 'site/app.js', timeout: 1500});
  await new Promise(resolve => setImmediate(resolve));
  assert.equal(preferred.textContent, original);
  assert.equal(nav.textContent, 'Download K500');
  assert.equal(direct.textContent, 'Download EXE');
  assert.equal(zipChecksum.textContent, 'ZIP checksum');
  assert.equal(preferred.href, expected.preferred || '/download/windows-preferred');
  assert.equal(nav.href, expected.preferred || '/download/windows-preferred');
  assert.equal(direct.href, expected.exe || '/download/windows');
  assert.equal(zipChecksum.href, expected.zipChecksum || host + 'tag/v1.1.3');
}

async function main() {
  for (const name of ['index.html', 'download/index.html', 'features/index.html']) {
    const html = fs.readFileSync(path.join(root, 'site', name), 'utf8');
    assert.match(html, /data-release-preferred/);
    assert.doesNotMatch(html, /data-release-template=/, name + ' has mutable release label');
    assert.doesNotMatch(html, />Download v1\.1\.3</, name + ' hard-codes stale button text');
  }
  const landing = fs.readFileSync(path.join(root, 'site/download/index.html'), 'utf8');
  assert.match(landing, /href="\/download\/windows-preferred"/);
  assert.match(landing, /href="\/download\/windows" data-release-download/);
  assert.doesNotMatch(landing, /data-zip-checksum-wrap hidden/);
  assert.doesNotMatch(script, /\.textContent\s*=/, 'release sync rewrites visible text');
  assert.doesNotMatch(script, /\.hidden\s*=/, 'release sync reveals content after paint');
  const tag = 'v1.1.3';
  const exe = host + 'download/' + tag + '/' + shortExe;
  const zip = host + 'download/' + tag + '/' + shortZip;
  const valid = {
    tag, version: '1.1.3',
    downloadFilename: shortExe, downloadUrl: exe,
    zipFilename: shortZip, zipUrl: zip, zipAvailable: true
  };
  await exercise(valid, {preferred: zip, exe, zipChecksum: zip + '.sha256'});
  await exercise({...valid, zipAvailable: false, zipUrl: null}, {
    preferred: exe, exe, zipChecksum: host + 'tag/v1.1.3'
  });
  await exercise({...valid, zipUrl: 'https://unsafe.example/payload.zip'}, {
    preferred: exe, exe, zipChecksum: host + 'tag/v1.1.3'
  });
  await exercise({...valid, downloadUrl: 'https://unsafe.example/payload.exe'});
  const newTag = 'v1.1.4';
  const newExe = host + 'download/' + newTag + '/' + shortExe;
  const newZip = host + 'download/' + newTag + '/' + shortZip;
  await exercise({...valid, tag: newTag, version:'1.1.4', downloadUrl: newExe, zipUrl: newZip},
    {preferred: newZip, exe: newExe, zipChecksum: newZip + '.sha256'});
  await exercise(null);
  console.log('PASS: release links update without text flicker, visibility changes or stale labels');
}
main().catch(e => { console.error(e); process.exitCode = 1; });
