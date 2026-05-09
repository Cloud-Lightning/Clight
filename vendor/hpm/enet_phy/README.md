# ENET PHY Local Vendor

This directory vendors the HPM SDK ENET PHY component into the project framework.

Current active use:

- `rtl8211/`
  Used by the HPM5E31 RGMII ENET path
- `hpm_enet_phy.h`
- `hpm_enet_phy_common.h`

The current build no longer needs the HPM SDK `components/enet_phy` source directory for RTL8211 support.
