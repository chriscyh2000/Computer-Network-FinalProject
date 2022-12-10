# Computer-Network-FinalProject
The final project of Computer Network 2022 (@NTU) (Project phase 2)

### The Function of this BULLETIN BOARD
- Allow multiple users comment on the board 
- Users can register / login / log out on this website
- Both frontend and backend are developed in C++

### Other features
- Used `poll` to perform multiplexing
- Error Detection: 
    - When users try to login, the server will check if the password is correct
    - When users try to register a new account, the server will check if the account has been used
    - When users enter data, the server will ensure that no fields are left blank

### Usage
```shell
# in your server side
g++ -std=c++17 server.cpp -o server
# initialize the database and run the server
./server [PORT_NUMBER] --init
# if you don't want to initialize it, just use the following command
./server

# in your client side
g++ -std=c++17 client.cpp -o client
./client [BACKEND_IP]:[BACKEND_PORTNUMBER] [FRONTEND_PORTNUMBER]
```
After run all these command, you can use `http://[FRONTEND_IP]:[FRONTEND_PORTNUMBER]` to connect the website
For example, if you want to set up both frontend and backend at `linux15.csie.ntu.edu.tw`, you can use the following command on that machine.
```shell
g++ -std=c++17 server.cpp -o server
./server 8999
# use another shell
g++ -std=c++17 client.cpp -o client
./client 127.0.0.1:8999 8989
```
Now you can use `http://linux15.csie.ntu.edu.tw:8989` to browse the website.