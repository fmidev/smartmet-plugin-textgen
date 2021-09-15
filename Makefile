SUBNAME = textgen
SPEC = smartmet-plugin-$(SUBNAME)
INCDIR = smartmet/plugins/$(SUBNAME)

REQUIRES = gdal

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

# Compiler options

DEFINES = -DUNIX -D_REENTRANT

INCLUDES += -isystem $(includedir)/mysql
# Compile options in detault, debug and profile modes

LIBS += -L$(libdir) \
	-lsmartmet-macgyver \
	-lsmartmet-spine \
	-lsmartmet-calculator \
	-lsmartmet-textgen \
	-lsmartmet-newbase \
	-lsmartmet-locus \
	-lmysqlpp \
	-lboost_date_time \
	-lboost_timer \
	-lboost_chrono \
	-lboost_thread \
	-lboost_iostreams \
	-lbz2 -lz -lrt

# What to install

LIBFILE = $(SUBNAME).so

# Compilation directories

vpath %.cpp $(SUBNAME)
vpath %.h $(SUBNAME)

# The files to be compiled

SRCS = $(wildcard $(SUBNAME)/*.cpp)
HDRS = $(wildcard $(SUBNAME)/*.h)
OBJS = $(patsubst %.cpp, obj/%.o, $(notdir $(SRCS)))

INCLUDES := -I$(SUBNAME) $(INCLUDES)

.PHONY: test rpm

# The rules

all: objdir $(LIBFILE)
debug: all
release: all
profile: all

$(LIBFILE): $(OBJS)
	$(CC) $(LDFLAGS) -shared -rdynamic -o $(LIBFILE) $(OBJS) $(LIBS)
	@echo "Checking $(LIBFILE) for unresolved references (except SmartMet::Engine::*)"
	@if ldd -r $(LIBFILE) 2>&1 | c++filt | grep ^undefined\ symbol | grep -v SmartMet::Engine:: ; \
		then rm -v $(LIBFILE); \
		exit 1; \
	fi

clean:
	rm -f $(LIBFILE) *~ $(SUBNAME)/*~
	rm -rf obj

format:
	clang-format -i -style=file $(SUBNAME)/*.h $(SUBNAME)/*.cpp

install:
	@mkdir -p $(plugindir)
	$(INSTALL_PROG) $(LIBFILE) $(plugindir)/$(LIBFILE)

test:
	cd test && make test

objdir:
	@mkdir -p $(objdir)

rpm: clean $(SPEC).spec
	rm -f $(SPEC).tar.gz # Clean a possible leftover from previous attempt
	tar -czvf $(SPEC).tar.gz --exclude test --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -tb $(SPEC).tar.gz
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

-include $(wildcard obj/*.d)
