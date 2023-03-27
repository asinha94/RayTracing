CC = g++
GDB = gdb
CONVERT = convert
CXXFLAGS = -O0 -g -std=c++17
TARGET = raytracer
PPM = output.ppm
JPEG = output.jpg


all: $(TARGET)
$(TARGET): $(TARGET).o
	$(CC) $(CXXFLAGS) $(TARGET).cpp -o $@

run: all
	./$(TARGET) $(PPM)

jpg: run
	$(CONVERT) $(PPM) $(JPEG)

gdb: run
	$(GDB) ./$(TARGET)

clean:
	rm -rf $(TARGET) $(TARGET).dSYM $(PPM) *.o a.out
