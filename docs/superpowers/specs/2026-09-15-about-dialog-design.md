# About dialog refresh

Approved on 2026-09-15 in response to feedback that this fork still sends bug
reports to the original author.

## Behavior

- Preserve Sean Werkema's original copyright credit and the existing freeware
  and warranty information.
- Identify Johnny J. Andersen as the maintainer of the modernized edition.
- Describe the fork as an independently maintained continuation of classic
  SpaceMonger.
- Replace the old support email with two keyboard-accessible native buttons:
  Project website and Report a bug.
- Open `https://andedammen.dk/spacemonger` and
  `https://github.com/Daduck/spacemonger/issues` in the default browser.
- Show a localized error containing the destination URL if Windows cannot open
  the browser.
- Update English (US), English (UK), and French together, retaining the source
  files' legacy encoding and the classic dialog appearance.

GitHub Issues must be enabled on `Daduck/spacemonger` before the bug-report
destination is presented as available. No issue or message is submitted by the
application; the button opens the issue tracker.

## Implementation and verification

Update the About dialog resource, its MFC event handlers, resource identifiers,
and the existing language tables. Retain the current version and single-file
distribution. No new UI framework or themed common-control dependency is needed.

Build the x64 Release executable and run the existing nine CTest executables.
Inspect the resulting dialog and translations for clipping and verify both
fixed link destinations and shell failure handling. Check the final diff for
unrelated changes and verify the GitHub repository setting after enabling it.

## Observed validation (2026-09-15)

- x64 Release configure/build succeeded; all nine existing CTests passed.
- Opened the actual executable's About dialog in all three languages and
  inspected dialog-only screenshots. Credits, description, warranty, and buttons
  fit; the French accents render correctly. Original application settings were
  restored after inspection.
- Reviewed the fixed URL destinations, native button message handlers, and
  localized shell failure path. Browser launch failure was not forced at runtime.
- The project website returned HTTP 200. Enabled GitHub Issues and confirmed
  `has_issues: true` with a fresh repository API read.
- The patch retains the existing version; publishing a new executable is a
  separate release action.
