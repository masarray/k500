# SonKuPik K500 landing site

Cloudflare Pages site for the SonKuPik K500 Windows application, with lightweight Pages Functions for latest-release metadata and same-origin downloads.

## Architecture

The site stays dependency-free and split by user intent so visitors do not need to load the full product tour just to download the app.

- `/` — ultra-light product homepage and fast navigation
- `/features/` — complete screenshot-led product tour
- `/download/` — dedicated lightweight download path with no gallery
- `/download/windows` — streams the latest stable Windows installer through the landing domain
- `/download/portable` — streams the latest stable portable ZIP through the landing domain
- `/api/release` — same-origin latest stable version metadata

All pages share `/styles.css`. The release helper `/app.js` is progressive enhancement only: download links already point at the same-origin latest routes, while the release label is synchronized after page load through `/api/release`. The browser never needs to call the GitHub API for release synchronization.

The Pages Functions live at repository-root `/functions` because the Cloudflare Pages project root is the repository root while the static build output is `site`.

## Latest-release strategy

GitHub Releases remains the upstream artifact store, but the visitor-facing download contract is owned by the landing site.

1. The Pages Function resolves GitHub's canonical `releases/latest` pointer server-side.
2. The stable tag is combined with the release workflow's versioned artifact naming contract.
3. The large installer/ZIP is streamed through Cloudflare without buffering it in Worker memory.
4. `Content-Disposition: attachment` keeps the user on the landing page while the file downloads.
5. Range and validator headers are forwarded so large downloads remain resilient.

This means a future stable release becomes the download target automatically without editing landing-page HTML.

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
