CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -O3 -std=c11 
SRC = main.c lib.c parser.c
TARGET = goenums

# sudo pacman -S mingw-w64-gcc
# sudo apt-get install mingw-w64

.PHONY: all go clean

all: c go-windows

# Pure C executable
c: $(SRC)
	$(CC) -DBUILD_MAIN=1 -D_DEFAULT_SOURCE=1 $(CFLAGS) -static  -o $(TARGET) $(SRC)
	strip $(TARGET)

# Go executable on Linux
go-linux: $(SRC)
	CGO_ENABLED=1 go build -o $(TARGET) -ldflags='-w -s -extldflags="-static"'

# Go executable on Windows (cross-compile)
go-windows: $(SRC)
	CC=x86_64-w64-mingw32-gcc CGO_ENABLED=1 GOOS=windows GOARCH=amd64 go build -o $(TARGET).exe -ldflags='-w -s -extldflags="-static"'

clean:
	rm -f $(TARGET) $(TARGET).exe

