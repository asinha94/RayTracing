CC = g++
GDB = gdb
CXXFLAGS = -O0 -g -std=c++17
TARGET = raytracer
PPM = output.ppm


all: $(TARGET)
$(TARGET): $(TARGET).o
	$(CC) $(CXXFLAGS) $(TARGET).cpp -o $@

run: all
	./$(TARGET) $(PPM)

gdb: run
	$(GDB) ./$(TARGET)

clean:
	rm -rf $(TARGET) $(TARGET).dSYM $(PPM) *.o a.out
