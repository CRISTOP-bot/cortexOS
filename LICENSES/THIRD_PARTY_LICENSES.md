# Third-party components

The following components are included as Git submodules. Their source trees
remain separately licensed; the top-level NucleOS GPL does not replace their
licenses.

| Component | Location | Upstream license | Notes |
|---|---|---|---|
| GNU Bash 5.3 | `third_party/bash/` | GPL-3.0 | Source mirror pinned by the Bash integration. |
| OpenRC | `third_party/openrc/` | BSD-2-Clause | Official OpenRC source pinned for the NucleOS port. |
| Fastfetch | `third_party/fastfetch/` | MIT | Official Fastfetch source pinned for the NucleOS port. |
| Archinstall | `tools/archinstall/upstream/` | GPL-3.0-only | Upstream installer sources vendored as the basis for the NucleOS adapter. |

The authoritative notices, copyright statements, and full license texts are
inside each submodule. Initialize them before inspecting or redistributing the
complete source tree:

```bash
git submodule update --init --recursive
```

If a future component is linked or copied into the kernel, its license must be
reviewed before merging and added to this table. Do not remove or rewrite
upstream copyright notices.
