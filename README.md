# FileServer

An implemented file server with server and client functionality implemenations in C.

# Instructions

1. cd into the FileServer directory and run "make"
2. Run the server executable from the directory containing the files the user will compare against.
3. Run the client executable from the directory containing the clients' files.

# Functionality

- LIST: prints the names of all files contained in the directory that the server executable was run from.
- DIFF: prints the names of any files that are contained in the server directory that do not exactly match with any files from the client directory.
- PULL: adds all files identified by the DIFF command to the client directory.
- LEAVE: terminates the clients' connection with the server.
