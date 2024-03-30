CFLAGS  = -g
LFLAGS  =
CC      = g++
RM      = /bin/rm -rf
AR      = ar rc
RANLIB  = ranlib

LIBRARY = bin/libgtfs.a

LIB_SRC = src/gtfs.cpp

LIB_OBJ = $(patsubst %.cpp,%.o,$(LIB_SRC))

# pattern rule for object files
%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

all: $(LIBRARY) 

$(LIBRARY): $(LIB_OBJ)
	$(AR) $(LIBRARY) $(LIB_OBJ)
	$(RANLIB) $(LIBRARY)

$(LIB_OBJ) : $(LIB_SRC)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) $(LIBRARY) src/*.o tests/test tests/test1.txt tests/test2.txt tests/test3.txt
