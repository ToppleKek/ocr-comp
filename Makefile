PREFIX = /usr/local
INCLUDES = -I/usr/local/include/tesseract -I/usr/include/leptonica
LIBS = -locrad -L/usr/local/lib -lstdc++ -llept -ltesseract

.PHONY: all install uninstall

all: ocr-comp

ocr-comp: ocr-comp.c
	gcc ocr-comp.c ${INCLUDES} ${LIBS} -o ocr-comp

install: all
	cp ocr-comp ${PREFIX}/bin/

uninstall:
	rm ${PREFIX}/bin/ocr-comp