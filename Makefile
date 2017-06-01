CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=engine.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=render

all: $(SOURCES) $(EXECUTABLE)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm -f $(OBJECTS)
