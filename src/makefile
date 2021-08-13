IDIR=../include
CC=g++-9.2
CPPFLAGS=-I$(IDIR)

ODIR=obj

_DEPS=vma.h pte_t.h frame_t.h process.h pager.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ=process.o pager.o mmu.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

mmu: $(OBJ)
	$(CC) -o $@ $^ $(CPPFLAGS)

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~