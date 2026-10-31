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
const FALLBACK = "v1.1.3";
function installer(tag) {
  const v = tag.slice(1).split(".").map(Number);
  const smart = v[0] > 1 || (v[0] === 1 && (v[1] > 1 || (v[1] === 1 && v[2] >= 3)));
  return "SonKuPik-K500-" + tag + "-Windows-" + (smart ? "Smart-Installer" : "Setup") + ".exe";
}
function downloadUrl(tag) {
  return "https://github.com/masarray/k500/releases/download/" + tag + "/" + installer(tag);
}
function mockFor(options) {
  const tag = options.tag || FALLBACK;
  let calls = 0;
  const release = {
    tag_name: tag, prerelease: !!options.prerelease, draft: !!options.draft,
    assets: options.missingAsset ? [] : [{
      name: installer(tag), size: 55597616, state: "uploaded",
      browser_download_url: options.badUrl || downloadUrl(tag)
    }]
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
    assert.equal(response.headers.get("location"), downloadUrl(expectedTag || FALLBACK));
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
  assert.equal(data.downloadUrl, downloadUrl(tag));
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
  await checkDownload({ method: "HEAD" });
  await checkDownload({ method: "POST", expectedStatus: 405 });
  await checkDownload({ kind: "other", expectedStatus: 404 });
  for (const file of ["site/index.html", "site/download/index.html", "site/features/index.html"]) {
    assert.ok(fs.readFileSync(path.join(root, file), "utf8").includes("data-release-download"));
  }
  assert.ok(fs.readFileSync(path.join(root, "site/download/index.html"), "utf8")
    .includes("data-release-page"), "must offer visible GitHub direct link");
  console.log("PASS: stable redirect, metadata fallback, safety and fast download CTAs");
}
main().catch(error => { console.error(error); process.exitCode = 1; });
