PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

REQUIRES_GDAL = yes

include ../common.mk

CFLAGS = -DUNIX -O0 -g $(FLAGS)

INCLUDES += \
	-I ../textgen \
	-I /usr/include/smartmet \
	-isystem /usr/include/mysql \
	`pkg-config --cflags libconfig++`

LIBS += \
	-lsmartmet-macgyver \
	-lsmartmet-spine \
	-lsmartmet-newbase \
	`pkg-config --libs libconfig++` \
	-lboost_date_time \
	-lboost_thread \
	-lboost_filesystem \
	-lboost_iostreams \
	-lboost_system \
	-lbz2 -lz -lpthread -ldl

all: $(PROG)
clean:
	rm -f $(PROG) *~

test: $(PROG)
ifeq ("$(wildcard cnf/local.conf)","")
	@echo "cnf/local.conf file does not exist!";
	exit 1;
endif
	@mkdir -p failures
	@echo Running tests:
	@for prog in $(PROG); do \
	./$$prog; \
	done

$(PROG) : % : %.cpp Makefile
	$(CXX) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)
