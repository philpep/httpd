all:
	@(cd httpd && $(MAKE))

.PHONY: clean

clean:
	@(cd httpd && $(MAKE) $@)
