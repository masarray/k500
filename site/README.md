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

All pages share `/styles.css`. The release helper `/app.js` is progressive enhancement only: download links already point at the same-origin latest routes, while the release label **and visible download-button version** are synchronized after page load through `/api/release`. Static HTML retains the current stable version as a fallback, so visitors can still see which release is offered if metadata sync is delayed. The browser never needs to call the GitHub API directly.

The Pages Functions live at repository-root `/functions` because the Cloudflare Pages project root is the repository root while the static build output is `site`.

## Latest-release strategy (direct + resilient)

The website never proxies executable bytes through Cloudflare. The default download prefers a published, verified ZIP from GitHub Releases. A clearly labeled direct EXE download remains available. A ZIP is simply the identical Smart Installer wrapped for delivery; it is not a portable edition.

1. `/api/release` requests the GitHub Releases JSON API for the latest non-draft, non-prerelease stable.
2. The resolver validates the semantic tag, exact expected Windows EXE/ZIP filenames, asset upload states and canonical GitHub download URLs. It never guesses a ZIP URL before upload.
3. Download links are progressively updated to the direct GitHub asset URL. Without JavaScript, `/download/windows-preferred` still works as a 302 redirect. The direct EXE remains visible even when ZIP is preferred.
4. GitHub API throttling, timeouts, malformed metadata, or a missing new installer fall back to the last known-good published stable Smart Installer (`v1.1.3`, EXE) rather than returning a blocking 503. Update that immutable fallback when publishing a newly qualified stable.
5. The download page always shows a separate GitHub Release page link and checksums link. The site does not ask users to disable security protections.
6. The offline Node regression covers successful ZIP discovery, missing/malicious ZIP, prereleases, and metadata failure. The Python packaging regression verifies original SHA-256, archive integrity and deterministic output. No desktop build is needed for website/package logic.

On main deployment, the existing consolidated CI's Linux `zip-release` job downloads the currently published stable Smart Installer, verifies it against its existing `SHA256SUMS.txt`, wraps it in `SonKuPik-K500-vX.Y.Z-Windows.zip` and publishes an additional `.zip.sha256`. The job skips ZIPs already uploaded and does not replace existing assets, rebuild Windows software or rewrite original release checksums. The same procedure is integrated into the stable promotion workflow (important for promotions published using GitHub's default token, whose release events do not trigger other workflows).

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
