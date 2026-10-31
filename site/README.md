# SonKuPik K500 landing site

Cloudflare Pages site for the SonKuPik K500 Windows application, with lightweight Pages Functions for latest-release metadata and same-origin downloads.

## Architecture

The site stays dependency-free and split by user intent so visitors do not need to load the full product tour just to download the app.

- `/` — ultra-light product homepage and fast navigation
- `/features/` — complete screenshot-led product tour
- `/download/` — dedicated lightweight download path with no gallery
- `/download/windows` — HTTP redirect to the validated public stable GitHub installer (fallback when metadata fails)
- `/api/release` — same-origin latest stable version metadata

All pages share `/styles.css`. The release helper `/app.js` is progressive enhancement only: download links already point at the same-origin latest routes, while the release label **and visible download-button version** are synchronized after page load through `/api/release`. Static HTML retains the current stable version as a fallback, so visitors can still see which release is offered if metadata sync is delayed. The browser never needs to call the GitHub API directly.

The Pages Functions live at repository-root `/functions` because the Cloudflare Pages project root is the repository root while the static build output is `site`.

## Latest-release strategy (direct + resilient)

The website never proxies executable bytes through Cloudflare. All user downloads are served directly from the verified GitHub Releases asset URL.

1. `/api/release` requests the GitHub Releases JSON API for the latest non-draft, non-prerelease stable.
2. The resolver validates the semantic tag, exact expected Windows installer filename, asset upload state and canonical GitHub download URL.
3. Download links are progressively updated to the direct GitHub CDN URL. Without JavaScript, `/download/windows` works as a 302 redirect.
4. GitHub API throttling, timeouts, malformed metadata, or a missing new installer fall back to the last known-good published stable Smart Installer (`v1.1.3`) rather than returning a blocking 503. Update that immutable fallback when publishing a newly qualified stable.
5. The download page always shows a separate GitHub Release page link and checksums link. The site does not ask users to disable security protections.
6. The offline Node routing regression covers both successful discovery and metadata failures. No desktop build is required to test the website logic.

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
