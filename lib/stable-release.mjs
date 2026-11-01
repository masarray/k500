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
  zipUrl: null,
  zipAvailable: false,
  preferredUrl: `${RELEASE_BASE}${FALLBACK_TAG}/${FALLBACK_ASSET}`,
  preferredFormat: 'exe',
  source: 'last-known-good'
});

function expectedInstaller(tag) {
  const [major, minor, patch] = tag.slice(1).split('.').map(Number);
  // SINGLE_PER_USER_STABLE_V1: v1.1.4+ publishes ONE user-scoped Setup.
  // Preserve immutable legacy Smart/Setup asset mapping for older versions.
  const perUser = major > 1 || (major === 1 && (minor > 1 || (minor === 1 && patch >= 4)));
  const hasSmart = major === 1 && minor === 1 && patch === 3;
  return `SonKuPik-K500-${tag}-Windows-${perUser ? 'Setup-PerUser' : hasSmart ? 'Smart-Installer' : 'Setup'}.exe`;
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

  // Keep updater-compatible versioned assets intact; use the short alias
  // only once an uploaded GitHub Release asset with the exact name exists.
  const publishedAsset = (name) => Array.isArray(data.assets) ? data.assets.find(item =>
    item?.name === name && item?.state === 'uploaded' &&
    Number.isSafeInteger(item?.size) && item.size > 0 &&
    validGitHubAsset(item?.browser_download_url, tag, name)
  ) : null;
  const originalName = expectedInstaller(tag);
  const original = publishedAsset(originalName);
  if (!original) throw new Error('Stable release has no verified supported Windows installer');
  // A published alias is eligible only with its checksum companion.
  // Prevent advertising partially-uploaded assets after a transient CI error.
  const shortExeCandidate = publishedAsset('SonkuPik-k500-setup.exe');
  const shortZipCandidate = publishedAsset('SonkuPik-k500-setup.zip');
  const shortExe = shortExeCandidate?.size === original.size &&
    publishedAsset('SonkuPik-k500-setup.exe.sha256') ? shortExeCandidate : null;
  const shortZip = shortZipCandidate?.size > original.size &&
    publishedAsset('SonkuPik-k500-setup.zip.sha256') ? shortZipCandidate : null;
  const legacyZip = publishedAsset(`SonKuPik-K500-${tag}-Windows.zip`);
  const selectedExe = shortExe || original;
  const selectedZip = shortZip || legacyZip;
  const zipUrl = selectedZip?.browser_download_url || null;
  return {
    tag,
    version: tag.slice(1),
    downloadUrl: selectedExe.browser_download_url,
    downloadFilename: selectedExe.name,
    zipUrl,
    zipFilename: selectedZip?.name || null,
    zipAvailable: Boolean(zipUrl),
    preferredUrl: zipUrl || selectedExe.browser_download_url,
    preferredFormat: zipUrl ? 'zip' : 'exe',
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
