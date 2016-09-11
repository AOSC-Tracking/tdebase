SUBDIRS = shared kcontrol app data update $(KHOTKEYS_ARTS_SUBDIR)

messages: rc.cpp
	$(EXTRACTRC) kcontrol/ui/*.ui data/*.khotkeys >> rc.cpp
	$(XGETTEXT) rc.cpp app/*.cpp shared/*.cpp shared/*.h kcontrol/*.cpp -o $(podir)/khotkeys.pot
