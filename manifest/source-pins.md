# External source pins

The project uses compatible Scarthgap sources. These source trees are external to this repository and are not vendored or committed here.

| Component | Ref | Expected commit |
|---|---|---|
| Poky / Yocto | Tag `yocto-5.0.21` | `4fb7016a07507683359ebd620f1951fd5ea44bfa` |
| `meta-raspberrypi` | Branch `scarthgap` | `018d44467c4f09eb63f8ea5e86e85d755250cf89` |\n| `meta-openembedded` (`meta-oe`) | Branch `scarthgap` | `2db6aece32c7f8ebf6015b468e1b4e21de072e3b` |

Verify local checkouts with:

```bash
git -C poky rev-parse HEAD
git -C meta-raspberrypi rev-parse HEAD\ngit -C meta-openembedded rev-parse HEAD
```

For a reproducible handoff, record any intentional pin changes here and update the build documentation. Keep all layer branches/commits compatible with the selected Yocto release.

The QEMU ARM64 baseline uses Poky core layers. The Raspberry Pi BSP layer is required for the later `raspberrypi4-64` build; it is not necessary to imply that the Pi image has already been built.
