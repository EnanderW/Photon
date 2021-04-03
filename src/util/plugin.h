#ifndef PHOTON_PLUGIN_H
#define PHOTON_PLUGIN_H

typedef void (*plugin_init)(void);

void load_plugins(const char *plugin_directory);
void load_plugin(const char *plugin_name, const char *dll_file);

#endif //PHOTON_PLUGIN_H
