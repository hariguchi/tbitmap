CC       := gcc
PERL     := perl

# Flags
OPTFLAGS  := -O0
PROFFLAGS := #-pg
DEFS      += 
CFLAGS    := -Wall -g -I.. $(PROFFLAGS) $(OPTFLAGS) $(DEFS)
CPPFLAGS  := -I..
LDFLAGS   := 

# Libraries
LDLIBS    := ../libTbitMap.a
LOADLIBES := 


# Target names
TARGET    := tbitmap-test
LIBTARGET := 

# Directories to build
OBJDIR := obj/
DEPDIR := dep/


# Source files
LIBSRCS   := 
SRCS      := tbitmap-test.c

# Object files
OBJS      := $(addprefix $(OBJDIR),$(SRCS:.c=.o))


$(TARGET): $(OBJS) $(LIBTARGET)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) $(PROF) -o $@

$(LIBTARGET): $(LIBOBJS)
	$(AR) $(ARFLAGS) $@ $^
	ranlib $@

.PHONY: perf
perf:
	$(MAKE) perfTest OPTFLAGS=-O3

.PHONY: clean
clean:
	rm -f $(TARGET) $(UTESTTARGET) $(PTESTTARGET) \
	$(TARGET).exe $(UTESTTARGET).exe $(PTESTTARGET).exe \
	$(LIBTARGET) $(OBJS) $(LIBOBJS) $(DEPDIR)*.d *.bak *.exe.* *~


# Include dependency files
include $(addprefix $(DEPDIR),$(SRCS:.c=.d))

$(DEPDIR)%.d : %.c
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< | sed "s@$*.o@$(OBJDIR)& $@@g " > $@'

$(DEPDIR)%.d : %.cpp
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< | sed "s@$*.o@$(OBJDIR)& $@@g " > $@'

$(OBJDIR)%.o: %.c $(DEPDIR)%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(OBJDIR)%.o: %.cc $(DEPDIR)%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(OBJDIR)%.o: %.cpp $(DEPDIR)%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

%.i : %.c
	$(CC) -E $(CPPFLAGS) $<
