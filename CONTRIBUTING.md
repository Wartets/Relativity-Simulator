# Contributing to Relativity-Simulator

Thank you for your interest in contributing to Relativity-Simulator. We welcome bug reports, documentation improvements, new features, performance work, and scientific validation contributions.

Before contributing, please read the project documentation first:
- [README.md](README.md)
- [docs/INDEX.md](docs/INDEX.md)
- [docs/DESCRIPTION.md](docs/DESCRIPTION.md)
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/TECHNICAL_MANUAL.md](docs/TECHNICAL_MANUAL.md)
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)

## Community expectations and behavior

This project is committed to a welcoming, respectful, and professional environment. By participating in this repository, you agree to uphold the standards described in the [Code of Conduct](CODE_OF_CONDUCT.md).

We expect contributors to:
- be respectful and constructive in discussions,
- assume good intent and give feedback in a helpful way,
- focus on the technical or scientific issue rather than personal differences,
- avoid harassment, discrimination, trolling, or disruptive behavior,
- keep discussions relevant to the project and the issue being addressed.

If a contribution or discussion fails to meet these expectations, maintainers may edit, reject, or close it as needed.

## How to contribute

### 1. Start with the existing documentation
Before opening a bug report or feature request, check whether the behavior is already described in the docs or in open issues. We strongly suggest reviewing:
- the project overview in [README.md](README.md),
- the architecture and implementation notes in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md),
- the mathematical and scientific scope in [docs/MATHEMATICAL_FORMULATION.md](docs/MATHEMATICAL_FORMULATION.md).

This helps avoid duplicate work and keeps discussions grounded in the project’s intended design.

### 2. Open useful issues
Use GitHub Issues to report bugs, suggestion, or enhancement ideas.

A good issue should include:
- a clear title,
- a concise description of the problem or request,
- steps to reproduce the issue,
- expected behavior vs. actual behavior,
- environment details (OS, compiler, CMake version, CPU/GPU if relevant),
- relevant logs, command output, or screenshots,
- a minimal example when possible.

Examples of helpful issues:
- “Crash occurs when running headless_exporter with Schwarzschild scenario on Windows MSVC”
- “Documentation example for the CLI reference is out of date”
- “Suggested improvement: add support for additional EOS validation case”

Examples of less useful issues:
- vague reports without reproduction steps,
- issues that are not related to the project,
- requests for large feature additions without context or design rationale.

Before opening a new issue, please search existing issues to confirm it has not already been reported.

### 3. Propose changes via pull requests
Pull requests are the preferred way to contribute code or documentation changes.

Please:
- keep pull requests focused on a single concern,
- create a branch from the main branch,
- write clear commit messages,
- explain the reason for the change and the expected outcome,
- include tests or validation steps when relevant,
- update documentation when behavior or interfaces change.

A good PR description usually includes:
- summary of the change,
- motivation and context,
- files touched,
- validation performed,
- any known limitations or follow-ups.

### 4. Respect project scope
This repository is a scientific simulation and rendering project with substantial architecture and algorithmic scope. Please keep contributions aligned with that purpose.

Large changes should usually be discussed in advance via an issue or by opening a draft pull request. This helps ensure the design fits the project’s goals and avoids unnecessary churn.

## Development and validation expectations

Contributions that affect build behavior, runtime logic, scientific models, or rendering should ideally include validation evidence.

Typical expectations:
- ensure the project still builds with supported toolchains,
- run the relevant unit tests or validation commands,
- respect the project’s C++23/CMake structure,
- preserve deterministic behavior and reproducibility where relevant,
- avoid breaking existing scenarios, CLI behavior, or documentation references.

Common validation commands:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

For this project, scientific correctness and reproducibility are important. If you change numerical algorithms, physics models, or rendering behavior, include a short explanation of the validation performed.

## Documentation and design guidance

If you change behavior, public interfaces, or user-facing workflows, please update the relevant documentation in the `docs/` directory and/or `README.md`.

When applicable, contributions should align with:
- the repository’s technical architecture,
- the scientific formulation and validation goals,
- the CLI and file-format documentation,
- the expected reproducibility and engineering constraints of the project.

## Pull request checklist

Before submitting a pull request, please confirm:
- [ ] I searched for existing issues or pull requests related to this change.
- [ ] I kept the scope focused and limited to the stated objective.
- [ ] I updated documentation if user-facing behavior or interfaces changed.
- [ ] I ran the relevant validation commands.
- [ ] I ensured the change follows the repository’s coding and scientific expectations.
- [ ] I followed the project’s [Code of Conduct](CODE_OF_CONDUCT.md).

## Contact and support

For bug reports, feature requests, and code contributions, use GitHub Issues and Pull Requests. For project-level conduct or moderation concerns, refer to the [Code of Conduct](CODE_OF_CONDUCT.md).

Thank you for helping improve Relativity-Simulator.
