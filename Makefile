TARGET = json-xml
LDLIBS += -lyajl

all: $(TARGET)

clean:
	rm -rf $(TARGET)
