# Languages
CC       := gcc
PERL     := perl

COMPILE.c   = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc  = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@


# Directories
OBJDIR := obj
DEPDIR := dep
BOOSTDIR := /usr/include/boost


# Flags
DEPFLAGS      = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
OPTFLAGS     := -O0
INCLUDEFLAGS := -I../include
PROFFLAGS    := #-pg
DEFS         += 
CFLAGS       := -Wall -g $(PROFFLAGS) $(INCLUDEFLAGS) $(OPTFLAGS) $(DEFS)
CXXFLAGS     := $(CFLAGS) -std=c++11
LDFLAGS      := 

# Libraries
LDLIBS    := 
LOADLIBES := 


# Target names
TARGET    := 
LIBTARGET := libTbitMap.a
REV_MAJOR  := 0
REV_MINOR  := 1


# Directories to build
OBJDIR := obj
DEPDIR := dep


# Source files
LIBSRCS   := tbitmap.c mtrie3l.c date.c
SRCS      := 

# Object files
LIBOBJS   := $(addprefix $(OBJDIR)/,$(LIBSRCS:.c=.o))
OBJS      := $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))


$(TARGET): $(OBJS) $(LIBTARGET)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) $(PROF) -o $@

$(LIBTARGET): $(LIBOBJS)
	$(AR) $(ARFLAGS) $@ $^
	ranlib $@

.PHONY: perf
perf:
	$(MAKE) -f $(lastword $(MAKEFILE_LIST)) OPTFLAGS=-O3 DEFS=-DNODEBUG

.PHONY: clean
clean:
	rm -f $(TARGET) $(TARGET).exe $(LIBTARGET) $(OBJS) $(LIBOBJS) \
	$(DEPDIR)/*.d *.bak *.exe.* *~

date.c: tbitmap-revision.c $(filter-out date.c, $(SRCS)) $(wildcard *.h)
	sed -e "s/DATE-STRING/`date`/" \
	    -e "s/MAJOR/$(REV_MAJOR)/" \
	    -e "s/MINOR/$(REV_MINOR)/" tbitmap-revision.c > date.c


$(OBJDIR)/%.o : %.c
$(OBJDIR)/%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cpp
$(OBJDIR)/%.o : %.cpp $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cc
$(OBJDIR)/%.o : %.cc $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cxx
$(OBJDIR)/%.o : %.cxx $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.i : %.c
	$(CC) -E $(CPPFLAGS) $<

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

# Include dependency files
include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
