include src_config.mk

MAKE_DIR = make -C $@

all: cc1 cpp2 ucc

ucc: util
	${MAKE_DIR}

cc1: util
	${MAKE_DIR}

cpp2: util
	${MAKE_DIR}

util:
	${MAKE_DIR}

clean:
	make -C cc1  clean
	make -C cpp2 clean
	make -C ucc  clean
	make -C util clean

.PHONY: cc1 cpp2 ucc util clean
