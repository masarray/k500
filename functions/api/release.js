const LATEST_RELEASE_URL = 'https://github.com/masarray/k500/releases/latest';

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

export async function onRequestGet() {
  try {
    const tag = await resolveLatestTag();
    const version = tag.startsWith('v') ? tag.slice(1) : tag;
    return new Response(JSON.stringify({
      tag,
      version,
      channel: 'stable',
      setup: '/download/windows',
      portable: '/download/portable'
    }), {
      status: 200,
      headers: {
        'Content-Type': 'application/json; charset=utf-8',
        'Cache-Control': 'public, max-age=60, s-maxage=300, stale-while-revalidate=86400',
        'X-Content-Type-Options': 'nosniff'
      }
    });
  } catch {
    return new Response(JSON.stringify({
      tag: null,
      version: null,
      channel: 'stable',
      setup: '/download/windows',
      portable: '/download/portable'
    }), {
      status: 503,
      headers: {
        'Content-Type': 'application/json; charset=utf-8',
        'Cache-Control': 'no-store',
        'X-Content-Type-Options': 'nosniff'
      }
    });
  }
}
