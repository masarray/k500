(() => {
  const api = 'https://api.github.com/repos/masarray/k500/releases/latest';

  const setHref = (id, href) => {
    const node = document.getElementById(id);
    if (node && href) node.href = href;
  };

  const initProductZoom = () => {
    const root = document.querySelector('[data-product-zoom]');
    const source = root?.querySelector('[data-zoom-source]');
    const pane = root?.querySelector('[data-zoom-pane]');
    const lens = root?.querySelector('[data-zoom-lens]');
    const image = source?.querySelector('img');
    if (!root || !source || !pane || !lens || !image) return;

    pane.style.backgroundImage = `url("${image.currentSrc || image.src}")`;

    const finePointer = window.matchMedia('(hover: hover) and (pointer: fine)');
    if (!finePointer.matches) return;

    let frame = 0;
    let pendingPoint = null;

    const render = () => {
      frame = 0;
      if (!pendingPoint) return;

      const rect = source.getBoundingClientRect();
      if (!rect.width || !rect.height) return;

      const x = Math.min(1, Math.max(0, (pendingPoint.x - rect.left) / rect.width));
      const y = Math.min(1, Math.max(0, (pendingPoint.y - rect.top) / rect.height));
      const lensWidth = lens.offsetWidth;
      const lensHeight = lens.offsetHeight;
      const lensX = Math.min(rect.width - lensWidth, Math.max(0, (x * rect.width) - (lensWidth / 2)));
      const lensY = Math.min(rect.height - lensHeight, Math.max(0, (y * rect.height) - (lensHeight / 2)));

      pane.style.setProperty('--zoom-x', `${(x * 100).toFixed(2)}%`);
      pane.style.setProperty('--zoom-y', `${(y * 100).toFixed(2)}%`);
      lens.style.transform = `translate3d(${lensX.toFixed(1)}px,${lensY.toFixed(1)}px,0)`;
    };

    const queuePoint = (event) => {
      pendingPoint = { x: event.clientX, y: event.clientY };
      if (!frame) frame = window.requestAnimationFrame(render);
    };

    const show = (event) => {
      root.classList.add('is-active');
      pane.setAttribute('aria-hidden', 'false');
      if (event?.clientX != null) queuePoint(event);
    };

    const hide = () => {
      root.classList.remove('is-active');
      pane.setAttribute('aria-hidden', 'true');
      pendingPoint = null;
      if (frame) {
        window.cancelAnimationFrame(frame);
        frame = 0;
      }
    };

    source.addEventListener('pointerenter', show);
    source.addEventListener('pointermove', queuePoint);
    source.addEventListener('pointerleave', hide);
    source.addEventListener('focus', () => {
      root.classList.add('is-active');
      pane.setAttribute('aria-hidden', 'false');
      const rect = source.getBoundingClientRect();
      queuePoint({ clientX: rect.left + (rect.width / 2), clientY: rect.top + (rect.height / 2) });
    });
    source.addEventListener('blur', hide);
    source.addEventListener('keydown', (event) => {
      if (event.key === 'Escape') {
        hide();
        source.blur();
      }
    });
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
    initProductZoom();
    if ('requestIdleCallback' in window) {
      window.requestIdleCallback(syncRelease, { timeout: 2200 });
    } else {
      window.setTimeout(syncRelease, 900);
    }
  };

  if (document.readyState === 'complete') schedule();
  else window.addEventListener('load', schedule, { once: true });
})();
