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
    
The script requires environment variable `BLACKBOX_DATASET_DIR`, which can identify any directory you prefer. For applications such as Adobe PDF Reader that use a security sandbox, put the dataset directory in the low-security area (provided by Windows) to allow BlackBox to write the trace files (the sandbox may prevent file writes to other areas of the filesystem):

    <user-home>/AppData/LocalLow/

BlackBox will write several files during the execution, for example:

1. **notepad.exe.process.\<timestamp\>.log**: internal log messages for development purposes.
2. **notepad.exe.module.\<timestamp\>.dat**: list of modules and their memory locations.
3. **notepad.exe.graph-node.\<timestamp\>.dat**: control flow graph nodes.
3. **notepad.exe.graph-edge.\<timestamp\>.dat**: intra-module control flow edges.
4. **notepad.exe.cross-module.\<timestamp\>.dat**: cross-module control flow edges.
5. **notepad.exe.meta.\<timestamp\>.dat**: metadata about the execution.
6. **notepad.exe.xhash.\<timestamp\>.dat**: lists the export name associated with each module hash identifier.
 
Other than the **process.log** file, this data is unreadable and should be analyzed using the Java tools. 

#### Generate *Trusted Profile* 

Move or copy the files to a convenient location and run the following sequence of commands (which can be found in the **/scripts** directories of the Java tools ([common](https://github.com/uci-plrg/cfi-common/tree/master/scripts), [graph](https://github.com/uci-plrg/cfi-x86-graph/tree/master/scripts), [merge](https://github.com/uci-plrg/cfi-x86-merge/tree/master/scripts)):

1. cd \<data-dir\> # where the above files are located
2. split-hashlogs # splits the files into a subdirectory for each execution
3. cd ..
4. mkdir -p cluster
5. transform-graphs -o cluster \<data-dir\>/*
6. mkdir -p logs
7. cs-train -o dataset.1 -n notepad -l logs/notepad.1.log cluster/* # only use `-n notepad` for the first merge
8. generate-monitor-data -o notepad.monitor.dat dataset.1

Copy notepad.monitor.dat to the `BLACKBOX_DATASET_DIR`.


