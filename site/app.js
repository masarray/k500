(() => {
  const api = 'https://api.github.com/repos/masarray/k500/releases/latest';

  const setHref = (id, href) => {
    const node = document.getElementById(id);
    if (node && href) node.href = href;
  };

  fetch(api, {
    headers: { Accept: 'application/vnd.github+json' },
    cache: 'no-store'
  })
    .then((response) => {
      if (!response.ok) throw new Error(`GitHub API returned ${response.status}`);
      return response.json();
    })
    .then((release) => {
      if (!release || !release.tag_name || !Array.isArray(release.assets)) return;

      const tag = release.tag_name;
      const setup = release.assets.find((asset) => /Windows-Setup\.exe$/i.test(asset.name));
      const portable = release.assets.find((asset) => /Windows-Portable\.zip$/i.test(asset.name));

      const label = document.getElementById('release-label');
      const title = document.getElementById('release-title');
      if (label) label.textContent = `Stable ${tag}`;
      if (title) title.textContent = `SonKuPik K500 ${tag}`;

      if (setup) {
        setHref('download-setup', setup.browser_download_url);
        setHref('download-setup-bottom', setup.browser_download_url);
      }
      if (portable) setHref('download-portable', portable.browser_download_url);
    })
    .catch(() => {
      // The HTML contains verified stable fallback links, so the page remains fully usable offline from the API.
    });
})();
