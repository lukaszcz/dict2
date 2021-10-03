INCLUDES =
METASOURCES = AUTO

dist_noinst_DATA = wfa.tex cache.tex

PDFS = $(subst .tex,.pdf,$(wildcard *.tex))

pdf-local: $(PDFS)

%.pdf : %.tex
	pdflatex $<
	pdflatex $<
	pdflatex $<
