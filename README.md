# loxocoin
Ledger-based cryptocurrency

This is my attempt to create a cryptocurrency in C++ from scratch. It 
works a bit differently to bitcoin in that it uses a ledger which is 
updated by "deltas" containing transactions. It's kind of like ethereum 
without the bloat (I was not aware of how etherium worked when I wrote 
this code, otherwise I wouldn't have bothered). 

Like all projects, I started with the intention of coding elegantly and 
well, but as I got bored I ended up cutting corners and it's ended up in 
its current state. Ultimately I had to stop as I do not have enough time 
to get the network code all working. It works on 2 computers. Beyond that 
there are problems. 

I am officially abandoning this project and releasing it under the LG-A 
licence (https://pastebin.com/Qf8rAb82). 

If this is interesting to you, please consider a bitcoin donation: 
1N8E7PiVjS1NGPRxQhKrVcssAxQoJB8hyU

# How to use

install libsodium and libevent

compile with: 

g++ -g -Wall -Werror utils.cpp ledger.cpp transaction.cpp blockdelta.cpp block.cpp wallet.cpp packethandlerinterface.cpp miner.cpp node.cpp user.cpp netcom.cpp loxocoin.cpp -L/usr/local/lib -lsodium -levent -pthread -o loxocoin

you will need to place at least one other host ip in the "hosts" file

to start a new "blockchain":

delete the file "block" if it exists

run ./loxocoin n

For more details of modes, etc. see loxocoin.cpp.
