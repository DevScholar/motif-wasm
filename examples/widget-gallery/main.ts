const factory = (await import('/artifacts/widget-gallery/widget-gallery.js')).default;
await factory({
  thisProgram: 'widget-gallery',
  locateFile: (path: string) => `/artifacts/widget-gallery/${path}`,
});
