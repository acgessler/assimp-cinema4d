Cinema4D (.c4d) support for Assimp through Melange
------

Melange is Maxon(tm)'s proprietary interfacing library to read and write C4D files and it is a necessary dependency for assimp-cinema4d. For this reason, this code lives in a separate repository to make clear that it has proprietary dependencies and is therefore not fully free software (and its use is subject to the Melange terms of use).

To build:
 * clone from https://github.com/acgessler/assimp-cinema4d
 * download melange (v6) from www.plugincafe.com and put it into `contrib/` so that` contrib/Melange/_melange/includes` and `contrib/Melange/_melange/lib/WIN`are available.
 * run cmake, enable the `ASSIMP_BUILD_NONFREE_C4D_IMPORTER` flag
 * build either `DEBUG` or `RELEASE` with *VC9, VC10 or VC11* (only these will work as these are targets supported by melange)
 
Currently no other targets other than Windows with MSVC are supported.
