# ITKMontage — Archived

This module has been ingested into the main ITK repository and is no longer
maintained as a remote module.

The code now lives in ITK proper at:

    Modules/Registration/Montage/

- Upstream ITK: https://github.com/InsightSoftwareConsortium/ITK
- Ingestion PR: https://github.com/InsightSoftwareConsortium/ITK/pull/6103
  (merged 2026-04-25, commit `1180a300d4`)

## What changed

Phase-correlation registration and image-mosaic stitching filters are now
built and released as part of ITK itself. Users no longer need to add this
repository as an external remote module.

To use the filters, build ITK with module `Module_Montage=ON` and link
against `ITKMontage`.

## History

The full history of this repository — including the original Insight
Journal contribution and all subsequent fixes — is preserved in this
repository's git log. New development happens in the ITK monorepo.

This repository has been placed in archive (read-only) mode.
