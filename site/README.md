# SonKuPik K500 landing site

Static Cloudflare Pages site for the SonKuPik K500 Windows application.

## Architecture

The site is intentionally dependency-free and split by user intent so visitors do not need to load the full product tour just to download the app.

- `/` — ultra-light product homepage and fast navigation
- `/features/` — complete screenshot-led product tour
- `/download/` — dedicated lightweight download path with no gallery

All pages share `/styles.css`. The release helper `/app.js` is progressive enhancement only: verified fallback download links are present directly in HTML, while latest-release synchronization runs only after page load during browser idle time and aborts quickly if GitHub is unavailable.

## Performance strategy

- no framework, package manager, build runtime, web fonts, analytics SDK or animation library
- local optimized WebP product media under `/assets/`
- only the homepage hero screenshot is preloaded
- below-the-fold feature screenshots use native lazy loading and async decoding
- long feature sections use CSS `content-visibility:auto` with intrinsic-size placeholders
- `/download/` intentionally contains no product screenshots
- immutable one-year caching for `/assets/*`
- short cache + stale-while-revalidate for non-fingerprinted CSS/JS
- HTML always revalidates so navigation/release copy can update promptly
- CSP restricts images to self-hosted assets and only allows GitHub API for optional release synchronization
- reduced-motion preference is respected

## Cloudflare Pages

Recommended Git integration settings:

- Framework preset: `None`
- Production branch: `main`
- Root directory: repository root / blank
- Build command: `exit 0`
- Build output directory: `site`

Direct upload alternative:

```bash
npx wrangler pages deploy site --project-name sonkupik-k500
```

## Public URL

`https://sonkupik-k500.pages.dev/`
