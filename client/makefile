CXX = g++
CXXFLAGS = -std=c++17 -O3 -pthread -Wall
LDFLAGS = -pthread

OBJS = functions/methods/tcpflood.o \
       functions/methods/httpflood.o \
       functions/tools/dnshandler.o \
       functions/tools/ping.o \
       client.o

TARGET = client

all: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean