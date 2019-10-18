# RC-2019-20

GROUP 20 - FS SERVER and CLIENT

This project consisted in both the creation of a small forum server and a client application.

It is organized as follows:

    Current folder:
    ----user
        ---*user.c (Main source file for client application)
    ----fs
        ---*fs.c (Main source file for server application)
    ----util
        ---*util.c (Source file with functions useful to both applications)
    *Makefile 

To compile the project one should execute the instruction -- make -- in the current folder;
To execute the forum server application on should execute -- ./server -- in the current folder;
To execute the client application on should execute -- ./client -- in the current folder;

**The server application follows the guidelines provided for organization of folders**
When in execution, the server application will create a folder named "topics" in which it will
place the folders with its topics (up to 99 topics). Each topic folder has
If the folder "topics" already exists, its contents become automatically available. 