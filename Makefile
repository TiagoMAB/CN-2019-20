SOURCES = util/util.c fs/fs.c
OBJS = $(SOURCES:%.c=%.o)
SOURCES1 = util/util.c user/newuser.c
OBJS1 = $(SOURCES1:%.c=%.o)
CC   = gcc
CFLAGS = -std=gnu99 -I../
LDFLAGS=-lm
TARGET = server
TARGET1 = client

all: $(TARGET)
all: $(TARGET1)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(TARGET) $(LDFLAGS) 
$(TARGET1): $(OBJS1)
	$(CC) $(CFLAGS) $^ -o $(TARGET1) $(LDFLAGS) 

user/user.o: user/newuser.c util/util.h
fs/fs.o: fs/fs.c util/util.h
util/util.o: util/util.c util/util.h


$(OBJS):
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f $(OBJS) $(TARGET) $(OBJS1) $(TARGET1)