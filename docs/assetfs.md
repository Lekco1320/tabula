# AssetFS Layout

AssetFS is a read-only resource partition for files used by the firmware at runtime.

The first version uses fixed directories and file names. It does not use a manifest.

## Mount Point

AssetFS is mounted at:

```text
/assets
```

## Directory Layout

```text
/assets/
  fonts/
  images/
  icons/
```

## Fonts

Font files are stored in:

```text
/assets/fonts/
```

EGF font files use the `.egf` extension.

The default font path is:

```text
/assets/fonts/default.egf
```

Named fonts are resolved by file name:

```text
<name> -> /assets/fonts/<name>.egf
```

## Notes

This layout only defines where resource files are placed. File formats such as EGF are defined separately.
