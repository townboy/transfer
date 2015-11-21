
.PHONY : all clean

clean :
	rm -rf *.o trans

all : trans
	@echo "compile done!"

trans : main.cpp tcp.cpp tcp.h handle.cpp handle.h
	g++ -g main.cpp tcp.cpp handle.cpp -o trans

