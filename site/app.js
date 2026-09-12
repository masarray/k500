(() => {
  const api = 'https://api.github.com/repos/masarray/k500/releases/latest';

  const setHref = (id, href) => {
    const node = document.getElementById(id);
    if (node && href) node.href = href;
  };

  const syncRelease = async () => {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 3200);

    try {
      const response = await fetch(api, {
        headers: { Accept: 'application/vnd.github+json' },
        cache: 'no-store',
        signal: controller.signal
      });
      if (!response.ok) return;

      const release = await response.json();
      if (!release?.tag_name || !Array.isArray(release.assets)) return;

      const setup = release.assets.find((asset) => /Windows-Setup\.exe$/i.test(asset.name));
      const portable = release.assets.find((asset) => /Windows-Portable\.zip$/i.test(asset.name));
      const label = document.getElementById('release-label');

      if (label) label.textContent = `Stable ${release.tag_name}`;
      if (setup) {
        setHref('download-setup', setup.browser_download_url);
        setHref('download-setup-nav', setup.browser_download_url);
      }
      if (portable) setHref('download-portable', portable.browser_download_url);
    } catch {
      // Verified stable links are embedded in HTML. Network/API failure never blocks download.
    } finally {
      clearTimeout(timeout);
    }
  };

  const schedule = () => {
    if ('requestIdleCallback' in window) {
      window.requestIdleCallback(syncRelease, { timeout: 2200 });
    } else {
      window.setTimeout(syncRelease, 900);
    }
  };

  if (document.readyState === 'complete') schedule();
  else window.addEventListener('load', schedule, { once: true });
})();
