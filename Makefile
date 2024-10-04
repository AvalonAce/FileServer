#################################################################
##
## FILE:	Makefile
## PROJECT:	CNT 4007 Project 2 - Professor Traynor
## DESCRIPTION: Compile Project 2
##
#################################################################

client: client.cpp
	g++ -o client client.cpp -l"Ws2_32"

clean:
	rm -f client
