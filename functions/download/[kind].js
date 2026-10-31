import { resolveStableRelease } from '../../lib/stable-release.mjs';

// Zero-byte proxy: browser downloads from GitHub's own release CDN.
// If GitHub's metadata API fails, resolveStableRelease returns the last
// verified public stable asset instead of blocking users with a 503.
export async function onRequest(context) {
  const { request, params } = context;
  if (request.method !== 'GET' && request.method !== 'HEAD') {
    return new Response('Method Not Allowed', {
      status: 405, headers: { Allow: 'GET, HEAD' }
    });
  }
  if (!['windows', 'windows-preferred', 'windows-zip'].includes(params.kind)) {
    return new Response('Not Found', { status: 404 });
  }

  const release = await resolveStableRelease();
  // Legacy Windows route remains EXE. Preferred chooses ZIP only when a
  // verified asset was uploaded. Explicit ZIP links fall back to the GitHub
  // release page (not an unexpected EXE) if the archive is unavailable.
  const target = params.kind === 'windows-preferred' ? release.preferredUrl :
    params.kind === 'windows-zip' ? (release.zipUrl || release.releaseUrl) :
    release.downloadUrl;
  return new Response(null, {
    status: 302,
    headers: {
      Location: target,
      'Cache-Control': 'no-store',
      'X-Content-Type-Options': 'nosniff'
    }
  });
}
