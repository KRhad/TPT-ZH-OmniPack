# Official TPT save compatibility corpus

Only fixtures with recorded upstream provenance belong here. The stable gate
accepts `.cps` and `.stm` files, hashes each file, and requires a real load,
simulate, save, and reload probe result. This README is deliberately not a
fixture; an empty corpus yields `NOT_TESTED` and blocks the stable channel.

The corpus may instead live outside the repository and be passed to
`tools/release_1_1_0.ps1 -OfficialSaveCorpus <directory>`. The directory must
contain `provenance.json`; corpus files are never added to a release ZIP.

The manifest schema is `omnipack-official-tpt-save-corpus-v1`:

```json
{
  "schema": "omnipack-official-tpt-save-corpus-v1",
  "corpus_id": "official-tpt-upstream-<revision>",
  "source_repository": "https://github.com/The-Powder-Toy/The-Powder-Toy",
  "source_revision": "<full-40-character-git-commit>",
  "retrieved_at": "YYYY-MM-DD",
  "redistribution": {
    "status": "local_only_not_for_redistribution",
    "basis": "<recorded license or redistribution boundary>"
  },
  "files": [
    {
      "path": "relative/path/example.cps",
      "sha256": "<64-hex-sha256>",
      "source_locator": "https://github.com/The-Powder-Toy/The-Powder-Toy/blob/<revision>/relative/path/example.cps"
    }
  ]
}
```

`redistribution.status` is either `local_only_not_for_redistribution` or
`redistribution_permitted`. Every fixture must be listed exactly once, its hash
must match, and its source locator must bind to the recorded official commit.
