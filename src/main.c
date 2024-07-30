#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define VERSION_STRING "v1.3.2"

#define TextStartsWith(text, startsWith) !strncmp(text, startsWith, strlen(startsWith))

bool _DoSettingString(char *buf, char *settingName, char *fmt, char **settingProperty)
{
    if (!TextStartsWith(buf, settingName)) return false;

    *settingProperty = malloc(512);
    **settingProperty = 0;
    sscanf(buf, fmt, *settingProperty);
    *settingProperty = realloc(*settingProperty, strlen(*settingProperty) + 1);

    return true;
}

#define DoSettingString(settingName, settingProperty) _DoSettingString(buf, settingName, settingName " %s\n", &settingProperty)

void _DoSettingBool(char *buf, char *settingName, char *fmt, bool *settingProperty)
{
    char *str;

    if (!_DoSettingString(buf, settingName, fmt, &str)) return;

    *settingProperty = !!strcmp(str, "False");

    free(str);
}

#define DoSettingBool(settingName, settingProperty) _DoSettingBool(buf, settingName, settingName " %s\n", &settingProperty)

typedef struct HeSource {
    char *name;
} HeSource;

typedef enum {
    PROGRAM_EXECUTABLE,
    PROGRAM_LIBRARY,
    PROGRAM_SOURCEGROUP,
} ProgramType;

typedef struct HeProgram {
    char *name;
    HeSource *sources;
    int sourceCount;
    ProgramType type;
    char **sourceGroups;
    int sourceGroupCount;
} HeProgram;

typedef struct HeFile {
    char *defaultPlatform;
    char *raylibVersion;
    char *defaultDistDir;
    bool useRaylib;
    HeProgram *programs;
    int programCount;
    char **dependencies;
    int dependencyCount;
    char **extraCflags;
    int extraCflagCount;
    char **nativeDependencies;
    int nativeDependencyCount;
    char **distData;
    int distDataCount;
} HeFile;

HeFile file = {
    .defaultPlatform = "linux64-debug",
    .raylibVersion = "5",
    .defaultDistDir = ".",
};

typedef struct HePlatform {
    bool isDebug;
    bool isRelease;
    char *name;
    char *execExtension;
    char *libExtension;
    char *compiler;
    char *raylibLdFlag;
    char **extraCflags;
    int extraCflagCount;
} HePlatform;

HePlatform *platforms;
int platformCount;

void loaddefaultplatforms()
{
    platformCount = 3;
    platforms = malloc(sizeof(HePlatform) * platformCount);
    platforms[0] = (HePlatform){false, true, "linux64", "", ".so", "gcc", "-lraylib"};
    platforms[1] = (HePlatform){true, false, "linux64-debug", "-debug", "-debug.so", "gcc", "-lraylib"};
    platforms[2] = (HePlatform){false, true, "win64", ".exe", ".dll", "x86_64-w64-mingw32-gcc", "-lraylibdll"};
}

void parseheplatforms()
{
    FILE *heplatforms = fopen("HePlatforms", "r");
    char buf[512];
    int currentPlatform = 0;

    if (!heplatforms)
    {
        printf("using default platforms\n");
        loaddefaultplatforms();
        return;
    }

    fgets(buf, 512, heplatforms);
    while (!feof(heplatforms))
    {
        if (TextStartsWith(buf, "Platform"))
        {
            currentPlatform = platformCount;
            platformCount++;
            platforms = realloc(platforms, platformCount * sizeof(HePlatform));
            platforms[currentPlatform] = (HePlatform){0};
        }
        DoSettingString("Platform", platforms[currentPlatform].name);
        DoSettingString("ExecExtension", platforms[currentPlatform].execExtension);
        DoSettingString("LibExtension", platforms[currentPlatform].libExtension);
        DoSettingString("Compiler", platforms[currentPlatform].compiler);
        DoSettingString("RaylibLdFlag", platforms[currentPlatform].raylibLdFlag);
        DoSettingBool("IsRelease", platforms[currentPlatform].isRelease);
        DoSettingBool("IsDebug", platforms[currentPlatform].isDebug);
        if (TextStartsWith(buf, "ExtraCFlag"))
        {
            platforms[currentPlatform].extraCflagCount++;
            platforms[currentPlatform].extraCflags = realloc(platforms[currentPlatform].extraCflags, platforms[currentPlatform].extraCflagCount * sizeof(char *));
            DoSettingString("ExtraCFlag", platforms[currentPlatform].extraCflags[platforms[currentPlatform].extraCflagCount - 1]);
        }
        fgets(buf, 512, heplatforms);
    }

    fclose(heplatforms);
}

void parsehefile()
{
    FILE *hefile = fopen("HeFile", "r");
    char buf[512];
    int currentProgram = -1;
    int line = 0;

    fgets(buf, 512, hefile);
    while (!feof(hefile))
    {
        line++;
        DoSettingString("DefaultPlatform", file.defaultPlatform);
        DoSettingString("DefaultDistDir", file.defaultDistDir);
        DoSettingString("RaylibVersion", file.raylibVersion);
        DoSettingBool("UseRaylib", file.useRaylib);

        if (TextStartsWith(buf, "Program") || TextStartsWith(buf, "Library") || TextStartsWith(buf, "SourceGroup"))
        {
            currentProgram = file.programCount;
            file.programCount++;
            file.programs = realloc(file.programs, file.programCount * sizeof(HeProgram));
            file.programs[currentProgram].sourceCount = 0;
            file.programs[currentProgram].sources = NULL;
            file.programs[currentProgram].type = PROGRAM_EXECUTABLE;
            file.programs[currentProgram].sourceGroups = NULL;
            file.programs[currentProgram].sourceGroupCount = 0;
            if (TextStartsWith(buf, "Library")) file.programs[currentProgram].type = PROGRAM_LIBRARY;
            else if (TextStartsWith(buf, "SourceGroup")) file.programs[currentProgram].type = PROGRAM_SOURCEGROUP;
        }
        DoSettingString("Program", file.programs[currentProgram].name);
        DoSettingString("Library", file.programs[currentProgram].name);
        DoSettingString("SourceGroup", file.programs[currentProgram].name);

        if (TextStartsWith(buf, "Source") && !TextStartsWith(buf, "SourceGroup"))
        {
            if (currentProgram == -1)
            {
                printf("warning: line %d: source has no program attached\n", line);
                continue;
            }

            file.programs[currentProgram].sourceCount++;
            file.programs[currentProgram].sources = realloc(file.programs[currentProgram].sources, file.programs[currentProgram].sourceCount * sizeof(HeSource));
        }
        if (currentProgram != -1 && file.programs[currentProgram].sourceCount)
        {
            DoSettingString("Source", file.programs[currentProgram].sources[file.programs[currentProgram].sourceCount - 1].name);
        }

        if (TextStartsWith(buf, "UseSourceGroup"))
        {
            if (currentProgram == -1)
            {
                printf("warning: line %d: source group has no program attached\n", line);
                continue;
            }

            file.programs[currentProgram].sourceGroupCount++;
            file.programs[currentProgram].sourceGroups = realloc(file.programs[currentProgram].sourceGroups, file.programs[currentProgram].sourceGroupCount * sizeof(char*));

            DoSettingString("UseSourceGroup", file.programs[currentProgram].sourceGroups[file.programs[currentProgram].sourceGroupCount - 1]);
        }

        if (TextStartsWith(buf, "Dependency"))
        {
            file.dependencyCount++;
            file.dependencies = realloc(file.dependencies, file.dependencyCount * sizeof(char *));
        }
        DoSettingString("Dependency", file.dependencies[file.dependencyCount - 1]);

        if (TextStartsWith(buf, "NativeDependency"))
        {
            file.nativeDependencyCount++;
            file.nativeDependencies = realloc(file.nativeDependencies, file.nativeDependencyCount * sizeof(char *));
        }
        DoSettingString("NativeDependency", file.nativeDependencies[file.nativeDependencyCount - 1]);

        if (TextStartsWith(buf, "ExtraCFlag"))
        {
            file.extraCflagCount++;
            file.extraCflags = realloc(file.extraCflags, file.extraCflagCount * sizeof(char *));
        }
        DoSettingString("ExtraCFlag", file.extraCflags[file.extraCflagCount - 1]);

        if (TextStartsWith(buf, "DistData"))
        {
            file.distDataCount++;
            file.distData = realloc(file.distData, file.distDataCount * sizeof(char *));
        }
        DoSettingString("DistData", file.distData[file.distDataCount - 1]);

        fgets(buf, 512, hefile);
    }

    fclose(hefile);
}

void genmakefile()
{
    FILE *makefile = fopen("Makefile", "w");

    fprintf(makefile, "# Generated using Helium " VERSION_STRING " (https://github.com/tornadocookie/he)\n\n");

    fprintf(makefile, "PLATFORM?=%s\n", file.defaultPlatform);
    fprintf(makefile, "DISTDIR?=%s\n\n", file.defaultDistDir);
    fprintf(makefile, ".PHONY: all\n\n");
    if (file.useRaylib) fprintf(makefile, "RAYLIB_NAME=raylib%s-$(PLATFORM)\n\n", file.raylibVersion);
    
    for (int i = 0; i < platformCount; i++)
    {
        fprintf(makefile, "ifeq ($(PLATFORM), %s)\n", platforms[i].name);
        fprintf(makefile, "EXEC_EXTENSION=%s\n", platforms[i].execExtension);
        fprintf(makefile, "LIB_EXTENSION=%s\n", platforms[i].libExtension);
        fprintf(makefile, "CC=%s\n", platforms[i].compiler);
        if (file.useRaylib)
        {
            fprintf(makefile, "RAYLIB_DLL=%s\n", platforms[i].raylibLdFlag);
        }
        if (platforms[i].isDebug)
        {
            fprintf(makefile, "CFLAGS+=-g\n");
            fprintf(makefile, "CFLAGS+=-D DEBUG\n");
        }
        if (platforms[i].isRelease)
        {
            fprintf(makefile, "CFLAGS+=-O2\n");
            fprintf(makefile, "CFLAGS+=-D RELEASE\n");
        }
        fprintf(makefile, "CFLAGS+=-D EXEC_EXTENSION=\\\"%s\\\"\n", platforms[i].execExtension);
        fprintf(makefile, "CFLAGS+=-D LIB_EXTENSION=\\\"%s\\\"\n", platforms[i].libExtension);
        for (int j = 0; j < platforms[i].extraCflagCount; j++)
        {
            fprintf(makefile, "CFLAGS+=%s\n", platforms[i].extraCflags[j]);
        }
        fprintf(makefile, "endif\n\n");
    }

    fprintf(makefile, "PROGRAMS=");

    for (int i = 0; i < file.programCount; i++)
    {
        if (file.programs[i].type == PROGRAM_EXECUTABLE) fprintf(makefile, "%s%s", file.programs[i].name, i != file.programCount - 1 ? " " : "");
    }

    fprintf(makefile, "\nLIBRARIES=");

    for (int i = 0; i < file.programCount; i++)
    {
        if (file.programs[i].type == PROGRAM_LIBRARY) fprintf(makefile, "%s%s", file.programs[i].name, i != file.programCount - 1 ? " " : "");
    }

    for (int i = 0; i < file.nativeDependencyCount; i++)
    {
        fprintf(makefile, "\n\n%s_NAME=lib%s-$(PLATFORM)\n", file.nativeDependencies[i], file.nativeDependencies[i]);
        fprintf(makefile, "CFLAGS+=-Ilib/$(%s_NAME)/include\nCFLAGS+=-Wl,-rpath,lib/$(%s_NAME)/lib\n", file.nativeDependencies[i], file.nativeDependencies[i]);
        fprintf(makefile, "LDFLAGS+=-Llib/$(%s_NAME)/lib\nLDFLAGS+=-l%s", file.nativeDependencies[i], file.nativeDependencies[i]);
    }

    fprintf(makefile, "\n\nall: $(DISTDIR) $(foreach prog, $(PROGRAMS), $(DISTDIR)/$(prog)$(EXEC_EXTENSION)) $(foreach lib, $(LIBRARIES), $(DISTDIR)/$(lib)$(LIB_EXTENSION))");

    if (file.nativeDependencyCount || file.useRaylib || file.distDataCount)
    {
        fprintf(makefile, " deps\n\n");
        fprintf(makefile, "ifneq ($(DISTDIR), .)\n");
        fprintf(makefile, "deps:\n");
        fprintf(makefile, "\tmkdir -p $(DISTDIR)/lib\n");
        for (int i = 0; i < file.nativeDependencyCount; i++)
        {
            fprintf(makefile, "\tif [ -d lib/$(%s_NAME) ]; then cp -r lib/$(%s_NAME) $(DISTDIR)/lib/$(%s_NAME); fi\n", file.nativeDependencies[i], file.nativeDependencies[i], file.nativeDependencies[i]);
        }
        if (file.useRaylib)
        {
            fprintf(makefile, "\tif [ -d lib/$(RAYLIB_NAME) ]; then cp -r lib/$(RAYLIB_NAME) $(DISTDIR)/lib/$(RAYLIB_NAME); fi\n");
        }
        for (int i = 0; i < file.distDataCount; i++)
        {
            fprintf(makefile, "\tcp -r %s $(DISTDIR)\n", file.distData[i]);
        }
        fprintf(makefile, "else\n");
        fprintf(makefile, "deps:\n");
        fprintf(makefile, "endif\n");
    }

    fprintf(makefile, "\n$(DISTDIR):\n");
    fprintf(makefile, "\tmkdir -p $@\n");

    fprintf(makefile, "\nCFLAGS+=-Isrc\n");
    fprintf(makefile, "CFLAGS+=-Iinclude\n");
    for (int i = 0; i < file.dependencyCount; i++)
    {
        fprintf(makefile, "CFLAGS+=-Ilib/%s/src\n", file.dependencies[i]);
        fprintf(makefile, "CFLAGS+=-Ilib/%s/include\n", file.dependencies[i]);
    }
    fprintf(makefile, "CFLAGS+=-D PLATFORM=\\\"$(PLATFORM)\\\"\n");
    for (int i = 0; i < file.extraCflagCount; i++)
    {
        fprintf(makefile, "CFLAGS+=%s\n", file.extraCflags[i]);
    }
    fprintf(makefile, "\n");

    if (file.useRaylib)
    {
        fprintf(makefile, "CFLAGS+=-Ilib/$(RAYLIB_NAME)/include\nCFLAGS+=-Wl,-rpath,lib/$(RAYLIB_NAME)/lib\n\n");
        fprintf(makefile, "LDFLAGS+=-lm\nLDFLAGS+=-Llib/$(RAYLIB_NAME)/lib\nLDFLAGS+=$(RAYLIB_DLL)\n\n");
    }

    for (int i = 0; i < file.programCount; i++)
    {
        for (int j = 0; j < file.programs[i].sourceCount; j++)
        {
            fprintf(makefile, "%s_SOURCES+=src/%s\n", file.programs[i].name, file.programs[i].sources[j].name);
        }
        for (int j = 0; j < file.programs[i].sourceGroupCount; j++)
        {
            fprintf(makefile, "%s_SOURCES+=$(%s_SOURCES)\n", file.programs[i].name, file.programs[i].sourceGroups[j]);
        }
        if (file.programs[i].type != PROGRAM_SOURCEGROUP)
        {
            fprintf(makefile, "\n$(DISTDIR)/%s$(%s): $(%s_SOURCES)\n", file.programs[i].name, file.programs[i].type == PROGRAM_LIBRARY ? "LIB_EXTENSION" : "EXEC_EXTENSION", file.programs[i].name);
            fprintf(makefile, "\t$(CC) -o $@ $^ $(CFLAGS)%s $(LDFLAGS)\n\n", file.programs[i].type == PROGRAM_LIBRARY ? " -fPIC -shared" : "");
        }
        else
        {
            fprintf(makefile, "\n");
        }
    }

    fprintf(makefile, "clean:\n");
    for (int i = 0; i < file.programCount; i++)
    {
        if (file.programs[i].type == PROGRAM_SOURCEGROUP) continue;
        fprintf(makefile, "\trm -f $(DISTDIR)/%s$(EXEC_EXTENSION)\n", file.programs[i].name);
    }

    fprintf(makefile, "\nall_dist:\n");
    for (int i = 0; i < platformCount; i++)
    {
        fprintf(makefile, "\tDISTDIR=$(DISTDIR)/dist/%s PLATFORM=%s $(MAKE)\n", platforms[i].name, platforms[i].name);
    }

    fclose(makefile);
}

int main(int argc, char **argv)
{
    printf("He " VERSION_STRING "\n");
    parseheplatforms();
    parsehefile();
    genmakefile();
}

