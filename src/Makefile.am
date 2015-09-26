pluginlib_LTLIBRARIES = \
	libjabber-plugin.la

libjabber_plugin_ladir = $(pluginlibdir)
libjabber_plugin_la_SOURCES = \
	jabber-plugin.c

libjabber_plugin_la_CFLAGS = $(ACCOUNTS_CFLAGS) $(GLADE_CFLAGS) \
		-DPLUGIN_XML_DIR=\"$(pluginlibdir)/xml\"
libjabber_plugin_la_LIBADD = $(ACCOUNTS_LIBS) $(GLADE_LIBS)

AM_CFLAGS = \
		$(ACCOUNTS_CFLAGS)
		-Wall -Werror \
		-DG_LOG_DOMAIN=\"$(PACKAGE)\"

AM_LIBS = $(ACCOUNTS_LIBS)
