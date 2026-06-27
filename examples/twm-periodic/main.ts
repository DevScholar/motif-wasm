import { createEmX11 } from '@em-x11/index.js';
import { launchTwm } from '@em-x11/runtime/twm-launch.js';
import { stageXbitmaps } from '@em-x11/runtime/xbitmaps-stage.js';

const emX11 = await createEmX11({ width: 1024, height: 768 });

stageXbitmaps(emX11);

await launchTwm(emX11);

const periodic = emX11.child_process.spawn('/artifacts/periodic/periodic', {
  thisProgram: 'periodic',
});
await periodic.ready;

console.log('[motif-wasm:twm-periodic] booted twm + periodic');
