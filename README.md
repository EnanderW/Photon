# Photon
A minecraft-server proxy like BungeeCord made in C.

This is my first project in C, it works on Windows and Linux. If you have any input/critique on the project, please
do not hesitate to send it to me, I would love to know what it is.

## Using Photon

NOTE: This project should DEFINITELY not be used seriously right now. It needs to be thoroughly tested first. I have only tested it
with two players and two minecraft-servers. I have no idea how fast it is, but I would really like to know. Problem is that I do not know how I would benchmark this.

In order to use Photon, you will have to build it yourself.

Requirements: (I think this is it?)
* CMake
* Libuv
* OpenSSL
* zlib

Make sure the CMake file is including and linking all the libraries correctly and then run CMake.
It should output a shared library file and an executable. The library file is the whole program and also used to make plugins, and the executable is the file you run for the proxy-program.
The executable is just a c-file that links with the library file. 

### Plugins
Photon is quite small, and does not come with much built-in. In order to customise Photon, you create plugins that hook into it. Photon shares
a similar API style to BungeeCord and Bukkit with events. Events is currently the only thing the API consists of.
(There are only a few events right now, but more might be added on demand or when I feel like it)

To add plugins to Photon, create a shared library (.dll on Windows) (.so on Linux) and put it inside the Photon plugins folder, which
is created when you run Photon.

Example plugin: https://github.com/EnanderW/photon-plugin-switch-server

Note: The init function is the function that is called when the plugin loads. It has to be named the same as the shared library file.

### Switching between minecraft servers
This is not built into Photon, rather it uses plugins to do so. I have created a plugin for the proxy and a bukkit plugin for the
minecraft server to make it work. These plugins are not as optimized as they could be, so if that is super important you could simply clone
the projects and optimize them for your needs.

Photon plugin: https://github.com/EnanderW/photon-plugin-switch-server <br>
Bukkit plugin: https://github.com/EnanderW/bukkit-plugin-switch-server