KIT := s60_30
CERT := dev

default : bin

bin :
	sake cert=$(CERT) kits=$(KIT)

bin-all :
	-rm -r build
	sake all release
	sake all release kits=s60_30,s60_31 cert=dev

apidoc :
	sake --trace cxxdoc

.PHONY : web

web :
	cp -a ../tools/web/hiit.css web/
	../tools/bin/txt2tags --target xhtml --infile web/index.txt2tags.txt --outfile web/index.html --encoding utf-8 --verbose -C web/config.t2t

HTDOCS := ../contextlogger.github.com
PAGEPATH := pyinbox
PAGEHOME := $(HTDOCS)/$(PAGEPATH)
DLPATH := $(PAGEPATH)/download
DLHOME := $(HTDOCS)/$(DLPATH)
MKINDEX := ../tools/bin/make-index-page.rb

release :
	-mkdir -p $(DLHOME)
	cp -a download/* $(DLHOME)/
	$(MKINDEX) $(DLHOME)
	cp -a web/*.css $(PAGEHOME)/
	cp -a web/*.html $(PAGEHOME)/
	chmod -R a+rX $(PAGEHOME)

upload :
	cd $(HTDOCS) && git add $(PAGEPATH) && git commit -a -m updates && git push

-include local/custom.mk

