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
To execute the forum server application on should execute -- ./server -- in the current
folder, with optional flag -p corresponding to the port number of the server that the clients
will connect with;
To execute the client application on should execute -- ./client -- in the current folder,
with optional flag -n corresponding to the IP address of the server and optional flag -p
corresponding to the port number of the server that the client will connect with;

**The server application follows the guidelines provided for organization of folders**
When in execution, the server application will create a folder named "topics" where it will
place the folders, each named with the topic name (up to 99 topics).

Each topic folder has folders with the name of the submitted questions (up to 99 questions) and
a text file with name "topic_UID.txt" where "topic" is the name of the topic. The file's
contents is a string corresponding to the user ID of the user who submitted that topic.

Each question folder contains the text file and image file corresponding to the question, a text
file with name "question_UID.txt" with the user ID of the user who submitted the question inside
and folders named "question_N" where N is the answer number.

Each answer folder contains the text file and image file corresponding to the question and a text
file with name "question_N_UID.txt" with the user ID of the user who submitted the answer inside.

If the folder "topics" already exists, its contents become automatically available.


When in execution, the client application will create a folder name "pid", where pid is the process
id of the client. Inside, the client will place folders, each named with the topic name (up to 99
topics).

Each topic folder will contain text and image files corresponding to the questions and their answers,
along with a *_UID.txt text file for each question and answer with the user ID of the user who
submitted that question/answer.

When the exit command is executed, the "pid" folder is deleted as well as its contents.