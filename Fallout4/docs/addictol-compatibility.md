# Addictol and Faster Decompression: what happened

*Investigation post-mortem · Faster Decompression 1.4.1 · 9 September 2026*

**This investigation traced the decompression conflict to a change in Addictol's GitHub code on 19 March 2026, 8 days after Faster Decompression's release. That change made Addictol hook the same inflate function already hooked by Faster Decompression. The public 1.2 release followed on 1 April, introducing the crash risk when both decompression features were enabled. The warning in Faster Decompression 1.4.1 makes that conflict visible and prevents overlapping hooks.**

## Why the investigation began

Compatibility testing before Faster Decompression's **11 March** release appeared successful with Addictol **1.1**, the latest published version at the time. That result informed the initial compatibility assessment when the combination was questioned in April. [Faster Decompression release date](https://www.nexusmods.com/fallout4/mods/102435), [Addictol old files](https://www.nexusmods.com/fallout4/mods/84214?tab=files).

A later Linux report, with a log dated **18 July**, showed Faster Decompression 1.3 on Fallout 4 1.11.221 failing to find its required decompression code:

```text
could not find any inflate cluster!
```

The log established failed activation. This investigation examined the source history and older binaries to determine what had changed since the earlier compatibility test.

## When compatibility changed

| Date | What changed |
| --- | --- |
| **Before 19 March** | Addictol accelerated two calls in the form-loading code. It left the shared `inflate` function entry untouched, allowing Faster Decompression's separate scan to find it. |
| **19 March 2026** | Addictol replaced those call-site hooks with a hook on the shared `inflate` function itself, for both OG and NG/AE. Both mods now targeted that entry. |
| **1 April 2026** | The first listed Addictol **1.2** release became publicly available with the shared-function rewrite. |

The code change is visible in [Addictol's 19 March commit](https://github.com/Dear-Modding-FO4/Addictol/commit/c451e4100563976fbb0d25af9180599efe0d68ce); the release date is in the [Nexus archive](https://www.nexusmods.com/fallout4/mods/84214?category=archived&tab=files). The earlier test could not establish compatibility with this later implementation.

## What the overlap could do

With the older implementations, initialization order could cause two different problems:

- **Addictol first:** its hook changed the bytes Faster Decompression 1.2/1.3 expected. Faster Decompression's scanner could reject the modified function and install no decompression hooks. The game could continue, with the failure visible only in the log.
- **Faster Decompression first:** the older Addictol implementation could overwrite Faster Decompression's hook. Its saved fallback could contain an incorrectly copied relative jump, sending execution to the wrong address when fallback was needed. Some decodes could succeed, while a later fallback could crash the game.

Addictol's [3 August source change](https://github.com/Dear-Modding-FO4/Addictol/commit/189bc43e4ec8db3f736993ca448e2e9405959383) adds a check for changes to the game's function. If Faster Decompression hooked it first, Addictol builds containing this check skip the Addictol decompression hook and leave Faster Decompression's hook in place. Only one decompression feature runs; their benefits are not combined. This has however not been released until now.

## Why 1.4.1 now warns

Players should know when Faster Decompression is inactive. Version 1.4.1 checks Addictol's effective `bLibDeflate` setting before installing any hooks, including when Addictol will load later.

If the setting is enabled, Faster Decompression displays the warning and installs none of its hooks. Startup can continue after **OK**. The plugin does not edit Addictol's settings. This avoids relying on DLL order and gives players a clear resolution.

**To keep both mods installed and use Faster Decompression**, set this in `Data/F4SE/Plugins/AddictolCustom.toml`, then restart:

```toml
[Patches]
bLibDeflate = false
```

If `[Patches]` already exists, edit the setting there. Addictol's other features can remain enabled according to their own compatibility requirements. Addictol Crash Logger alone does not trigger this warning.
