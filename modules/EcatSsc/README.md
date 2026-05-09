# EcatSsc Module

This module keeps the framework-facing EtherCAT C++ wrapper classes and status snapshot API.

Included files:

- `EcatSscStack.hpp`
- `EcatSscSlave.hpp`

Application code should include this module instead of including vendored SSC or HPM EtherCAT port headers directly.

Vendored EtherCAT C sources, SSC files, and board-specific glue now live under `Clight/vendor/hpm/ethercat/`.
