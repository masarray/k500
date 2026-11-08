# SonKuPik K500 landing site

Cloudflare Pages site for the SonKuPik K500 Windows application, with lightweight Pages Functions for latest-release metadata and same-origin downloads.

## Architecture

The site stays dependency-free and split by user intent so visitors do not need to load the full product tour just to download the app.

- `/` — ultra-light product homepage and fast navigation
- `/features/` — complete screenshot-led product tour
- `/download/` — dedicated lightweight download path with no gallery
- `/download/windows-preferred` — 302 to the verified stable ZIP if uploaded; otherwise verified stable EXE
- `/download/windows` — legacy direct EXE route, always available when stable EXE exists
- `/download/windows-zip` — explicit ZIP link (shows official GitHub release page until ZIP exists)
- `/api/release` — same-origin latest stable version metadata

All pages share `/styles.css`. Release hydration is URL-only: every download heading, label, and button renders its final, version-independent wording in HTML at first paint. `/app.js` may update only verified `href` targets from `/api/release`—never visible text or hidden sections. Same-origin `/download/windows-preferred` works without JavaScript; it resolves the latest stable ZIP or EXE server-side. This eliminates text swaps and layout shifts on refresh, even when release metadata arrives late, changes version, or fails.

The Pages Functions live at repository-root `/functions` because the Cloudflare Pages project root is the repository root while the static build output is `site`.

## Latest-release strategy (direct + resilient)

The website never proxies executable bytes through Cloudflare. **Current stable is v1.1.6** (owner-tested v1.1.6-rc.1 promoted byte-identically through GitHub issue comment). The default download prefers the published `sonkupik-k500-setup.zip`, whose installer is named `SonKuPik-K500-v1.1.6-Windows-Setup.exe`; the alternative `sonkupik-k500-setup.exe` is the direct EXE. The direct EXE and installer stored inside the ZIP are byte-identical to the official Per-User setup (installer SHA-256: `937a28a4bb715f60ba7ed06c437bbce165357d519bab824b77fa84c39b284899`). The ZIP archive itself has its own separate checksum and is not a portable edition. The versioned `-Windows-Setup-PerUser.exe` release asset remains for existing strict in-app updater clients; do not publish a versioned `-Windows-Setup.exe` top-level asset because older machine-scope updaters misclassify that filename.

1. `/api/release` requests the GitHub Releases JSON API for the latest non-draft, non-prerelease stable.
2. The resolver validates stable semantic tags, uploaded asset state/size, exact allowed EXE/ZIP filenames and canonical GitHub download URLs. It prefers the short aliases when actually uploaded; otherwise the versioned assets continue to work. URLs are never guessed.
3. Download links are progressively updated to the direct GitHub asset URL. Without JavaScript, `/download/windows-preferred` still works as a 302 redirect. The direct EXE remains visible even when ZIP is preferred.
4. GitHub API throttling, timeouts, malformed metadata, or a missing new installer fall back to the last known-good published stable (`v1.1.6`, verified ZIP and EXE) rather than returning a blocking 503. Update that immutable fallback when publishing a newly qualified stable.
5. The download page always shows a separate GitHub Release page link and checksums link. The site does not ask users to disable security protections.
6. The offline Node regressions cover successful ZIP discovery, missing/malicious ZIP, prereleases, metadata failure, and assert the live landing script never changes visible text after paint. The Python packaging regression verifies original SHA-256, archive integrity and deterministic output. No desktop build is needed for website/package logic.

The existing **stable promotion job** creates the short-name EXE, verified ZIP, README and SHA-256 companions **atomically from the accepted RC bytes**. There is no separate ZIP job, Windows rebuild, or release-triggered follow-on workflow. The stable v1.1.6 release and its checksum assets are immutable; earlier v1.1.5 and other historical releases remain unchanged.

Note: availability of GitHub's download CDN and client-side antivirus decisions are outside the website's control. No legitimate distribution strategy can guarantee every browser or security product accepts unsigned EXEs.

## Performance strategy

- no framework, package manager, web fonts, analytics SDK or animation library
- local optimized WebP product media under `/assets/`
- only the homepage hero screenshot is preloaded
- below-the-fold feature screenshots use native lazy loading and async decoding
- long feature sections use CSS `content-visibility:auto` with intrinsic-size placeholders
- `/download/` intentionally contains no product screenshots
- immutable one-year caching for `/assets/*`
- short cache + stale-while-revalidate for non-fingerprinted CSS/JS
- HTML always revalidates so navigation/release copy can update promptly
- browser CSP keeps release synchronization same-origin
- reduced-motion preference is respected

## Cloudflare Pages

Recommended Git integration settings:

- Framework preset: `None`
- Production branch: `main`
- Root directory: repository root / blank
- Build command: `exit 0`
- Build output directory: `site`

Pages Functions are deployed from the repository-root `/functions` directory alongside the `site` output.

## Public URL

`https://sonkupik-k500.pages.dev/`

## Three-platform download policy
Windows prefers verified `sonkupik-k500-setup.zip` containing the byte-identical `SonKuPik-K500-v1.1.6-Windows-Setup.exe`; no new Windows build. Mac and Linux redirects only choose exact published GitHub release assets. If assets do not yet exist, the buttons lead to the real release page, never guessed links or placeholder installers. macOS Gatekeeper and Windows antivirus protections must not be bypassed.
