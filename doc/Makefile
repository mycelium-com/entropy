# Mycelium Entropy User Manual
#
# Requires Asciidoctor: http://asciidoctor.org/

all: me.html verify.html

me.html: me.adoc printers.adoc docinfo.html
	asciidoctor $<

verify.html: verify.adoc docinfo.html
	asciidoctor $<

clean:
	rm -f me.html verify.html

.PHONY: all clean
