#include "plugin.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

static void set_null_termination(char *str, const char *ending) {
    char *start = strstr(str, ending);
    if (start == NULL) {
        printf("Wrong\n");
        return;
    }
    *(start) = 0;
}

static bool str_ends_with(const char *str, const char *ending, int ending_length) {
    const int str_len = strlen(str);
    if (str_len < ending_length) return false;

    char *check = str + (str_len - ending_length);
    for (int i = 0; i < ending_length; i++)
        if (check[i] != ending[i]) return false;

    return true;
}

#if defined(WIN32)
#include <libloaderapi.h>
void load_plugin(const char *plugin_name, const char *dll_file) {
    printf("Loading plugin '%s'\n", plugin_name);
    HMODULE lib = LoadLibraryA(dll_file);
    if (!lib) {
        fprintf(stderr,"Failed to load plugin '%s'\n", plugin_name);
        return;
    }

    char init_func_name[100];
    sprintf(init_func_name, "init_%s", plugin_name);

    plugin_init init = (plugin_init) GetProcAddress(lib, init_func_name);
    if (!init) {
        fprintf(stderr,"Failed to init plugin '%s'\n", plugin_name);
        FreeLibrary(lib);
        return;
    }

    init();
}

void load_plugins(const char *plugin_directory) {
    DIR *dfd;
    struct dirent *dir;

    dfd = opendir(plugin_directory);
    if (!dfd) {
        printf("Creating plugins directory..\n");
        mkdir(plugin_directory);
        return;
    }

    while ((dir = readdir(dfd)) != NULL) {
        if (str_ends_with(dir->d_name, ".dll", 4)) {
            char plugin_name[50];
            memcpy(plugin_name, dir->d_name, strlen(dir->d_name));
            set_null_termination(plugin_name, ".dll");

            char full_path[7 + 50]; // plugins folder + plugin name
            sprintf(full_path, "plugins/%s", dir->d_name);
            load_plugin(plugin_name, full_path);
        }
    }

    closedir(dfd);
}

#else
#include <dlfcn.h>

void load_plugin(const char *plugin_name, const char *dll_file) {
    printf("Loading plugin '%s'\n", plugin_name);
    void *lib = dlopen(dll_file, RTLD_NOW);
    if (!lib) {
        fprintf(stderr, "Failed to load plugin '%s'\n", plugin_name);
        return;
    }

    char init_func_name[100];
    sprintf(init_func_name, "init_%s", plugin_name);

    plugin_init init = (plugin_init) dlsym(lib, init_func_name);
    if (!init) {
        fprintf(stderr,"Failed to init plugin '%s'\n", plugin_name);
        dlclose(lib);
        return;
    }

    init();
}

#include <sys/stat.h>
void load_plugins(const char *plugin_directory) {
    DIR *dfd;
    struct dirent *dir;

    dfd = opendir(plugin_directory);
    if (!dfd) {
        printf("Creating plugins directory..\n");
        mkdir(plugin_directory, 0777);
        return;
    }

    while ((dir = readdir(dfd)) != NULL) {
        if (str_ends_with(dir->d_name, ".so", 3)) {
            char plugin_name[50];
            memcpy(plugin_name, dir->d_name, strlen(dir->d_name));
            set_null_termination(plugin_name, ".so");

            char full_path[7 + 50]; // plugins folder + plugin name
            sprintf(full_path, "plugins/%s", dir->d_name);
            load_plugin(plugin_name, full_path);
        }
    }

    closedir(dfd);
}

#endif
