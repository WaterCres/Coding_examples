CPPFLAGS=
CXXFLAGS= $(INC) -g -O2 -Wall -Wextra $(WNO) -std=c++20 -fconcepts-diagnostics-depth=10 $(SANFLAGS)
LDFLAGS=$(SANFLAGS)
LIBS=
INC=-I./src

WNO=-Wno-error=unused-variable -Wno-unused-parameter -Wno-error=unused-but-set-variable -Wno-error=sign-compare
SANFLAGS=-fsanitize=address,undefined,leak


REPORT=report.tex

.PHONY: all
all: main.bin topo.bin

topo.bin: ./test/topotest.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

main.bin: main.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

report:
	pdflatex -shell-escape $(REPORT)
	pdflatex -shell-escape $(REPORT)

.PHONY: clean
clean:
	rm -f *.o main.bin

# 