package main

// #include <string.h>
// #include "main.h"
// #include <stdlib.h>
import "C"
import (
	"os"
	"unsafe"
)

func main() {
	// convert os.Args to C style argv
	argc := C.int(len(os.Args))
	argv := make([]*C.char, argc)
	for i, arg := range os.Args {
		argv[i] = C.CString(arg)
		defer C.free(unsafe.Pointer(argv[i]))
	}

	// call C function
	C.run(argc, &argv[0])
}
