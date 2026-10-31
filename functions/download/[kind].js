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
  if (params.kind !== 'windows') {
    return new Response('Not Found', { status: 404 });
  }

  const release = await resolveStableRelease();
  return new Response(null, {
    status: 302,
    headers: {
      Location: release.downloadUrl,
      'Cache-Control': 'no-store',
      'X-Content-Type-Options': 'nosniff'
    }
  });
}
