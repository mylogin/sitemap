CXX := g++
CXX_FLAGS := -Wall -Wextra -std=c++11
EXECUTABLE := sitemap
PREFIX := /usr/local/bin
LIBRARIES := -pthread
ifeq ($(OS),Windows_NT)
	LIBRARIES += -lws2_32
	EXECUTABLE := $(addsuffix .exe,$(EXECUTABLE))
endif
ifeq ($(shell grep '\#define CPPHTTPLIB_OPENSSL_SUPPORT' sitemap.h -c),1)
	LIBRARIES += -lcrypto -lssl
endif

.PHONY: all clean install uninstall

all: $(EXECUTABLE)

$(EXECUTABLE): url.o html.o sitemap.o
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(LIBRARIES)

url.o: deps/url/url.cpp deps/url/url.hpp deps/url/string.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

html.o: deps/html/html5.cpp deps/html/html5.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

sitemap.o: sitemap.cpp sitemap.h deps/http/httplib.h
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

clean:
	-rm -f *.o $(EXECUTABLE)

install:
	install $(EXECUTABLE) $(PREFIX)

uninstall:
	rm -f $(PREFIX)/$(EXECUTABLE) 