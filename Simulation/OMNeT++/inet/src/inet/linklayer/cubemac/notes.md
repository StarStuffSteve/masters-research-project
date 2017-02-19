### Other Files Changed
D:\Amazon Drive\Sync\Home\TCD\MAI\Project\GitHub\Simulation\OMNeT++\inet\.oppfeatures:
D:\Amazon Drive\Sync\Home\TCD\MAI\Project\GitHub\Simulation\OMNeT++\inet\Makefile.vc:
D:\Amazon Drive\Sync\Home\TCD\MAI\Project\GitHub\Simulation\OMNeT++\inet\out\gcc-debug\src\.last-copts:
D:\Amazon Drive\Sync\Home\TCD\MAI\Project\GitHub\Simulation\OMNeT++\inet\src\inet\features.h:
<!--  -->
D:\Amazon Drive\Sync\Home\TCD\MAI\Project\GitHub\Simulation\OMNeT++\inet\src\inet\routing\aodv\AODVRouting.cc:
<!--  -->

### Simulation Manual

It is possible to build a whole source directory tree with a single Makefile. A source tree will generate a single output file (executable or library). A source directory tree will always have a Makefile in its root, and source files may be placed anywhere in the tree.

To turn on this option, use the opp_makemake --deep option. opp_makemake will collect all .cc and .msg files from the whole subdirectory tree, and generate a Makefile that covers all. If you need to exclude a specific directory, use the -X exclude/dir/path option. (Multiple -X options are accepted.)

An example:

_takes forever..._
$ opp_makemake -f --deep -X experimental -X obsolete
