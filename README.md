# xfs2json
xfs2json is a command line tool to convert XFS files to JSON and vice versa. 

XFS is a binary serialization format used by CapCom's MT Framework 2.0 (and 3.0). This tool specifically targets XFS version 16 which is used in Monster Hunter Generations Ultimate. There are plans to add support for XFS version 19 (used in Monster Hunter World) in the future.

## Usage
The tool can be used via simple drag and drop or via command line. The command line usage is as follows:
```
Usage: xfs2json [-h] [-o <output>] <input>
Converts MT Framework XFS files to and from JSON.

    -h, --help            show this help message and exit
    -o, --output=<str>    Output file/directory
```
`input` can be both a file or a directory. If a directory is provided, all files in the directory will be converted (both ways).

## Building
To build the tool a c99 compliant compiler is required.
```
git clone https://github.com/Fexty12573/xfs2json.git
cd xfs2json
git submodule update --init --recursive
```

### Windows
#### Visual Studio
1. Open the folder `xfs2json` in Visual Studio.
2. Click on `Build` -> `Build All`.

#### Command Line
```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Linux
```
mkdir build && cd build
cmake ..
cmake --build . --config Release
```
or
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
