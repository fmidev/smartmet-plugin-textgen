all:

clean:
	rm -f PluginTest *~
	rm -f cnf/geonames.conf
	rm -fr tmp-geonames-db
	rm -f tmp-geonames-db.log

TEST_PREPARE_TARGETS := cnf/geonames.conf
TEST_FINISH_TARGETS := .dummy
ifdef CI
GEONAMES_HOST_EDIT := sed -e 's/"smartmet-test"/"localhost"/g'
TEST_PREPARE_TARGETS += geonames-database
TEST_FINISH_TARGETS += stop-test-db
else
GEONAMES_HOST_EDIT := cat
endif

TEST_DB_DIR := tmp-geonames-db
TESTER_PARAM := --handler=/textgen --reactor-config=cnf/reactor.conf

test:	$(TEST_PREPARE_TARGETS)
	@rm -f failures/*
	@echo Running tests:
	ok=true; smartmet-plugin-test $(TESTER_PARAM) || ok=false; $(MAKE) $(TEST_FINISH_TARGETS); $$ok

cnf/geonames.conf: cnf/geonames.conf.in .dummy
	$(GEONAMES_HOST_EDIT) $< >$@

geonames-database:
	rm -rf tmp-geonames-db
	if ! /usr/share/smartmet/test/db/create-local-db.sh $(TEST_DB_DIR) >tmp-geonames-db.log 2>&1 ; then \
	    cat tmp-geonames-db.log; \
	    false; \
	fi
	netstat -ln | grep ^tcp
	sleep 5
	/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) start -w

stop-test-db:
	/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) stop

.dummy:
	true
