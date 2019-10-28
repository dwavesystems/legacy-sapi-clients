VERSION := $(shell head -n 1 $(dir $(lastword $(MAKEFILE_LIST)))/../version.txt)
CLIENTS := sapi-c-client-$(VERSION)-linux64.tar.gz \
           sapi-c-client-$(VERSION)-osx.tar.gz \
           sapi-c-client-$(VERSION)-win32.zip \
           sapi-c-client-$(VERSION)-win64.zip \
           sapi-matlab-client-$(VERSION)-linux64.tar.gz \
           sapi-matlab-client-$(VERSION)-osx.tar.gz \
           sapi-matlab-client-$(VERSION)-win32.zip \
           sapi-matlab-client-$(VERSION)-win64.zip \
           sapi-python-client-$(VERSION)-linux64.tar.gz \
           sapi-python-client-$(VERSION)-osx.tar.gz \
           sapi-python-client-$(VERSION)-win32.zip \
           sapi-python-client-$(VERSION)-win64.zip

prefix = /usr
DOC_DIR = $(DESTDIR)$(prefix)/share/qubist-clients

all: client-downloads.json

client-downloads.json: ../version.txt
	python gen_clients_json.py client-downloads.json

install: $(CLIENTS) client-downloads.json
	install -d "$(DOC_DIR)"
	install -m644 client-downloads.json "$(DOC_DIR)"
	for c in $(CLIENTS); do install -m644 "$$c" "$(DOC_DIR)" ; done

qubist-clients-$(VERSION).tar.gz: $(CLIENTS) client-downloads.json
	tar czf $@ --transform='s,^,qubist-clients/,' $^

rpm-source: qubist-clients-$(VERSION).tar.gz

clean:
	-rm -f client-downloads.json
	-rm -f qubist-clients-$(VERSION).tar.gz

.PHONY: all install clean rpm-source
