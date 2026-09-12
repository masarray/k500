# SonKuPik K500 landing page

Static, dependency-free landing page for SonKuPik K500. The site does not require Node.js, npm, a framework, or a build step.

## Cloudflare Pages — Git integration

Recommended settings:

- Framework preset: **None**
- Production branch: **main**
- Root directory: leave blank / repository root
- Build command: `exit 0`
- Build output directory: `site`

Cloudflare Pages will publish the contents of this directory and automatically create preview deployments for pull requests/branches.

## Cloudflare Pages — Direct Upload

If the Pages project is configured for Direct Upload instead of Git integration:

```bash
npx wrangler pages deploy site --project-name sonkupik-k500
```

For a preview deployment:

```bash
npx wrangler pages deploy site --project-name sonkupik-k500 --branch preview
```

## Structure

```text
site/
├── index.html     # page content and SEO metadata
├── styles.css     # responsive visual system
├── app.js         # latest GitHub Release lookup + verified fallback links
├── favicon.svg    # lightweight favicon
├── _headers       # Cloudflare Pages security/cache headers
└── robots.txt
```

## Release links

The HTML carries stable v1.0.0 fallback download URLs. At runtime, `app.js` queries GitHub's public latest-release endpoint and upgrades the Windows Setup and Portable links when a newer public release exists. If that request fails, the verified stable fallback remains usable.

## Notes

The logo shown by the page is loaded from the repository's public `assets/SonKuPik-k500-logo.png` on `raw.githubusercontent.com`. The Content Security Policy in `_headers` explicitly allows that host and the GitHub Releases API.
