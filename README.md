# IHSplay Pro

Steam-styled **Steam Remote Play** client for LG webOS TVs (fork of [IHSplay](https://github.com/mariotaku/ihsplay)).

## Install on TV (Homebrew Channel)

1. Open **Homebrew Channel → Settings → Add repository**
2. Paste:

```
https://raw.githubusercontent.com/skwizzy222/ihsplay-pro/main/homebrew/repo.json
```

3. Find **Steam Remote Play** (`org.ihsplay.pro`) → Install

This is a **separate app** from upstream IHSplay (`org.mariotaku.ihsplay`).

## What's new

- Steam-inspired UI (navy / `#66c0f4` / green PLAY)
- Cancel while connecting / pairing
- Streaming PIN pad
- Add computer by IP (unicast discovery)
- Video keyframe recovery fix
- Safer gamepad hotplug
- Persistent device identity

## Build webOS IPK (WSL)

```bash
./tools/webos/setup_ndk.sh
./tools/webos/build_ipk_wsl.sh
# → dist/org.ihsplay.pro_*_arm.ipk
```

## License

GPL-3.0 (same as upstream IHSplay).
