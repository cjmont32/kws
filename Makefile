INSTALL_PATH=srv/http/kws
INSTALL_ROOT=$(shell echo -n $(INSTALL_PATH) | sed -E 's/([^\/]+)\/.*/\1/')

JXUTIL_PATH=src/jxutil

DB_PATH=bin/db
OBJ_PATH=bin/objs
CGI_PATH=bin/cgi
PKG_PATH=bin/pkgs
PKG_NAME=kws_app

HDR_LIST=src/app/cgi.h src/app/html.h src/app/util.h src/app/db.h

OBJ_LIST_0=$(JXUTIL_PATH)/rel/jxutil.a
OBJ_LIST_1=$(OBJ_PATH)/util.o $(OBJ_PATH)/html.o $(OBJ_PATH)/cgi.o
OBJ_LIST_2=$(OBJ_PATH)/db.o $(OBJ_PATH)/main.o
OBJ_LIST_3=$(OBJ_LIST_0) $(OBJ_LIST_1) $(OBJ_LIST_2)
OBJ_LIST_4=$(OBJ_LIST_3) $(OBJ_PATH)/common.o

CGI_LIST=$(CGI_PATH)/index.cgi $(CGI_PATH)/search.cgi

STATIC_FILES=static/css/*.css static/js/*.js

ifneq ($(KWS_MAKE_RELEASE_BUILD),1)
CC_FLAGS=-I$(JXUTIL_PATH)/src -g -Wall
else
CC_FLAGS=-I$(JXUTIL_PATH)/src -O3 -Wall -Werror
endif

LD_FLAGS=-lsqlite3

$(OBJ_PATH)/util.o: src/app/util.c src/app/util.h src/app/html.h
	cc -c -o $(OBJ_PATH)/util.o src/app/util.c $(CC_FLAGS)

$(OBJ_PATH)/html.o: src/app/html.c src/app/html.h
	cc -c -o $(OBJ_PATH)/html.o src/app/html.c $(CC_FLAGS)

$(OBJ_PATH)/cgi.o: src/app/cgi.c $(HDR_LIST)
	cc -c -o $(OBJ_PATH)/cgi.o src/app/cgi.c $(CC_FLAGS)

$(OBJ_PATH)/db.o: src/app/db.c src/app/db.h
	cc -c -o $(OBJ_PATH)/db.o src/app/db.c $(CC_FLAGS)

$(OBJ_PATH)/main.o: src/app/main.c $(HDR_LIST)
	cc -c -o $(OBJ_PATH)/main.o src/app/main.c $(CC_FLAGS)

$(OBJ_PATH)/common.o: src/app/common.c $(HDR_LIST)
	cc -c -o $(OBJ_PATH)/common.o src/app/common.c $(CC_FLAGS)

$(CGI_PATH)/index.cgi: src/app/index.c $(OBJ_LIST_4)
	cc -o $(CGI_PATH)/index.cgi src/app/index.c $(OBJ_LIST_4) $(CC_FLAGS) $(LD_FLAGS)

$(CGI_PATH)/search.cgi: src/app/search.c $(OBJ_LIST_3)
	cc -o $(CGI_PATH)/search.cgi src/app/search.c $(OBJ_LIST_3) $(CC_FLAGS) $(LD_FLAGS)

$(JXUTIL_PATH)/rel/jxutil.a:
	make -C $(JXUTIL_PATH) librel

$(DB_PATH)/kws.db: setup
	cat sql/*.sql > $(DB_PATH)/all.sql
	sqlite3 -line -init $(DB_PATH)/all.sql $(DB_PATH)/kws.db ''

$(PKG_PATH)/$(PKG_NAME).tar.gz: $(CGI_LIST) $(STATIC_FILES)
	mkdir -p $(PKG_PATH)/$(INSTALL_PATH)
	cp -f $(CGI_LIST) $(PKG_PATH)/$(INSTALL_PATH)
	cp -rf static $(PKG_PATH)/$(INSTALL_PATH)
	rm -f $(PKG_PATH)/*.tar.gz
	fakeroot tar -C $(PKG_PATH) -cvf $(PKG_PATH)/$(PKG_NAME).tar $(INSTALL_ROOT)
	gzip $(PKG_PATH)/$(PKG_NAME).tar
	rm -rf $(PKG_PATH)/$(INSTALL_ROOT)

setup:
	@mkdir -p $(OBJ_PATH) $(CGI_PATH) $(PKG_PATH) $(DB_PATH)

all: setup $(PKG_PATH)/$(PKG_NAME).tar.gz

install: $(PKG_PATH)/$(PKG_NAME).tar.gz
	tar -C / --overwrite -xvf $(PKG_PATH)/$(PKG_NAME).tar.gz

db: $(DB_PATH)/kws.db

clean:
	make -C $(JXUTIL_PATH) clean
	rm -rf bin