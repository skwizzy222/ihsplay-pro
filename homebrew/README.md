# Homebrew Channel — IHSplay Pro

Add this repository URL in **Homebrew Channel → Settings → Repositories**:

```
https://cdn.jsdelivr.net/gh/skwizzy222/ihsplay-pro@main/homebrew/repo.json
```

(If that fails, try the GitHub raw mirror — often blocked on TVs:)

```
https://raw.githubusercontent.com/skwizzy222/ihsplay-pro/main/homebrew/repo.json
```

Then find **Steam Remote Play** (`org.ihsplay.pro`) and install.

## Notes

- Does not replace upstream `org.mariotaku.ihsplay` — installs as a separate app.
- Uses jsDelivr CDN so TVs that cannot reach `raw.githubusercontent.com` still work.
- After each release, update `homebrew/repo.json` + ship the IPK under `homebrew/`.
