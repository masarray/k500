/* Public release resolver: data comes from GitHub Releases, never from an RC
 * or a guessed filename. A known-good stable installer remains usable offline
 * from GitHub API when metadata lookups are throttled or unavailable.
 */
const REPO = 'masarray/k500';
const RELEASE_API = `https://api.github.com/repos/${REPO}/releases/latest`;
const STABLE_TAG_RE = /^v(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)$/;
const FALLBACK_TAG = 'v1.1.3';
const FALLBACK_ASSET = `SonKuPik-K500-${FALLBACK_TAG}-Windows-Smart-Installer.exe`;
const RELEASE_BASE = `https://github.com/${REPO}/releases/download/`;
const FALLBACK_RELEASE = Object.freeze({
  tag: FALLBACK_TAG,
  version: FALLBACK_TAG.slice(1),
  downloadUrl: `${RELEASE_BASE}${FALLBACK_TAG}/${FALLBACK_ASSET}`,
  releaseUrl: `https://github.com/${REPO}/releases/tag/${FALLBACK_TAG}`,
  source: 'last-known-good'
});

function expectedInstaller(tag) {
  const [major, minor, patch] = tag.slice(1).split('.').map(Number);
  const hasSmart = major > 1 || (major === 1 && (minor > 1 || (minor === 1 && patch >= 3)));
  return `SonKuPik-K500-${tag}-Windows-${hasSmart ? 'Smart-Installer' : 'Setup'}.exe`;
}

function validGitHubAsset(url, tag, filename) {
  if (typeof url !== 'string') return false;
  try {
    const parsed = new URL(url);
    return parsed.protocol === 'https:' &&
      parsed.hostname === 'github.com' &&
      !parsed.username && !parsed.password && !parsed.port &&
      !parsed.search && !parsed.hash &&
      parsed.pathname === `/${REPO}/releases/download/${tag}/${filename}`;
  } catch {
    return false;
  }
}

function validateRelease(data) {
  const tag = data?.tag_name;
  if (data?.draft !== false || data?.prerelease !== false ||
      typeof tag !== 'string' || !STABLE_TAG_RE.test(tag)) {
    throw new Error('GitHub did not return a published stable release');
  }

  const assetName = expectedInstaller(tag);
  const asset = Array.isArray(data.assets) ? data.assets.find(item =>
    item?.name === assetName &&
    item?.state === 'uploaded' &&
    Number.isSafeInteger(item?.size) && item.size > 0 &&
    validGitHubAsset(item?.browser_download_url, tag, assetName)
  ) : null;
  if (!asset) throw new Error('Stable release has no verified supported Windows installer');
  return {
    tag,
    version: tag.slice(1),
    downloadUrl: asset.browser_download_url,
    releaseUrl: `https://github.com/${REPO}/releases/tag/${tag}`,
    source: 'github'
  };
}

export async function resolveStableRelease() {
  try {
    const response = await fetch(RELEASE_API, {
      headers: {
        Accept: 'application/vnd.github+json',
        'User-Agent': 'SonKuPik-K500-Landing/2.0',
        'X-GitHub-Api-Version': '2022-11-28'
      },
      cf: { cacheEverything: true, cacheTtl: 300 }
    });
    if (!response.ok) throw new Error(`Release API HTTP ${response.status}`);
    return validateRelease(await response.json());
  } catch (error) {
    // Intentionally no public 503: the last verified immutable stable
    // artifact is preferable to blocking users when GitHub API misbehaves.
    console.warn('Release metadata unavailable; using published stable fallback', String(error));
    return FALLBACK_RELEASE;
  }
}
