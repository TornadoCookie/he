#!/bin/sh

# Argument 1: program name

# Initialize src directory
mkdir src
printf "#include <stdio.h>\n\nint main()\n{\n    printf(\"Hello, world!\");\n    return 0;\n}\n" >> src/main.c

# Initialize lib directory
# TODO replace this with a location that will always work
mkdir lib
cp -r ~/c_stuff/OpenSC5/lib/raylib5.5-linux64-debug lib

# Initialize HeFile
printf "DefaultDistDir build\nDefaultPlatform linux64-debug\nRaylibVersion 5.5\nUseRaylib\n\nProgram $1\nSource main.c\n" >> HeFile
