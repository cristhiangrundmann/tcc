.phony: all bib clean

all:
	-pdflatex tcc.tex

bib:
	-pdflatex tcc.tex
	-biber tcc
	-pdflatex tcc.tex
	-pdflatex tcc.tex

clean:
	-rm tcc.aux tcc.bbl tcc.bcf tcc.blg tcc.idx tcc.log tcc.lof tcc.lol tcc.loq tcc.lot tcc.toc tcc.loc tcc.locd tcc.run.xml tcc.pdf
