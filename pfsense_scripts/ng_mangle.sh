#!/bin/sh

# Loads ng_mangle kernel module
kldload ng_mangle 2> /dev/null

# Reattaches WAN interface that pfSense detached from netgraph during startup
php -r 'pfSense_ngctl_attach(".", "em0");'

# Enables the ng_mangle module for WAN interface
ngctl mkpeer em0: mangle lower lower 2> /dev/null
ngctl name em0:lower mangle0 2> /dev/null
ngctl connect em0: mangle0: upper upper 2> /dev/null

# Statically sets TTL of 'mangled' WAN interface
ngctl msg mangle0: set_ttl_lower 65

exit 0
