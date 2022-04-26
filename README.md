# Block Buster

## Description
*Block Buster* is a minimalist multiplayer voxel first-person shooter. It features 4 different game modes, 7 different weapons, its own game map creation tool and match making server.

## Screenshots

# ![gameplay-1](https://github.com/MartGon/BlockBuster/blob/main/docs/game/itchio/imgs/Gameplay1.png?raw=true)
# ![gameplay-2](https://github.com/MartGon/BlockBuster/blob/main/docs/game/itchio/imgs/Gameplay2.png?raw=true)
# ![editor](https://github.com/MartGon/BlockBuster/blob/main/docs/editor/itchio/maps/Editor.png?raw=true)
# ![menu](https://github.com/MartGon/BlockBuster/blob/main/docs/game/itchio/imgs/Menu.png?raw=true)

## Playing

Click [here](https://defu.itch.io/block-buster) to download a Windows build from itch.io. 

## Map Editor

You can find more detailed information about the Editor application by clicking [here](https://defu.itch.io/blockbuster-editor).

## Match Making Server

Unlike this project, the Match Making Server is made in Rust. Click [here](https://github.com/MartGon/BlockBuster-MatchMaking) to check its repo.

## Build Requirements (Client/Editor)

In order to build properly the client or editor application, you will need the following libraries:

- SDL2 (https://www.libsdl.org/download-2.0.php)
- OpenGL Development libraries (https://www.khronos.org/opengl/)

And the following tools:

- CMake (https://cmake.org/)
- C++17 compatible compiler

## Build steps (Linux)

0. Install the required libraries
1. Clone this repository
2. Create a build directory

`mkdir build && cd build`

3. Run CMake

`cmake ..`

4. Run Make

`make -j 4`


## Software libraries used in this project

- glad (https://glad.dav1d.de/)
- glm  (https://glm.g-truc.net/0.9.9/glm)
- ImGui (https://github.com/ocornut/imgui)
- stb (https://github.com/nothings/stb)
- freetype (https://freetype.org/)
- enet (http://enet.bespin.org/)
- openal-soft (https://github.com/kcat/openal-soft)
- json (https://github.com/nlohmann/json)
- zip (https://github.com/kuba--/zip)
- argparse (https://github.com/p-ranav/argparse)
- base64 (https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp/index)
- http-lib (https://github.com/yhirose/cpp-httplib)
- doctest (https://github.com/doctest/doctest)
- result (https://github.com/oktal/result)

## About

This whole project took exactly one year to complete. Started on 23rd April 2021.