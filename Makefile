CC = g++
MAKE = make
RM = rm -f

TARGET = SecondFileSystem
SOURCE = _main1.cpp
SUPPORT = BufferManager.cpp File.cpp FileManager.cpp FileSystem.cpp INode.cpp Kernel.cpp OpenFileManager.cpp SecondFileSystem.cpp
FLAGS = -std=c++11 -m32

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $@  $^ $(SUPPORT) $(FLAGS)


.PHONY: clean

clean:
	$(RM) $(TARGET) test.img img_out.png
