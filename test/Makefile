all:

clean:
	rm -f cnf/geonames.conf
	rm -fr tmp-geonames-db
	rm -f tmp-geonames-db.log
	rm -rf tmp

TEST_DB_DIR := $(shell pwd)/tmp-geonames-db

TEST_PREPARE_TARGETS := cnf/geonames.conf cnf/gis.conf wgs84mode
TEST_FINISH_TARGETS := .dummy
ifdef CI
GEONAMES_HOST_EDIT := sed -e 's|"smartmet-test"|"$(TEST_DB_DIR)"|g'
TEST_PREPARE_TARGETS += start-geonames-db
TEST_FINISH_TARGETS += stop-geonames-db
else
GEONAMES_HOST_EDIT := cat
endif

TESTER_PARAM := --handler=/textgen --reactor-config=cnf/reactor.conf -e tmp/

test:	$(TEST_PREPARE_TARGETS)
	@rm -f failures/*
	@echo Running tests:
	ok=true; $(TEST_RUNNER) smartmet-plugin-test $(TESTER_PARAM) || ok=false; $(MAKE) $(TEST_FINISH_TARGETS); $$ok

cnf/geonames.conf: cnf/geonames.conf.in .dummy
	$(GEONAMES_HOST_EDIT) $< >$@

cnf/gis.conf: cnf/gis.conf.in .dummy
	$(GEONAMES_HOST_EDIT) $< >$@

geonames-database:
	@-$(MAKE) stop-geonames-db
	rm -rf tmp-geonames-db
	if ! /usr/share/smartmet/test/db/create-local-db.sh $(TEST_DB_DIR) >tmp-geonames-db.log 2>&1 ; then \
	    cat tmp-geonames-db.log; \
	    false; \
	fi

start-geonames-db: geonames-database
	/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) start -w

stop-geonames-db:
	@if [ -d $(TEST_DB_DIR) ] ; then \
	    /usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) stop; \
	fi

wgs84mode:
	rm -rf tmp && mkdir tmp && cp output/* tmp/
	if grep --quiet "#define WGS84 1" /usr/include/smartmet/newbase/NFmiGlobals.h; then for f in tmp/*.wgs84; do mv $$f tmp/$$(basename $$f .wgs84); done; fi

.dummy:
	true

.PHONY: cnf/geonames.conf cnf/gis.conf
