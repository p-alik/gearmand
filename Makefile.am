INCLUDES = 
SUBDIRS = libgearman tests server examples

test: all
	cd tests; make test; cd ..

test-extended: all
	cd tests; make test-extended; cd ..

valgrind:
	cd tests; make valgrind; cd ..

rpm: all dist
	cp libgearman-$(VERSION).tar.gz /home/brian/rpmbuild/SOURCES/
	rpmbuild -ba support/libgearman.spec
	cp /home/brian/rpmbuild/RPMS/x86_64/libgearman-$(VERSION)*.rpm .
	cp /home/brian/rpmbuild/SRPMS/libgearman-$(VERSION)*.rpm .
