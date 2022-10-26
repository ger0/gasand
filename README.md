# gasand  
Multiplayer sand falling game  

https://user-images.githubusercontent.com/28660288/198022932-3b9e9fdf-d7e6-4fd1-91b7-1365905c0cbb.mp4
  
Dependencies: SDL2  
Usage:
Server: 
> gasand-server \<port\>  
  
You can change the map dimensions as long as they don't exceed the value of 200  
> gasand-server \<port\> --set-size \<width\> \<height\>  
  
Client:  
> gasand \<address\> \<port\>  
  
You can change display dimensions above 400
> gasand \<address\> \<port\> --set-size \<width\> \<height\>  
