# Specs

This directory contains product and implementation specs for the ESP32 boat MFD.

Files:

- `00-findings.md` - current architecture/refactor findings.
- `01-visual-reference-model.md` - visual patterns from open-source marine
  projects and commercial MFD conventions.
- `02-layout-system-spec.md` - layout document, screen type, and widget
  addressing model.
- `03-screen-specs.md` - per-screen visualization and interaction specs.
- `04-layout-presets.md` - recommended default screen stacks for common users.
- `05-gesture-subsystem-spec.md` - asynchronous touch and gesture subsystem
  design.
- `06-ui-interactions.md` - user-facing interaction model and gesture behavior.
- `07-event-driven-mvc.md` - proposed event-driven MVC/MVU-lite architecture
  and migration path.
- `08-configuration-storage-sync.md` - configuration storage, source
  precedence, local/external synchronization, and conflict handling.

Use these specs when implementing the data-driven renderer, refining individual
screens, or adding Signal K plugin/device-management features.
