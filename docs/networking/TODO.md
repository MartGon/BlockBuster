# TODO - Networking

## TO DO

- Setup scene in client
    - Make simple map
        - This requires redoing serialization - DONE
        - Refactor to use this App::Client::Map in project - DONE
    - Use App::Clinet::Map in client - DONE
    - Render and calculate delta time - DONE
    - Make simple *player* entity, a cube with a given position and move dir. Move in random dirs - DONE
    - Decide different sampling/packet rates.
- Creat Enet wrapper classes
    - Server
    - Client
- Networking
    - Server and Clent Startup
    - Design server/client packet structs (tagged unions): 
        - Client Request
        - Server accept
        - Client Move Command 
        - Server Move Update
    - Handle client requests and send accept packet
    - Send player positions and update in client
    - Rust Proxy
    - Interpolation
    - Ray Cast packet
    - Lag compensation
    - Client prediction

