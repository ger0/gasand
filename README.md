# gasand
Multiplayer sand falling game 

Dependencies: SDL2
Usage:
Server: 
> gasand-server <port> 
You can change the map dimensions as long as they don't exceed the value of 200
> gasand-server <port> --set-size <width> <height>

Client:
> gasand <address> <port> 
You can change display dimensions above 400 using --set-size
> gasand <address> <port> --set-size <width> <height>
