# OpenOCD 0.11 compatibility shim.
# Cortex-Debug 1.12+ calls CDSWOConfigure using tpiu init/names (OpenOCD 0.12+).
# This file redefines the proc using the old tpiu config command and must be
# listed first in launch.json configFiles so it overrides the built-in version.
proc CDSWOConfigure { CDCPUFreqHz CDSWOFreqHz CDSWOOutput } {
    tpiu config internal $CDSWOOutput uart off $CDCPUFreqHz $CDSWOFreqHz
}
