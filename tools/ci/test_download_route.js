#!/usr/bin/env node
/* PUBLIC_STABLE_DIRECT_DOWNLOAD_REGRESSION_V2
 * Cloudflare Pages handlers with offline mocked GitHub API. No Windows build.
 */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const root = path.resolve(__dirname, "../..");
const API = "https://api.github.com/repos/masarray/k500/releases/latest";
const FALLBACK = "v1.1.4";
function installer(tag) {
  const v = tag.slice(1).split(".").map(Number);
  const perUser = v[0] > 1 || (v[0] === 1 && (v[1] > 1 || (v[1] === 1 && v[2] >= 4)));
  const smart = v[0] === 1 && v[1] === 1 && v[2] === 3;
  return "SonKuPik-K500-" + tag + "-Windows-" + (perUser ? "Setup-PerUser" : smart ? "Smart-Installer" : "Setup") + ".exe";
}
function zipUrl(tag) {
  return "https://github.com/masarray/k500/releases/download/" + tag + "/SonKuPik-K500-" + tag + "-Windows.zip";
}
function downloadUrl(tag) {
  return "https://github.com/masarray/k500/releases/download/" + tag + "/" + installer(tag);
}
const SHORT_EXE = "sonkupik-k500-setup.exe";
const SHORT_ZIP = "sonkupik-k500-setup.zip";
function shortAssetUrl(tag, name) {
  return "https://github.com/masarray/k500/releases/download/" + tag + "/" + name;
}
function mockFor(options) {
  const tag = options.tag || FALLBACK;
  let calls = 0;
  const release = {
    tag_name: tag, prerelease: !!options.prerelease, draft: !!options.draft,
    assets: options.missingAsset ? [] : [{
      name: installer(tag), size: 55597616, state: "uploaded",
      browser_download_url: options.badUrl || downloadUrl(tag)
    }, ...(options.zip ? [{
      name: "SonKuPik-K500-" + tag + "-Windows.zip", size: 55000000,
      state: "uploaded", browser_download_url: options.badZipUrl || zipUrl(tag)
    }] : []),
    ...(options.shortExe ? [{
      name: SHORT_EXE, size: options.emptyShortExe ? 0 : 55597616,
      state: "uploaded", browser_download_url: options.badShortExe || shortAssetUrl(tag, SHORT_EXE)
    }] : []),
    ...(options.shortZip ? [{
      name: SHORT_ZIP, size: 55599198,
      state: "uploaded", browser_download_url: options.badShortZip || shortAssetUrl(tag, SHORT_ZIP)
    }] : []),
    ...(options.shortExe && !options.missingShortExeChecksum ? [{
      name: SHORT_EXE + ".sha256", size: 99, state: "uploaded",
      browser_download_url: shortAssetUrl(tag, SHORT_EXE + ".sha256")
    }] : []),
    ...(options.shortZip && !options.missingShortZipChecksum ? [{
      name: SHORT_ZIP + ".sha256", size: 99, state: "uploaded",
      browser_download_url: shortAssetUrl(tag, SHORT_ZIP + ".sha256")
    }] : [])]
  };
  return {
    fetch: async (url, args) => {
      calls++;
      assert.equal(String(url), API);
      assert.equal(args.headers.Accept, "application/vnd.github+json");
      if (options.throwNetwork) throw new Error("network failed");
      if (options.apiStatus) return new Response("upstream failure", { status: options.apiStatus });
      return Response.json(release);
    },
    calls: () => calls
  };
}
function handlerFor(file, name, fakeFetch) {
  const shared = fs.readFileSync(path.join(root, "lib/stable-release.mjs"), "utf8")
    .replace(/\bexport\s+/g, "");
  const route = fs.readFileSync(path.join(root, file), "utf8")
    .replace(/^import [^\n]+\n/gm, "").replace(/\bexport\s+/g, "");
  const context = { fetch: fakeFetch, Request, Response, Headers, URL,
    console: { warn() {} } };
  vm.runInNewContext(shared + "\n" + route + "\nglobalThis.handler = " + name + ";",
    context, { filename: file, timeout: 2000 });
  return context.handler;
}
async function checkDownload(options, expectedTag) {
  const f = mockFor(options);
  const handler = handlerFor("functions/download/[kind].js", "onRequest", f.fetch);
  const response = await handler({
    request: new Request("https://sonkupik-k500.pages.dev/download/windows", {
      method: options.method || "GET"
    }),
    params: { kind: options.kind || "windows" }
  });
  assert.equal(response.status, options.expectedStatus || 302);
  if (response.status === 302) {
    assert.equal(response.headers.get("location"), options.expectedTarget || downloadUrl(expectedTag || FALLBACK));
    assert.equal(response.headers.get("cache-control"), "no-store");
    assert.equal(await response.text(), "", "must redirect, not proxy executable");
    assert.equal(f.calls(), 1);
  } else {
    assert.equal(f.calls(), 0);
  }
}
async function checkApi(options, expectedTag, source) {
  const f = mockFor(options);
  const handler = handlerFor("functions/api/release.js", "onRequestGet", f.fetch);
  const response = await handler();
  assert.equal(response.status, 200);
  const data = await response.json();
  const tag = expectedTag || FALLBACK;
  assert.equal(data.tag, tag);
  assert.equal(data.version, tag.slice(1));
  assert.equal(data.channel, "stable");
  assert.equal(data.setup, "/download/windows");
  const shortExe = !!options.shortExe && !options.badShortExe &&
    !options.emptyShortExe && !options.missingShortExeChecksum && source !== "last-known-good";
  const exeUrl = shortExe ? shortAssetUrl(tag, SHORT_EXE) : downloadUrl(tag);
  assert.equal(data.downloadUrl, exeUrl);
  assert.equal(data.downloadFilename, shortExe ? SHORT_EXE : installer(tag));
  const modernZip = !!options.shortZip && !options.badShortZip && !options.missingShortZipChecksum && source !== "last-known-good";
  const legacyZip = !!options.zip && !options.badZipUrl && source !== "last-known-good";
  const hasZip = modernZip || legacyZip;
  const chosenZip = modernZip ? shortAssetUrl(tag, SHORT_ZIP) : legacyZip ? zipUrl(tag) : null;
  assert.equal(data.zipAvailable, hasZip);
  assert.equal(data.zipUrl, chosenZip);
  assert.equal(data.zipFilename, modernZip ? SHORT_ZIP : legacyZip ? "SonKuPik-K500-" + tag + "-Windows.zip" : null);
  assert.equal(data.preferredUrl, chosenZip || exeUrl);
  assert.equal(data.preferredFormat, hasZip ? "zip" : "exe");
  assert.equal(data.resolution, source || "github");
}
async function main() {
  for (const tag of ["v1.0.3", "v1.1.2", "v1.1.3", "v1.1.4", "v2.0.0"]) {
    await checkDownload({ tag }, tag);
    await checkApi({ tag }, tag);
  }
  for (const failed of [
    { tag: "v1.1.4-rc.1", prerelease: true },
    { tag: "v1.1.4", draft: true },
    { tag: "v1.1.5", missingAsset: true },
    { tag: "v1.1.5", badUrl: "https://example.org/installer.exe" },
    { tag: "v1.1.5", badUrl: downloadUrl("v1.1.5") + "?untrusted=1" },
    { apiStatus: 403 }, { apiStatus: 429 }, { apiStatus: 500 },
    { throwNetwork: true }
  ]) {
    await checkDownload(failed);
    await checkApi(failed, FALLBACK, "last-known-good");
  }
  await checkDownload({ kind: "windows", shortExe: true, expectedTarget: shortAssetUrl(FALLBACK, SHORT_EXE) });
  await checkDownload({ kind: "windows", shortExe: true, badShortExe: "https://untrusted.example/setup.exe" });
  await checkDownload({ kind: "windows-preferred", shortExe: true, shortZip: true, expectedTarget: shortAssetUrl(FALLBACK, SHORT_ZIP) });
  await checkDownload({ kind: "windows-zip", shortExe: true, shortZip: true, expectedTarget: shortAssetUrl(FALLBACK, SHORT_ZIP) });
  await checkDownload({ kind: "windows-preferred", shortExe: true, expectedTarget: shortAssetUrl(FALLBACK, SHORT_EXE) });
  await checkDownload({ kind: "windows-preferred", zip: true, shortZip: true, expectedTarget: shortAssetUrl(FALLBACK, SHORT_ZIP) });
  await checkDownload({ kind: "windows-preferred", zip: true, shortZip: true, badShortZip: "https://evil.example/setup.zip", expectedTarget: zipUrl(FALLBACK) });
  await checkApi({ shortExe: true, shortZip: true });
  await checkApi({ shortExe: true, zip: true });
  await checkApi({ shortExe: true, badShortExe: "https://untrusted.example/setup.exe" });
  await checkApi({ shortExe: true, emptyShortExe: true });
  await checkApi({ shortExe: true, missingShortExeChecksum: true });
  await checkApi({ shortZip: true, missingShortZipChecksum: true });
  await checkDownload({ kind: "windows-preferred", shortExe: true, missingShortExeChecksum: true });
  await checkDownload({ kind: "windows-preferred", shortZip: true, missingShortZipChecksum: true });
  await checkApi({ shortZip: true, badShortZip: "https://untrusted.example/unsafe.zip" });
  await checkApi({ apiStatus: 429, shortExe: true, shortZip: true }, FALLBACK, "last-known-good");
  await checkDownload({ method: "HEAD" });
  await checkDownload({ kind: "windows-preferred", zip: true, expectedTarget: zipUrl(FALLBACK) });
  await checkDownload({ kind: "windows-zip", zip: true, expectedTarget: zipUrl(FALLBACK) });
  await checkDownload({ kind: "windows-preferred" });
  await checkDownload({ kind: "windows-zip", expectedTarget: "https://github.com/masarray/k500/releases/tag/" + FALLBACK });
  await checkDownload({ kind: "windows-preferred", zip: true, badZipUrl: "https://not-github.example/evil.zip" });
  await checkApi({ zip: true });
  await checkApi({ zip: true, badZipUrl: "https://not-github.example/evil.zip" });
  await checkApi({ apiStatus: 500, zip: true }, FALLBACK, "last-known-good");
  await checkDownload({ method: "POST", expectedStatus: 405 });
  await checkDownload({ kind: "other", expectedStatus: 404 });
  for (const file of ["site/index.html", "site/download/index.html", "site/features/index.html"]) {
    assert.ok(fs.readFileSync(path.join(root, file), "utf8").includes("data-release-preferred"));
  }
  assert.ok(fs.readFileSync(path.join(root, "site/download/index.html"), "utf8").includes("data-release-download"));
  assert.ok(fs.readFileSync(path.join(root, "site/download/index.html"), "utf8")
    .includes("data-release-page"), "must offer visible GitHub direct link");
  console.log("PASS: stable redirect, metadata fallback, safety and fast download CTAs, ZIP discovery");
}
main().catch(error => { console.error(error); process.exitCode = 1; });
