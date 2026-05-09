# lwIP Local Vendor

This directory contains vendored Ethernet/lwIP port glue and the TCP echo sample app.

Layout:

- `upstream/src/`
  Local copy of the lwIP upstream core and headers previously compiled from the SDK middleware tree
- `hpm_lwip/`
  Local lwIP port/common glue previously stored under `app/vendor/hpm_lwip`
- `apps/lwip_tcpecho/`
  Local copy of the lwIP TCP echo sample app headers and source

Framework-facing wrappers remain under `Clight/modules/EthLwip/`.

The current build no longer needs the HPM SDK `middleware/lwip/src` source directory for the lwIP core.
