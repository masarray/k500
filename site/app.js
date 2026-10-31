(() => {
  const releaseApi = '/api/release';

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

    const toleranceX = 180;
    const toleranceY = 90;
    const exitDelayMs = 240;
    let frame = 0;
    let pendingPoint = null;
    let hideTimer = 0;

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

    const cancelHide = () => {
      if (!hideTimer) return;
      window.clearTimeout(hideTimer);
      hideTimer = 0;
    };

    const show = (event) => {
      cancelHide();
      root.classList.add('is-active');
      pane.setAttribute('aria-hidden', 'false');
      if (event?.clientX != null) queuePoint(event);
    };

    const hide = () => {
      cancelHide();
      root.classList.remove('is-active');
      pane.setAttribute('aria-hidden', 'true');
      pendingPoint = null;
      if (frame) {
        window.cancelAnimationFrame(frame);
        frame = 0;
      }
    };

    const scheduleHide = () => {
      if (hideTimer) return;
      hideTimer = window.setTimeout(() => {
        hideTimer = 0;
        hide();
      }, exitDelayMs);
    };

    const insideTolerance = (event) => {
      const rect = source.getBoundingClientRect();
      return (
        event.clientX >= rect.left - toleranceX &&
        event.clientX <= rect.right + toleranceX &&
        event.clientY >= rect.top - toleranceY &&
        event.clientY <= rect.bottom + toleranceY
      );
    };

    source.addEventListener('pointerenter', show);
    document.addEventListener('pointermove', (event) => {
      if (!root.classList.contains('is-active')) return;
      if (insideTolerance(event)) {
        cancelHide();
        queuePoint(event);
        return;
      }
      scheduleHide();
    }, { passive: true });

    source.addEventListener('focus', () => {
      cancelHide();
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
    window.addEventListener('blur', hide);
  };

  const syncRelease = async () => {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 3200);

    try {
      const response = await fetch(releaseApi, {
        headers: { Accept: 'application/json' },
        cache: 'no-store',
        signal: controller.signal
      });
      if (!response.ok) return;

      const release = await response.json();
      if (!/^v\d+\.\d+\.\d+$/.test(release?.tag || '')) return;

      // No visible content hydration: HTML labels remain stable and same-origin
      // /download routes redirect to verified latest stable even without JS.
      // Do not trust arbitrary URLs from metadata as executable download targets.
      const downloadUrl = release.downloadUrl;
      const legacyExe = `SonKuPik-K500-${release.tag}-Windows-${(() => {
        const [major, minor, patch] = release.tag.slice(1).split('.').map(Number);
        return major > 1 || (major === 1 && (minor > 1 || (minor === 1 && patch >= 3)));
      })() ? 'Smart-Installer' : 'Setup'}.exe`;
      const shortExe = 'SonkuPik-k500-setup.exe';
      const shortZip = 'SonkuPik-k500-setup.zip';
      const legacyZip = `SonKuPik-K500-${release.tag}-Windows.zip`;
      const exeFilename = release.downloadFilename || legacyExe;
      const zipFilename = release.zipFilename || legacyZip;
      const verifiedReleaseUrl = (url, filename) => {
        if (typeof url !== 'string') return false;
        try {
          const parsed = new URL(url);
          return parsed.protocol === 'https:' && parsed.host === 'github.com' &&
            parsed.pathname === `/masarray/k500/releases/download/${release.tag}/${filename}` &&
            !parsed.search && !parsed.hash && !parsed.username && !parsed.password;
        } catch {
          return false;
        }
      };
      const validExeName = exeFilename === shortExe || exeFilename === legacyExe;
      if (!validExeName || !verifiedReleaseUrl(downloadUrl, exeFilename)) return;
      // ZIP is selected ONLY when explicitly listed as a canonical uploaded
      // GitHub Release asset. Never direct the user to a guessed ZIP URL.
      const verifiedZip = release.zipAvailable === true &&
        (zipFilename === shortZip || zipFilename === legacyZip) &&
        verifiedReleaseUrl(release.zipUrl, zipFilename);
      const preferredUrl = verifiedZip ? release.zipUrl : downloadUrl;
      document.querySelectorAll('[data-release-preferred]').forEach((element) => {
        element.href = preferredUrl;
      });
      document.querySelectorAll('[data-release-download]').forEach((element) => {
        element.href = downloadUrl;
      });
      // Visible copy is deliberately static: asynchronous release metadata must
      // never rewrite headings, button text, or insert checksum text after paint.
      // Only invisible href targets are upgraded. Same-origin 302 routes already
      // handle the latest stable before this script runs or if it is blocked.
      document.querySelectorAll('[data-zip-checksum]').forEach((element) => {
        if (verifiedZip) {
          element.href = 'https://github.com/masarray/k500/releases/download/' +
            release.tag + '/' + zipFilename + '.sha256';
        }
      });
      document.querySelectorAll('[data-release-page]').forEach((element) => {
        element.href = `https://github.com/masarray/k500/releases/tag/${release.tag}`;
      });
      document.querySelectorAll('[data-release-checksums]').forEach((element) => {
        element.href = `https://github.com/masarray/k500/releases/download/${release.tag}/` +
          (exeFilename === shortExe ? shortExe + '.sha256' : 'SHA256SUMS.txt');
      });

      document.querySelectorAll('[data-release-aria-template]').forEach((element) => {
        const template = element.getAttribute('data-release-aria-template');
        if (template) element.setAttribute('aria-label', template
          .replace(/\\{tag\\}/g, release.tag)
          .replace(/\\{version\\}/g, release.version || release.tag.slice(1)));
      });
    } catch {
      // Same-origin download routes still resolve the latest stable release even if metadata sync fails.
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
