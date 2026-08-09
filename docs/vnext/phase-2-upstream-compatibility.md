# 1.0.2 latest upstream compatibility refresh

## Outcome

```text
TARGET_VERSION=1.0.2
BASE_COMMIT=430b3bd2868c17ba3d15c9a4580289173bcc79a6
CHECKED_DATE=2026-08-10
UPSTREAM_STABLE_VERSION=100.1 build 400
UPSTREAM_STABLE_REFERENCE=refs/tags/v100.1.400
UPSTREAM_TAG_OBJECT=c8be5165b7289706e17473bcfb38de112e78f140
UPSTREAM_STABLE_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_MASTER_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
GITHUB_LATEST_RELEASE=v100.1.400
GITHUB_RELEASE_TARGET=master
GITHUB_RELEASE_PUBLISHED_UTC=2026-08-08T03:15:47Z
OFFICIAL_WEBSITE_DOWNLOAD_STATUS=200
BRANCH_ONLY_FETCH=PASS
TAG_FETCH=YELLOW_LOCAL_HISTORICAL_TAG_COLLISION
UPSTREAM_COMMITS_SINCE_100_1_MERGE=0
LOCAL_VS_UPSTREAM=214 local-only / 0 upstream-only
CHANGE_IMPACT=NONE
ADAPTATION_REQUIRED=false
UPSTREAM_REGRESSION=GREEN
V1_0_2_GATE=GREEN
NEXT_VERSION=1.0.3
```

## Primary-source refresh

The official download page returned HTTP 200 and displayed version `100.1`.
The official GitHub latest-release endpoint reported non-draft, non-prerelease
`v100.1.400`, targeting `master`, published on 2026-08-08. The official remote
resolved both the annotated tag's peeled commit and `master` to
`d768aeb89acad986bd252d7e904bf44bb374545f`.

The direct sources are:

- <https://powdertoy.co.uk/Download.html>
- <https://github.com/The-Powder-Toy/The-Powder-Toy>
- <https://github.com/The-Powder-Toy/The-Powder-Toy/releases/tag/v100.1.400>

`git fetch official --prune` completed successfully. A separate
`git fetch official --tags --prune` safely refused to overwrite the unrelated
local historical tag `v99.5.394`; no tag was forced, deleted or replaced. The
current `v100.1.400` tag and branch reference were independently resolved with
`ls-remote`, so this isolated historical collision does not conceal a current
upstream delta.

## Change impact

The second parent of the local `ba30cd2e6` 100.1 merge is exactly
`d768aeb89`. The refreshed official `master` is the same commit, and
`git rev-list ba30cd2e6..official/master` reports zero commits. Current local
HEAD is a descendant of official master, with `0 upstream-only` and `214
local-only` commits. Therefore there is no new upstream change to classify under
Simulation, Air, Thermal, Element, Reaction, Lua, Save, UI, Render, Build,
Performance or Bugfix.

The existing 13-commit 100.1 classification remains in
[`upstream-diff.md`](upstream-diff.md). The current final runtime regression
continues to pass the upstream 100.1 Lua boundary fixture (`lua_bounds=11`) and
nine isolated OPS save/load round trips. Since the 1.0.1 milestone commit changed
documentation only after the final source validation, no production behavior
changed between the validated `97d2fc2c1` client and this refresh baseline.

## Gate and rollback

No source merge or production adaptation was required. The latest applicable
upstream is already integrated, the existing build/Lua/save evidence remains
applicable, and the 1.0.2 gate is GREEN.

Rollback for this audit is simply its documentation commit; there is no code delta
to revert. The historical `v99.5.394` tag collision remains YELLOW and must be
resolved only by an explicit Git-maintenance decision, never by force overwrite.
