cmake_minimum_required(VERSION 3.16)
project(Photon C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(Photon src/main.c)
add_library(PhotonAPI SHARED
        src/mc_server.c
        src/mc_server.h
        src/util/config.c
        src/util/config.h
        src/util/conversion.c
        src/util/conversion.h
        src/proxy/playerconnection.h
        src/proxy/serverconnection.c
        src/proxy/serverconnection.h
        src/proxy/proxy.c
        src/proxy/proxy.h
        src/bridge/bridge.c
        src/bridge/bridge.h
        src/session/session.c
        src/session/session.h
        src/util/hashmap.c
        src/util/hashmap.h
        src/util/linkedlist.c
        src/util/linkedlist.h
        src/player/player.c
        src/player/player.h
        src/player/uuid.h
        src/stage/stage.h
        src/stage/player/playerhandshake.c
        src/stage/player/playerlogin.c
        src/stage/playerstage.h
        src/proxy/connectionrequest.c
        src/proxy/connectionrequest.h
        src/util/http.c
        src/util/http.h
        src/stage/server/serverlogin.c
        src/stage/player/playerplay.c
        src/stage/server/serverplay.c
        src/multiplex/multiplex.h
        src/multiplex/multiplexdispatcher.c
        src/multiplex/multiplexworker.c
        src/proxy/proxycontext.h
        src/stage/serverstage.h
        src/session/settings.c
        src/session/settings.h
        src/stage/player/playerstatus.c
        src/util/types.h
        src/util/plugin.c
        src/util/plugin.h
        src/phapi/phapi.h
        src/phapi/event/event.c
        src/phapi/event/event.h
        src/workercontext.c
        src/workercontext.h
        src/util/arraylist.c
        src/util/arraylist.h)

if(WIN32)
    target_link_libraries(PhotonAPI uv z libcrypto libssl)
else()
    target_link_libraries(PhotonAPI uv z crypto ssl)
endif()

target_link_libraries(Photon PhotonAPI)
set_target_properties(PhotonAPI PROPERTIES PREFIX "")