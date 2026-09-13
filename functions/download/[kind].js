const REPO = 'masarray/k500';
const LATEST_RELEASE_URL = `https://github.com/${REPO}/releases/latest`;

const ARTIFACTS = {
  windows: {
    filename: 'SonKuPik-K500-Windows-Setup.exe',
    versioned: (tag) => `SonKuPik-K500-${tag}-Windows-Setup.exe`,
    contentType: 'application/vnd.microsoft.portable-executable'
  },
  portable: {
    filename: 'SonKuPik-K500-Windows-Portable.zip',
    versioned: (tag) => `SonKuPik-K500-${tag}-Windows-Portable.zip`,
    contentType: 'application/zip'
  }
};

async function resolveLatestTag() {
  const response = await fetch(LATEST_RELEASE_URL, {
    redirect: 'manual',
    headers: {
      Accept: 'text/html',
      'User-Agent': 'SonKuPik-K500-Landing/1.0'
    },
    cf: { cacheEverything: true, cacheTtl: 300 }
  });

  const location = response.headers.get('location');
  if (!location) throw new Error(`Latest release redirect missing (${response.status})`);

  const target = new URL(location, LATEST_RELEASE_URL);
  const match = target.pathname.match(/\/releases\/tag\/([^/?#]+)$/);
  if (!match) throw new Error('Unable to parse latest release tag');

  const tag = decodeURIComponent(match[1]);
  if (!/^v?\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$/.test(tag)) {
    throw new Error('Unexpected release tag format');
  }
  return tag;
}

function upstreamHeaders(request) {
  const headers = new Headers({
    Accept: 'application/octet-stream',
    'User-Agent': 'SonKuPik-K500-Landing/1.0'
  });
  for (const name of ['range', 'if-range', 'if-none-match', 'if-modified-since']) {
    const value = request.headers.get(name);
    if (value) headers.set(name, value);
  }
  return headers;
}

async function fetchArtifact(request, url) {
  return fetch(url, {
    method: request.method,
    redirect: 'follow',
    headers: upstreamHeaders(request),
    cf: { cacheEverything: true, cacheTtl: 31536000 }
  });
}

function proxyResponse(request, upstream, descriptor) {
  const headers = new Headers();
  for (const name of ['content-type', 'content-length', 'content-range', 'accept-ranges', 'etag', 'last-modified']) {
    const value = upstream.headers.get(name);
    if (value) headers.set(name, value);
  }
  if (!headers.has('content-type')) headers.set('Content-Type', descriptor.contentType);
  headers.set('Content-Disposition', `attachment; filename="${descriptor.filename}"`);
  headers.set('Cache-Control', 'private, no-store');
  headers.set('X-Content-Type-Options', 'nosniff');

  return new Response(request.method === 'HEAD' ? null : upstream.body, {
    status: upstream.status,
    statusText: upstream.statusText,
    headers
  });
}

export async function onRequest(context) {
  const { request, params } = context;
  if (request.method !== 'GET' && request.method !== 'HEAD') {
    return new Response('Method Not Allowed', { status: 405, headers: { Allow: 'GET, HEAD' } });
  }

  const descriptor = ARTIFACTS[params.kind];
  if (!descriptor) return new Response('Not Found', { status: 404 });

  try {
    const tag = await resolveLatestTag();
    const assetName = descriptor.versioned(tag);
    const assetUrl = `https://github.com/${REPO}/releases/download/${encodeURIComponent(tag)}/${assetName}`;
    const upstream = await fetchArtifact(request, assetUrl);

    if (!upstream.ok && upstream.status !== 206 && upstream.status !== 304) {
      upstream.body?.cancel();
      return new Response('Latest stable download is temporarily unavailable. Please try again shortly.', {
        status: 503,
        headers: {
          'Content-Type': 'text/plain; charset=utf-8',
          'Cache-Control': 'no-store'
        }
      });
    }

    return proxyResponse(request, upstream, descriptor);
  } catch {
    return new Response('Latest stable download is temporarily unavailable. Please try again shortly.', {
      status: 503,
      headers: {
        'Content-Type': 'text/plain; charset=utf-8',
        'Cache-Control': 'no-store'
      }
    });
  }
}
