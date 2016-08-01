The typical usage model for monitoring a program with BlackBox involves 3 basic steps:

1. Generate traces of the program by running it several times in the BlackBox virtual machine (built from this repository).
2. Merge the traces together into a *trusted profile* of the program, using the Java CFI tools for x86 binaries:
  * [cfi-common](https://github.com/uci-plrg/cfi-common)
  * [cfi-x86-graph](https://github.com/uci-plrg/cfi-x86-graph)
  * [cfi-x86-merge](https://github.com/uci-plrg/cfi-x86-merge)
3. Monitor the program by running it in the BlackBox VM with the *trusted profile*.

The following sections give more details about each step. For build instructions, see the `doc/build.md` file in each repository.

#### Generate Traces

Use the [bb](clients/blackbox/util/debug/bb) script to launch a program in BlackBox, for example notepad:

    bb notepad
    
The script requires environment variable `BLACKBOX_DATASET_DIR`, which can identify any directory you prefer. For applications such as Adobe PDF Reader that use a security sandbox, putting the dataset directory under 

    <user-home>/AppData/LocalLow/
    
will allow BlackBox to write the trace files (the sandbox may prevent file writes to other areas of the filesystem).
