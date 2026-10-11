#!/usr/bin/env node
/* PUBLIC_STABLE_DOWNLOAD_ROUTING_REGRESSION_V1
 * Exercise the real Cloudflare Pages handlers with mocked fetch.
 * No internet, dependencies, Qt, Windows builds, or published releases needed.
 */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const root = path.join(__dirname, "../..");
const latestUrl = "https://github.com/masarray/k500/releases/latest";
const assetRoot = "https://github.com/masarray/k500/releases/download/";

function loadHandler(file, exportName, fetchMock) {
  const fullPath = path.join(root, file);
  const source = fs.readFileSync(fullPath, "utf8");
  const signature = "export async function " + exportName;
  assert.ok(source.includes(signature), file + " must export " + exportName);
  const script = source.replace(signature, "async function " + exportName) +
    "\nglobalThis.testHandler = " + exportName + ";\n";
  const context = { fetch: fetchMock, Request, Response, Headers, URL };
  vm.runInNewContext(script, context, { filename: file, timeout: 1500 });
  return context.testHandler;
}

async function simulate(options = {}) {
  const tag = options.tag || "v1.1.2";
  const seen = [];
  const fetchMock = async (url, init = {}) => {
    seen.push({ url: String(url), init });
    if (String(url) === latestUrl) {
      if (options.noRedirect) return new Response(null, { status: 200 });
      return new Response(null, {
        status: 302,
        headers: { Location: "https://github.com/masarray/k500/releases/tag/" + tag }
      });
    }
    assert.ok(String(url).startsWith(assetRoot), "asset must use trusted repository URL");
    if (options.assetStatus && options.assetStatus >= 400) {
      return new Response(null, { status: options.assetStatus });
    }
    const partial = init.headers.get("range") === "bytes=0-3";
    const status = partial ? 206 : 200;
    const body = init.method === "HEAD" ? null : partial ? "K500" : "binary";
    const headers = { "content-type": "application/octet-stream" };
    if (partial) headers["content-range"] = "bytes 0-3/100";
    return new Response(body, { status, headers });
  };
  const handler = loadHandler("functions/download/[kind].js", "onRequest", fetchMock);
  const req = new Request("https://sonkupik-k500.pages.dev/download/windows", {
    method: options.method || "GET",
    headers: options.range ? { Range: options.range } : undefined
  });
  const response = await handler({
    request: req,
    params: { kind: options.kind || "windows" }
  });
  return { response, seen, assets: seen.filter(x => x.url.startsWith(assetRoot)) };
}

async function checkVersion(tag, expectedName) {
  const { response, assets } = await simulate({ tag });
  assert.equal(response.status, 200, tag + " must download successful stable asset");
  assert.equal(assets.length, 1, tag + " must request exactly one installer");
  assert.ok(assets[0].url.endsWith("/" + expectedName), "wrong asset for " + tag);
  assert.equal(response.headers.get("content-disposition"),
    'attachment; filename="' + expectedName + '"',
    tag + " download filename must be the exact published installer");
  assert.equal(response.headers.get("cache-control"), "private, no-store");
}

async function checkApi(tag, expectedStatus) {
  const fetchMock = async (url) => {
    assert.equal(String(url), latestUrl);
    return new Response(null, {
      status: 302,
      headers: { Location: "https://github.com/masarray/k500/releases/tag/" + tag }
    });
  };
  const handler = loadHandler("functions/api/release.js", "onRequestGet", fetchMock);
  const response = await handler();
  assert.equal(response.status, expectedStatus, "API release gate must match proxy for " + tag);
  if (expectedStatus === 200) {
    const body = await response.json();
    assert.equal(body.tag, tag);
    assert.equal(body.channel, "stable");
    assert.equal(body.setup, "/download/windows");
  }
}

async function main() {
  // Historical public releases do not contain the native unified artifact.
  for (const tag of ["v1.0.3", "v1.1.1", "v1.1.2"]) {
    await checkVersion(tag, "SonKuPik-K500-" + tag + "-Windows-Setup.exe");
  }
  // Switch only when /releases/latest actually points at a qualifying STABLE.
  for (const tag of ["v1.1.3", "v1.1.12", "v1.2.0", "v2.0.0"]) {
    await checkVersion(tag, "SonKuPik-K500-" + tag + "-Windows-Smart-Installer.exe");
  }
  await checkApi("v1.1.2", 200);
  await checkApi("v1.1.3", 200);
  // A published RC and build metadata must NEVER masquerade as public stable.
  for (const tag of ["v1.1.3-rc.1", "v1.1.3+canary", "v01.1.3", "v1.1.3-preview"]) {
    const { response, assets } = await simulate({ tag });
    assert.equal(response.status, 503, tag + " must fail closed");
    assert.equal(assets.length, 0, "must not attempt prerelease/invalid asset");
    await checkApi(tag, 503);
  }
  const unavailable = await simulate({ tag: "v1.1.3", assetStatus: 404 });
  assert.equal(unavailable.response.status, 503, "missing native Smart must be unavailable");
  assert.equal(unavailable.assets.length, 1, "no silent downgrade to Machine installer");
  assert.ok(unavailable.assets[0].url.endsWith("-Windows-Smart-Installer.exe"));
  const noRedirect = await simulate({ noRedirect: true });
  assert.equal(noRedirect.response.status, 503, "unverifiable stable must fail closed");
  assert.equal(noRedirect.assets.length, 0);
  const head = await simulate({ tag: "v1.1.3", method: "HEAD" });
  assert.equal(head.response.status, 200);
  assert.equal(head.assets[0].init.method, "HEAD", "HEAD must remain HEAD upstream");
  assert.equal(await head.response.text(), "", "HEAD must not include installer body");
  const range = await simulate({ tag: "v1.1.3", range: "bytes=0-3" });
  assert.equal(range.response.status, 206, "range request must retain partial status");
  assert.equal(range.assets[0].init.headers.get("range"), "bytes=0-3");
  assert.equal(range.response.headers.get("content-range"), "bytes 0-3/100");
  assert.equal((await simulate({ method: "POST" })).response.status, 405);
  assert.equal((await simulate({ kind: "unapproved" })).response.status, 404);
  console.log("PASS: stable Smart download routing (legacy, native, prerelease, API, 404, HEAD, Range)");
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
