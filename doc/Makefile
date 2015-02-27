# Mycelium Entropy User Manual
#
# Requires Asciidoctor: http://asciidoctor.org/

all: me.html

me.html: me.adoc docinfo.html
	asciidoctor $<

clean:
	rm -f me.html

.PHONY: all clean
