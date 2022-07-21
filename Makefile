.POSIX:

NAME = stagit
VERSION = 0.9.4

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/man
DOCPREFIX = ${PREFIX}/share/doc/${NAME}

LIBGIT_INC = -I/usr/local/include
LIBGIT_LIB = -L/usr/local/lib -lgit2 -lmd4c-html

# use system flags.
STAGIT_CFLAGS = ${LIBGIT_INC} ${CFLAGS}
STAGIT_LDFLAGS = ${LIBGIT_LIB} ${LDFLAGS}
STAGIT_CPPFLAGS = -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -D_BSD_SOURCE

SRC = \
	stagit.c\
	stagit-index.c
COMPATSRC = \
	reallocarray.c\
	strlcat.c\
	strlcpy.c
BIN = \
	stagit\
	stagit-index\
	highlight
MAN1 = \
	stagit.1\
	stagit-index.1
DOC = \
	LICENSE\
HDR = compat.h

COMPATOBJ = \
	reallocarray.o\
	strlcat.o\
	strlcpy.o

OBJ = ${SRC:.c=.o} ${COMPATOBJ}

all: ${BIN}

.o:
	${CC} -o $@ ${LDFLAGS}

.c.o:
	${CC} -o $@ -c $< ${STAGIT_CFLAGS} ${STAGIT_CPPFLAGS}

dist:
	rm -rf ${NAME}-${VERSION}
	mkdir -p ${NAME}-${VERSION}
	cp -f ${MAN1} ${HDR} ${SRC} ${COMPATSRC} ${DOC} \
		${NAME}-${VERSION}
	# make tarball
	tar -cf - ${NAME}-${VERSION} | \
		gzip -c > ${NAME}-${VERSION}.tar.gz
	rm -rf ${NAME}-${VERSION}

${OBJ}: ${HDR}

stagit: stagit.o ${COMPATOBJ}
	${CC} -o $@ stagit.o ${COMPATOBJ} ${STAGIT_LDFLAGS}

stagit-index: stagit-index.o ${COMPATOBJ}
	${CC} -o $@ stagit-index.o ${COMPATOBJ} ${STAGIT_LDFLAGS}

highlight:
	cp highlight.py highlight

clean:
	rm -f ${BIN} ${OBJ} ${NAME}-${VERSION}.tar.gz

install: all
	# installing executable files.
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BIN} ${DESTDIR}${PREFIX}/bin
	for f in ${BIN}; do chmod 755 ${DESTDIR}${PREFIX}/bin/$$f; done

uninstall:
	# removing executable files.
	for f in ${BIN}; do rm -f ${DESTDIR}${PREFIX}/bin/$$f; done

.PHONY: all clean dist install uninstall
