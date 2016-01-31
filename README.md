# NetPipes

The NetPipes are a simple way to send datas on the network. The NetPipes make the output of a program the input of another one in the network, in only one way or in two ways.

## How to compile it ?

Write these commands in a console :  
`git clone https://github.com/paly2/NetPipes`  
`cd NetPipes`  
`make`

## How to use it ?

### Using the bash pipes

For example, if you want to send to the `wc` command some lines which you type in the input of the client :  
Server:  
`sudo ./server | wc`  
Client:  
`./client`  
Now, you write some things in the client, and press Ctrl+C. You'll see the result of the wc command in the server.

### Using a child process

Bash pipes have a flaw : them can't redirect the standards stream in both directions. So, the NetPipes can create a child process and redirect its both streams. So, for example, if you want to use StockFish (UCI chess engine) with NetPipes, you can do on the client side :  
`./client stockfish`
