IDIR =../openrtx/src/imbe_vocoder
CC=g++
CFLAGS=-I$(IDIR)

ODIR=imbe_vocoder
LDIR =../lib

LIBS=-lm

IMBE=aux_sub.cc basicop2.cc ch_decode.cc ch_encode.cc dc_rmv.cc \
     decode.cc dsp_sub.cc encode.cc imbe_vocoder.cc imbe_vocoder_impl.cc \
     math_sub.cc pe_lpf.cc pitch_est.cc pitch_ref.cc \
     qnt_sub.cc rand_gen.cc sa_decode.cc sa_encode.cc \
     sa_enh.cc tbls.cc uv_synt.cc v_synt.cc v_uv_det.cc

# _DEPS = hellomake.h
# DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = Module17_Host.o $(IMBE)
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Module17_Host: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

