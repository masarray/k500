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
      if (!release?.tag) return;

      const label = document.getElementById('release-label');
      if (label) label.textContent = `Stable ${release.tag}`;
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
