import { resolveStableRelease } from '../../lib/stable-release.mjs';

export async function onRequestGet() {
  const release = await resolveStableRelease();
  return new Response(JSON.stringify({
    tag: release.tag,
    version: release.version,
    channel: 'stable',
    setup: '/download/windows',
    downloadUrl: release.downloadUrl,
    zipUrl: release.zipUrl,
    zipAvailable: release.zipAvailable,
    preferredUrl: release.preferredUrl,
    preferredFormat: release.preferredFormat,
    releaseUrl: release.releaseUrl,
    resolution: release.source
  }), {
    status: 200,
    headers: {
      'Content-Type': 'application/json; charset=utf-8',
      'Cache-Control': 'no-store',
      'X-Content-Type-Options': 'nosniff'
    }
  });
}
