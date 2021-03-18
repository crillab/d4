ROOTDIR    = .
EXEC      ?= d4
MROOT      = .

CSRCS      = $(wildcard *.cc */*.cc)
CHDRS      = $(wildcard *.hh */*.hh)

NAMEOBJDIR = objs
OBJDIR = $(shell mkdir -p $(NAMEOBJDIR); echo $(NAMEOBJDIR))
NOM = $(basename $(notdir $(CSRCS)))
COBJS = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(NOM)))

PCOBJS     = $(addsuffix p,  $(COBJS))
DCOBJS     = $(addsuffix d,  $(COBJS))
RCOBJS     = $(addsuffix r,  $(COBJS))

PATOH_PATH = patoh/libpatoh.a

CXX        = g++ -std=c++11
#CFLAGS    ?= -Wall -Wno-parentheses

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
LFLAGS     += -L/opt/local/lib -I/opt/local/include -lz -lgmpxx -lgmp patoh/libpatoh.a
endif
ifeq ($(UNAME), Darwin)
LFLAGS     += -I/opt/local/include -lz -lgmpxx -lgmp patoh_mac/libpatoh.a
endif



COPTIMIZE ?= -O3

CFLAGS    +=
LDFLAGS   +=
LFLAGS    +=

DEP 	  := $(COBJS:%.o=%.d)

.PHONY: s p d r rs clean

s:      $(EXEC)
p:      $(EXEC)_profile
d:      $(EXEC)_debug
r:      $(EXEC)_release
rs:     $(EXEC)_static

## Compile options
$(OBJDIR)/%.o:                    CFLAGS +=$(COPTIMIZE) -g -D DEBUG
$(OBJDIR)/%.op:                   CFLAGS +=$(COPTIMIZE) -pg -g -D NDEBUG
$(OBJDIR)/%.od:                   CFLAGS +=-O2 -g -D DEBUG -Wfatal-errors
$(OBJDIR)/%.or:                   CFLAGS +=$(COPTIMIZE) -g -D NDEBUG

## Link options
$(EXEC):                LFLAGS += -g
$(EXEC)_profile:        LFLAGS += -g -pg
$(EXEC)_debug:          LFLAGS += -g
$(EXEC)_static:         LFLAGS += --static

## Dependencies
$(EXEC):                $(COBJS)
$(EXEC)_profile:        $(PCOBJS)
$(EXEC)_debug:          $(DCOBJS)
$(EXEC)_release:        $(RCOBJS)
$(EXEC)_static:         $(RCOBJS)



$(OBJDIR)/%.or: */%.cc */%.hh
	@echo Compiling: $(subst $(MROOT)/,,$@)
	g++ -D STATIC_COMP $(CFLAGS) -MT $@ -MMD -MP -c -o $@ $<


$(EXEC)_static:
	@echo Linking: "$@ ( $(foreach f,$^,$(subst $(MROOT)/,,$f)) )"
	g++ -D STATIC_COMP $^ -o $@ $(LDFLAGS) $(LFLAGS)




## Build rule
$(OBJDIR)/%.o $(OBJDIR)/%.op $(OBJDIR)/%.od: */%.cc */%.hh
	@echo Compiling: $(subst $(MROOT)/,,$@)
	@$(CXX) $(CFLAGS) -MT $@ -MMD -MP -c -o $@ $<


## Linking rules (standard/profile/debug/release)
$(EXEC) $(EXEC)_profile $(EXEC)_debug $(EXEC)_release:
	@echo Linking: "$@ ( $(foreach f,$^,$(subst $(MROOT)/,,$f)) )"
	@$(CXX) $^ -o $@ $(LDFLAGS) $(LFLAGS)

all:

-include $(DEP)

## Clean rule
clean:
	rm -f $(SRC:.cc=.o) $(SRC:.cc=)
	rm -f *~
	rm -f */*~
	rm -rf $(OBJDIR)
	rm -f $(EXEC)*
