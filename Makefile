CC       := gcc
PERL     := perl

# Flags
OPTFLAGS  := -O0
PROFFLAGS := #-pg
DEFS      += 
CFLAGS    := -Wall -g $(PROFFLAGS) $(OPTFLAGS) $(DEFS)
CPPFLAGS  :=
LDFLAGS   := 

# Libraries
LDLIBS    :=
LOADLIBES := 

# Target names
TARGET    := 
LIBTARGET := libTbitMap.a
REV_MAJOR  := 0
REV_MINOR  := 1

# Directories to build
OBJDIR := obj/
DEPDIR := dep/


# Source files
LIBSRCS   := tbitmap.c mtrie3l.c date.c
SRCS      :=

# Object files
LIBOBJS   := $(addprefix $(OBJDIR),$(LIBSRCS:.c=.o))
OBJS      :=


$(TARGET): $(OBJS) $(LIBTARGET)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) $(PROF) -o $@

$(LIBTARGET): $(LIBOBJS)
	$(AR) $(ARFLAGS) $@ $^
	ranlib $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(UTESTTARGET) $(PTESTTARGET) \
	$(TARGET).exe $(UTESTTARGET).exe $(PTESTTARGET).exe \
	$(LIBTARGET) $(OBJS) $(LIBOBJS) $(DEPDIR)*.d *.bak *.exe.* *~


# Include dependency files
include $(addprefix $(DEPDIR),$(SRCS:.cpp=.d))

date.c: tbitmap-revision.c $(filter-out date.c, $(SRCS)) $(wildcard *.h)
	sed -e "s/DATE-STRING/`date`/" \
	    -e "s/MAJOR/$(REV_MAJOR)/" \
	    -e "s/MINOR/$(REV_MINOR)/" tbitmap-revision.c > date.c

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
