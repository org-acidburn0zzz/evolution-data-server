# We need to use our own CMake module until the one upstream is fixed
find_package(GtkDoc NO_DEFAULT_PATH PATHS "${CMAKE_SOURCE_DIR}/cmake/modules")

set(INSTALL_DOCDIR "${SHARE_INSTALL_PREFIX}/gtk-doc/html")

# ******************************
# Camel Documentation
# ******************************

set(CAMEL_SOURCEDIRS
	${CMAKE_SOURCE_DIR}/src/camel
	${CMAKE_SOURCE_DIR}/src/camel/providers/local
	${CMAKE_SOURCE_DIR}/src/camel/providers/imapx
	${CMAKE_SOURCE_DIR}/src/camel/providers/nntp
	${CMAKE_SOURCE_DIR}/src/camel/providers/pop3
	${CMAKE_SOURCE_DIR}/src/camel/providers/sendmail
	${CMAKE_SOURCE_DIR}/src/camel/providers/smtp
)

set(CAMEL_DEPENDENCIES
	camel
	camellocal
	camelimapx
	camelnntp
	camelpop3
	camelsendmail
	camelsmtp
)

set(CAMEL_IGNORE_HEADERS
	addresses.h
	address-data.h
	camel-charset-map-private.h
	camel-enumtypes.h
	camel-i18n.h
	camel-imapx-tokenise.h
	camel-imapx-utils.h
	camel-local-private.h
	camel-nntp-private.h
	camel-nntp-resp-codes.h
	camel-search-private.h
	camel-test.h
	camel-test-provider.h
	camel-win32.h
	folders.h
	messages.h
	session.h
)

set(CAMEL_XML ${CMAKE_CURRENT_BINARY_DIR}/camel-docs.sgml)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/camel-docs.sgml.in
	${CAMEL_XML}
	@ONLY
)

gtk_doc_add_module(
	camel
	XML ${CAMEL_XML}
	SOURCE ${CAMEL_SOURCEDIRS}
	LIBRARIES ${CAMEL_DEPENDENCIES}
	IGNOREHEADERS ${CAMEL_IGNORE_HEADERS}
	SCANOPTS "--deprecated-guards=CAMEL_DISABLE_DEPRECATED"
)

add_custom_target(camel-documentation ALL DEPENDS doc-camel)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/camel/html
		  DESTINATION ${INSTALL_DOCDIR}/camel)

# ******************************
# Evolution Data Server documentation
# ******************************

set(EDS_SOURCEDIRS
	${CMAKE_SOURCE_DIR}/src/libedataserver
	${CMAKE_BINARY_DIR}/src/libedataserver
	${CMAKE_SOURCE_DIR}/src/libebackend
	${CMAKE_SOURCE_DIR}/src/addressbook/libebook
	${CMAKE_SOURCE_DIR}/src/addressbook/libebook-contacts
	${CMAKE_SOURCE_DIR}/src/addressbook/libedata-book
	${CMAKE_SOURCE_DIR}/src/calendar/libecal
	${CMAKE_SOURCE_DIR}/src/calendar/libedata-cal
	${CMAKE_SOURCE_DIR}/tests/test-server-utils
)

set(EDS_DEPENDENCIES
	camel
	edataserver-private
	edbus-private
	egdbus-book
	egdbus-cal
	ebackend
	edataserver
	ebook
	ebook-contacts
	edata-book
	ecal
	edata-cal
	etestserverutils
)

set(EDS_IGNORE_HEADERS
	e-alphabet-index-private.h
	e-backend-enumtypes.h
	e-book-enumtypes.h
	e-book-contacts-enumtypes.h
	e-book-view-private.h
	e-cal-enumtypes.h
	e-cal-view-private.h
	e-client-private.h
	e-data-cal-types.h
	e-phone-number-private.h
	e-source-enumtypes.h
	e-transliterator-private.h
	libedataserver-private.h
	ximian-vcard.h
)

set(EDS_EXAMPLE ${CMAKE_CURRENT_BINARY_DIR}/xml/evolution-data-server-cursor-example.sgml)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/xml/evolution-data-server-cursor-example.sgml.in
	${EDS_EXAMPLE}
	@ONLY
)

set(EDS_XML ${CMAKE_CURRENT_BINARY_DIR}/evolution-data-server-docs.sgml)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/evolution-data-server-docs.sgml.in
	${EDS_XML}
	@ONLY
)

file(GLOB EDS_IMAGES "${CMAKE_CURRENT_SOURCE_DIR}/images/*.png")

gtk_doc_add_module(
	evolution-data-server
	XML ${EDS_XML}
	SOURCE ${EDS_SOURCEDIRS}
	LIBRARIES ${EDS_DEPENDENCIES}
	IGNOREHEADERS ${EDS_IGNORE_HEADERS}
	CONTENT_FILES ${EDS_EXAMPLE} ${EDS_IMAGES}
	SCANOPTS "--deprecated-guards=EDS_DISABLE_DEPRECATED"
)

add_custom_target(evolution-data-server-documentation ALL DEPENDS doc-evolution-data-server)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/evolution-data-server/html
		  DESTINATION ${INSTALL_DOCDIR}/evolution-data-server)

# ******************************
# Private documentation
# ******************************

if(WITH_PRIVATE_DOCS)
	set(EDBUS_PRIVATE_XML ${CMAKE_CURRENT_BINARY_DIR}/libedbus-private-docs.sgml)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/libedbus-private-docs.sgml.in
		${EDBUS_PRIVATE_XML}
		@ONLY
	)

	set(EDBUS_PRIVATE_SOURCEDIRS
		${CMAKE_BINARY_DIR}/src/private
	)

	set(EDBUS_PRIVATE_DEPENDENCIES
		edbus-private
	)

	set(EDBUS_PRIVATE_IGNORE_HEADERS
		e-dbus-localed.h
	)

	gtk_doc_add_module(
		libedbus-private
		XML ${EDBUS_PRIVATE_XML}
		SOURCE ${EDBUS_PRIVATE_SOURCEDIRS}
		LIBRARIES ${EDBUS_PRIVATE_DEPENDENCIES}
		IGNOREHEADERS ${EDBUS_PRIVATE_IGNORE_HEADERS}
		SCANOPTS "--deprecated-guards=EDS_DISABLE_DEPRECATED"
	)

	add_custom_target(private-documentation ALL DEPENDS doc-libedbus-private)

	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libedbus-private/html
			  DESTINATION ${INSTALL_DOCDIR}/libedbus-private)
endif(WITH_PRIVATE_DOCS)
