
# Other Variable

# C++ compiler 
CXX     = g++
OBJDIRP = linux-obj
PROGRAM = HttpGrabberExample
LDFLAGS =
#LIBS = -lpthread
LIBS = -lpthread -lopencv_calib3d -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_stereo \
       -lopencv_plot
INCS = -I.

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   # 64 bit 
   CFLAGS=-m64
   LDFLAGS=-m64
else
   # 32 bit 
   CFLAGS=
   LDFLAGS=
endif

ifeq ($(BUILD_TYPE),)
  MY_CFLAGS = -fpic -Wall -O3 $(INCS) -DNDEBUG
  OBJDIR =$(OBJDIRP)_rel
endif

ifeq ($(BUILD_TYPE),debug)
  MY_CFLAGS = -fpic -g -Wall $(INCS) -D_DEBUG
  OBJDIR =$(OBJDIRP)_dbg
endif

# Command to make an object file:
#set TESTING flag to avoid use of jni
COMPILE = $(CXX) $(MY_CFLAGS) $(CFLAGS) -c

# Command used to link objects:
LD = $(CXX) -fPIC

# ------------------------------------------------------------------------
# Source file groups

SOURCES=compat.cpp HttpDefs.cpp HttpGrabber.cpp HttpMsg.cpp HttpGrabberExample.cpp log.cpp cencode.cpp

# ------------------------------------------------------------------------
# Object file groups

OBJS = $(foreach SRCFILE,$(SOURCES),$(OBJDIR)/$(SRCFILE:.cpp=.o))

####### Build rules

all: $(PROGRAM)

$(PROGRAM) : $(OBJS)
	echo $(LD) $(LDFLAGS) $^ $(LIBS) -o $(OBJDIR)/$(PROGRAM)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(OBJDIR)/$(PROGRAM)


clean:
	rm -f $(OBJDIR)/*.o 
	rm -f $(OBJDIR)/$(PROGRAM)


$(OBJDIR)/%.o: %.cpp
	mkdir -p $(OBJDIR)
	$(COMPILE) $< -o $@
