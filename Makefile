TARGET = json-xml
prefix = /usr/local
BINDIR = $(prefix)/bin

LDLIBS += -lyajl

all: $(TARGET)

install: $(TARGET)
	install $(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -rf $(TARGET)
