VERSION  = 1.0
NAME     = sqsqtools
PKG      = $(NAME)-$(VERSION)
ARCHIVE  = $(PKG).tar.gz

incdir   = $(prefix)/include
libdir   = $(prefix)/lib
docdir   = $(prefix)/share/doc/$(NAME)

CFLAGS   := -Wall

RM       = rm -f
INSTALL  = install
BINDEST  = /sbin

EXEC     = sqsq-image
OBJS     = sqsq-image.o

DOCFILES = README.md LICENSE

all: $(EXEC)

$(EXEC): $(OBJS) $(LIB)
	$(CC) -o $@ $(OBJS) $(LIB) $(LDLIBS)

$(INSTALL):
	$(INSTALL) -m 755 sqsq-image $(DESTDIR)$(BINDEST)
	$(INSTALL) -m 755 sqsq-verify $(DESTDIR)$(BINDEST)

clean:
	$(RM) *.o sqsq-image

dist:
	git archive --format=tar.gz --prefix=$(PKG)/ -o ../$(ARCHIVE) v$(VERSION)
