BINDIR:=../bin
.PHONY: all clean install uninstall

clean:
	$(foreach OBJ,$(wildcard ${BINDIR}/*),$(shell del OBJ))
