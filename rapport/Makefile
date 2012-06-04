
TXTS=$(wildcard RapportZ3-*.txt)
TEXS=$(TXTS:.txt=.tex)
DVIS=$(TEXS:.tex=.dvi)
PDFS=$(TEXS:.tex=.pdf)

all: $(PDFS)

%.tex: %.txt
	iconv -t utf-8 $< | ~/.cabal/bin/pandoc --highlight-style=pygments -s -N -t latex -o $@

%.dvi: %.tex
	rm -f $*.aux
	latex $<

%.pdf: %.tex
	rm -f $*.aux
	pdflatex $<

clean:
	rm -f $(TEXS) $(DVIS) $(PDFS) *.aux *.log *.out