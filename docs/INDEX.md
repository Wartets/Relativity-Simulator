# Documentation Index

This directory contains the formal technical documentation for the Relativity-Simulator.

## Core Documents

- [DESCRIPTION.md](DESCRIPTION.md)  
  System-level specification, [scientific scope](DESCRIPTION.md#1-global-vision-philosophy--core-objectives), [runtime modes](DESCRIPTION.md#1-global-vision-philosophy--core-objectives), [spacetimes](DESCRIPTION.md#4-spacetime-catalog--coordinate-representations), [relativistic hydrodynamics](DESCRIPTION.md#6-relativistic-hydrodynamics-grhdgrmhd--accretion-systems), and [validation scenarios](DESCRIPTION.md#12-verification-criteria--benchmark-protocols).

- [ARCHITECTURE.md](ARCHITECTURE.md)  
  Software architecture, [subsystem hierarchy](ARCHITECTURE.md#2-core-subsystems--computational-hierarchy), [render pipeline](ARCHITECTURE.md#29-render-pipeline), [capture subsystem](ARCHITECTURE.md#210-capture-subsystem), [profiling](ARCHITECTURE.md#211-profiling-logging-and-persistence), [constants engine](ARCHITECTURE.md#213-physical-constants-engine), [concurrency model](ARCHITECTURE.md#3-concurrency-execution-pipeline--scheduling), [scheduling](ARCHITECTURE.md#31-deterministic-simulation-scheduler), and [precision design](ARCHITECTURE.md#4-precision-architecture--numerical-stability).

- [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md)  
  Implementation-oriented component overview, [design constraints](TECHNICAL_MANUAL.md#11-design-constraints), [renderer back ends](TECHNICAL_MANUAL.md#29-rendering-back-ends), [metric usage table](TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine), [integrator selection rules](TECHNICAL_MANUAL.md#runtime-use-of-the-integrator-selection), [orchestration](TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface), and [interface modules](TECHNICAL_MANUAL.md#212-interface-support-modules).

- [MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md)  
  Governing equations, [metric definitions](MATHEMATICAL_FORMULATION.md#1-spacetime-metrics--line-elements), [geodesics](MATHEMATICAL_FORMULATION.md#2-geodesic-equations--curvature-invariants), [post-Newtonian terms](MATHEMATICAL_FORMULATION.md#3-post-newtonian-pn-n-body-dynamics), [relativistic hydrodynamics](MATHEMATICAL_FORMULATION.md#4-relativistic-hydrodynamics-grhdgrmhd), [polarized transport](MATHEMATICAL_FORMULATION.md#6-polarized-radiative-transfer), [uncertainty propagation](MATHEMATICAL_FORMULATION.md#7-uncertainty-quantification-formulation), [ray tracing and disk emission](MATHEMATICAL_FORMULATION.md#8-image-formation-in-the-renderer), [body shading](MATHEMATICAL_FORMULATION.md#84-celestial-body-shading), and [motion script mathematics](MATHEMATICAL_FORMULATION.md#9-motion-script-mathematics).

- [FILE_FORMATS.md](FILE_FORMATS.md)  
  [YAML scenario schema](FILE_FORMATS.md#1-declarative-scenario-definition-yaml), [FITS 4.0 images and cubes](FILE_FORMATS.md#2-flexible-image-transport-system-fits), [custom HDF5 containers](FILE_FORMATS.md#3-hierarchical-data-format-h5), [VTK PolyData geometry](FILE_FORMATS.md#4-visualization-toolkit-polydata-vtp), [SPICE SPK and HORIZONS ingestion](FILE_FORMATS.md#5-planetary-ephemeris--spice-integration), [image codecs](FILE_FORMATS.md#6-image-output-formats), [capture session structure](FILE_FORMATS.md#7-capture-session-directory), [telemetry recordings](FILE_FORMATS.md#8-telemetry-recording-files), [motion script configuration](FILE_FORMATS.md#9-motion-script-and-capture-settings-files), and [persistent settings](FILE_FORMATS.md#10-persistent-configuration-files).

- [CLI_REFERENCE.md](CLI_REFERENCE.md)  
  [Standalone executables](CLI_REFERENCE.md#1-executable-binaries), [interactive REPL commands](CLI_REFERENCE.md#2-interactive-terminal-commands-repl), [performance presets](CLI_REFERENCE.md#25-command-processing-and-performance-presets), [navigation modes and keybindings](CLI_REFERENCE.md#3-graphical-interface--interactive-controls), and [headless batch exporter](CLI_REFERENCE.md#4-headless-batch-exporter-headless_exporter).

- [TREE.md](TREE.md)  
  Repository layout.

## Asset Licensing References

- [../assets/sky/ambientcg/LICENCE.txt](../assets/sky/ambientcg/LICENCE.txt)
- [../assets/sky/eso/LICENCE.txt](../assets/sky/eso/LICENCE.txt)
- [../assets/earth/solarsystemscope/LICENCE.txt](../assets/earth/solarsystemscope/LICENCE.txt)
