<!--
Thanks for contributing to OpenRTX!

Fill in the sections below and delete any that genuinely don't apply. Text in
comments like this one won't appear in your PR.

-->

### Summary

<!-- What does this change do, and why? One or two sentences is fine. -->

### Related issue

<!--
Fixes #___            (a bug this closes)
-->

### Design notes and alternatives

<!--
If this is a large change, open an issue to agree the approach BEFORE
writing the code. CONTRIBUTING.md asks for this, and it'll surely save you 
rounds in the code review.

Delete this section for small fixes.

Design agreed in #___ (for anything large, per CONTRIBUTING.md)
-->

### Testing performed

<!--
Required. Describe what you actually did: unit tests added, `meson test`
results, manual steps, what you observed on the air.

A PR with this section empty will be sent back, because a reviewer cannot
verify firmware behaviour by reading the diff.
-->

**Tested on:**

- [ ] Linux emulator
- [ ] MD-3x0 (TYT MD-380 / MD-390)
- [ ] MD-UV3x0 (TYT MD-UV380 / UV390)
- [ ] MD-9600
- [ ] GDx (Radioddity GD-77 / Baofeng DM-1801)
- [ ] DM-1701
- [ ] CS7000 / CS7000-PLUS
- [ ] RT-4D
- [ ] Module17
- [ ] ttwrplus
- [ ] Not tested on hardware — **help wanted**

<!--
Don't have the radio this touches? Tick "help wanted" and say which target
needs coverage. CI builds a flashable binary for every target on every PR,
so another contributor or user can test it for you. That is a normal and
welcome state for a PR to be in — an untested PR that says so is far easier
to review than one that is silent about it.
-->

### Changelog entry

<!--
One line for the release notes, or "none" for internal changes. Example:

    M17: fix callsign encoding for callsigns containing a slash
-->

### Checklist

- [ ] This PR is one logical change; unrelated fixes are in separate PRs
- [ ] Commits are tidy; each one builds, no "fix typo" commits left in the history
- [ ] I have read [CONTRIBUTING.md](../blob/master/CONTRIBUTING.md)

<!--
Still working on this? Open it as a Draft and mark it Ready for review when
it's done — that way nobody spends a review round on a moving target.

After you push changes that address a review, use GitHub's "re-request
review" button. It is the clearest way to say the ball is back in the
reviewer's court.

Anyone can review a PR, not just maintainers. If you have the hardware and
the time, reviewing someone else's change is one of the most useful things
you can do here.
-->