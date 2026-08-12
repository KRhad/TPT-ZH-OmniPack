# Third-party license summary

The canonical matrix is [`third-party-audit.md`](third-party-audit.md). First-round
selected roles are:

| Candidate | Decision |
|---|---|
| The Powder Toy upstream | `ADAPT` |
| SDL3 / SDL_GPU | `ADAPT` |
| SDL_shadercross | `ADAPT` |
| Cantera 3.2.0 | `DIRECT_REUSE` for optional offline tooling only |
| CoolProp 8.0.0 | `DIRECT_REUSE` for optional offline tooling only |
| Athena++ | `REFERENCE_ONLY` |
| hydro-cl-lua | `REFERENCE_ONLY` |
| TPT benchmark | `REFERENCE_ONLY` |
| TPT/GPU falling-sand parallelization experiments | `REFERENCE_ONLY` |
| NIST Chemistry WebBook SRD 69 | `REFERENCE_ONLY` |
| NASA NTRS document 19720017735 | `REFERENCE_ONLY` for one public acoustic feasibility value |
| OpenStax Chemistry | `ADAPT` for the attributed 1.0.8 carbon-combustion identity and enthalpy value |

No external solver, shader or database dump has been copied into production. The
1.0.8 runtime includes one attributed OpenStax CC BY 4.0 thermochemical value and
ships its attribution notice. An
otherwise permissive code license does not authorize redistribution of third-party
mechanisms or scientific data shipped alongside a tool.
