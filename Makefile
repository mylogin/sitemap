CXX := g++
CXX_FLAGS := -Wall -std=c++11 -g
EXECUTABLE := sitemap
PREFIX := /usr/local/bin
LIBRARIES :=
SSL := 0
ifeq ($(shell grep '\#define CPPHTTPLIB_OPENSSL_SUPPORT' sitemap.h -c),1)
	SSL := 1
endif
ifeq ($(OS),Windows_NT)
	LIBRARIES += -lws2_32
	ifeq ($(SSL),1)
		LIBRARIES += -lcrypt32 -lcryptui
	endif
	EXECUTABLE := $(addsuffix .exe,$(EXECUTABLE))
else
	LIBRARIES += -pthread
endif
ifeq ($(SSL),1)
	LIBRARIES += -lcrypto -lssl
endif

.PHONY: all clean install uninstall

all: $(EXECUTABLE)

$(EXECUTABLE): url.o html.o sitemap.o
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(LIBRARIES)

url.o: deps/url/url.cpp deps/url/url.hpp deps/url/string.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

html.o: deps/html/src/html5.cpp deps/html/include/html5.hpp
	$(CXX) $(CXX_FLAGS) -Ideps/html/include -c -o $@ $<

sitemap.o: sitemap.cpp sitemap.h deps/http/httplib.h
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

clean:
	-rm -f *.o $(EXECUTABLE)

install:
	install $(EXECUTABLE) $(PREFIX)

uninstall:
	rm -f $(PREFIX)/$(EXECUTABLE) 