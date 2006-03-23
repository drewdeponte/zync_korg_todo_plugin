SUBDIRS = src

all clean:
	for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir $@ ; done

install:
	for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir $@ ; done

	mkdir -p /usr/local/share/zync/korg_todo_plugin
	cp COPYING /usr/local/share/zync/korg_todo_plugin/
	cp AUTHORS /usr/local/share/zync/korg_todo_plugin/
	cp README /usr/local/share/zync/korg_todo_plugin/
	cp INSTALL /usr/local/share/zync/korg_todo_plugin/
	cp example.KOrgTodoPlugin.conf /usr/local/share/zync/korg_todo_plugin/