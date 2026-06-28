import { createEmX11 } from '@em-x11/index.js';

const emX11 = await createEmX11({ width: 1024, height: 768 });

const periodic = emX11.child_process.spawn('/artifacts/periodic/periodic', {
  thisProgram: 'periodic',
});
await periodic.ready;

console.log('[motif-wasm:periodic] booted periodic solo');
