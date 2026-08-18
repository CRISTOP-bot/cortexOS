# Third-party components

The following components are included as Git submodules. Their source trees
remain separately licensed; the top-level CortexOS GPL does not replace their
licenses.

| Component | Location | Upstream license | Notes |
|---|---|---|---|
| GNU Bash 5.3 | `third_party/bash/` | GPL-3.0 | Source mirror pinned by the Bash integration. |
| OpenRC | `third_party/openrc/` | BSD-2-Clause | Official OpenRC source pinned for the CortexOS port. |
| Fastfetch | `third_party/fastfetch/` | MIT | Official Fastfetch source pinned for the CortexOS port. |
| Original id Software Doom source | `third_party/doom/` | GPLv2 | Source-only submodule pinned for the CortexOS port; no IWAD/PWAD game data is included. |
| Archinstall | `tools/archinstall/upstream/` | GPL-3.0-only | Upstream installer sources vendored as the basis for the CortexOS adapter. |

The authoritative notices, copyright statements, and full license texts are
inside each submodule. Initialize them before inspecting or redistributing the
complete source tree:

```bash
git submodule update --init --recursive
```

If a future component is linked or copied into the kernel, its license must be
reviewed before merging and added to this table. Do not remove or rewrite
upstream copyright notices.
