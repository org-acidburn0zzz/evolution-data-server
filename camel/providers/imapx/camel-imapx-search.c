/*
 * camel-imapx-search.c
 *
 * This library is free software you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "camel-imapx-search.h"

#include <camel/camel.h>
#include <camel/camel-search-private.h>

#include "camel-imapx-folder.h"

#define CAMEL_IMAPX_SEARCH_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), CAMEL_TYPE_IMAPX_SEARCH, CamelIMAPXSearchPrivate))

struct _CamelIMAPXSearchPrivate {
	GWeakRef server;
};

enum {
	PROP_0,
	PROP_SERVER
};

G_DEFINE_TYPE (
	CamelIMAPXSearch,
	camel_imapx_search,
	CAMEL_TYPE_FOLDER_SEARCH)

static void
imapx_search_set_property (GObject *object,
                           guint property_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVER:
			camel_imapx_search_set_server (
				CAMEL_IMAPX_SEARCH (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
imapx_search_get_property (GObject *object,
                           guint property_id,
                           GValue *value,
                           GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVER:
			g_value_take_object (
				value,
				camel_imapx_search_ref_server (
				CAMEL_IMAPX_SEARCH (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
imapx_search_dispose (GObject *object)
{
	CamelIMAPXSearchPrivate *priv;

	priv = CAMEL_IMAPX_SEARCH_GET_PRIVATE (object);

	g_weak_ref_set (&priv->server, NULL);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (camel_imapx_search_parent_class)->dispose (object);
}

static CamelSExpResult *
imapx_search_result_match_all (CamelSExp *sexp,
			       CamelFolderSearch *search)
{
	CamelSExpResult *result;

	g_return_val_if_fail (search != NULL, NULL);

	if (search->current != NULL) {
		result = camel_sexp_result_new (sexp, CAMEL_SEXP_RES_BOOL);
		result->value.boolean = TRUE;
	} else {
		gint ii;

		result = camel_sexp_result_new (sexp, CAMEL_SEXP_RES_ARRAY_PTR);
		result->value.ptrarray = g_ptr_array_new ();

		for (ii = 0; ii < search->summary->len; ii++)
			g_ptr_array_add (
				result->value.ptrarray,
				(gpointer) search->summary->pdata[ii]);
	}

	return result;
}

static CamelSExpResult *
imapx_search_result_match_none (CamelSExp *sexp,
				CamelFolderSearch *search)
{
	CamelSExpResult *result;

	g_return_val_if_fail (search != NULL, NULL);

	if (search->current != NULL) {
		result = camel_sexp_result_new (sexp, CAMEL_SEXP_RES_BOOL);
		result->value.boolean = FALSE;
	} else {
		result = camel_sexp_result_new (sexp, CAMEL_SEXP_RES_ARRAY_PTR);
		result->value.ptrarray = g_ptr_array_new ();
	}

	return result;
}

static CamelSExpResult *
imapx_search_process_criteria (CamelSExp *sexp,
			       CamelFolderSearch *search,
			       CamelIMAPXServer *server,
			       const GString *criteria,
			       const gchar *from_function)
{
	CamelSExpResult *result;
	CamelIMAPXMailbox *mailbox;
	GPtrArray *uids = NULL;
	GError *error = NULL;

	mailbox = camel_imapx_folder_list_mailbox (
		CAMEL_IMAPX_FOLDER (search->folder), NULL, &error);

	/* Sanity check. */
	g_return_val_if_fail (
		((mailbox != NULL) && (error == NULL)) ||
		((mailbox == NULL) && (error != NULL)), NULL);

	if (mailbox != NULL) {
		uids = camel_imapx_server_uid_search (
			server, mailbox, criteria->str, NULL, &error);
		g_object_unref (mailbox);
	}

	/* Sanity check. */
	g_return_val_if_fail (
		((uids != NULL) && (error == NULL)) ||
		((uids == NULL) && (error != NULL)), NULL);

	/* XXX No allowance for errors in CamelSExp callbacks!
	 *     Dump the error to the console and make like we
	 *     got an empty result. */
	if (error != NULL) {
		g_warning (
			"%s: (UID SEARCH %s): %s",
			from_function, criteria->str, error->message);
		uids = g_ptr_array_new ();
		g_error_free (error);
	}

	if (search->current != NULL) {
		result = camel_sexp_result_new (sexp, CAMEL_SEXP_RES_BOOL);
		result->value.boolean = (uids && uids->len > 0);
	} else {
		result = camel_sexp_result_new (sexp, CAMEL_SEXP_RES_ARRAY_PTR);
		result->value.ptrarray = g_ptr_array_ref (uids);
	}

	g_ptr_array_unref (uids);

	return result;
}

static CamelSExpResult *
imapx_search_match_all (CamelSExp *sexp,
			gint argc,
			CamelSExpTerm **argv,
			CamelFolderSearch *search)
{
	CamelIMAPXServer *server;
	CamelSExpResult *result;

	if (argc != 1)
		return imapx_search_result_match_none (sexp, search);

	server = camel_imapx_search_ref_server (CAMEL_IMAPX_SEARCH (search));
	if (!server || search->current) {
		g_clear_object (&server);

		/* Chain up to parent's method. */
		return CAMEL_FOLDER_SEARCH_CLASS (camel_imapx_search_parent_class)->
			match_all (sexp, argc, argv, search);
	}

	/* let's change the requirements a bit, the parent class expects as a result boolean,
	   but here is expected GPtrArray of matched UIDs */
	result = camel_sexp_term_eval (sexp, argv[0]);

	g_object_unref (server);

	g_return_val_if_fail (result != NULL, result);
	g_return_val_if_fail (result->type == CAMEL_SEXP_RES_ARRAY_PTR, result);

	return result;
}

static CamelSExpResult *
imapx_search_body_contains (CamelSExp *sexp,
                            gint argc,
                            CamelSExpResult **argv,
                            CamelFolderSearch *search)
{
	CamelIMAPXServer *server;
	CamelSExpResult *result;
	GString *criteria;
	gint ii, jj;

	/* Match everything if argv = [""] */
	if (argc == 1 && argv[0]->value.string[0] == '\0')
		return imapx_search_result_match_all (sexp, search);

	/* Match nothing if empty argv or empty summary. */
	if (argc == 0 || search->summary->len == 0)
		return imapx_search_result_match_none (sexp, search);

	server = camel_imapx_search_ref_server (CAMEL_IMAPX_SEARCH (search));

	/* This will be NULL if we're offline.  Search from cache. */
	if (server == NULL) {
		/* Chain up to parent's method. */
		return CAMEL_FOLDER_SEARCH_CLASS (camel_imapx_search_parent_class)->
			body_contains (sexp, argc, argv, search);
	}

	/* Build the IMAP search criteria. */

	criteria = g_string_sized_new (128);

	if (search->current != NULL) {
		const gchar *uid;

		/* Limit the search to a single UID. */
		uid = camel_message_info_uid (search->current);
		g_string_append_printf (criteria, "UID %s", uid);
	}

	for (ii = 0; ii < argc; ii++) {
		struct _camel_search_words *words;
		const guchar *term;

		if (argv[ii]->type != CAMEL_SEXP_RES_STRING)
			continue;

		/* Handle multiple search words within a single term. */
		term = (const guchar *) argv[ii]->value.string;
		words = camel_search_words_split (term);

		for (jj = 0; jj < words->len; jj++) {
			gchar *cp;

			if (criteria->len > 0)
				g_string_append_c (criteria, ' ');

			g_string_append (criteria, "BODY \"");

			cp = words->words[jj]->word;
			for (; *cp != '\0'; cp++) {
				if (*cp == '\\' || *cp == '"')
					g_string_append_c (criteria, '\\');
				g_string_append_c (criteria, *cp);
			}

			g_string_append_c (criteria, '"');
		}
	}

	result = imapx_search_process_criteria (sexp, search, server, criteria, G_STRFUNC);

	g_string_free (criteria, TRUE);
	g_object_unref (server);

	return result;
}

static CamelSExpResult *
imapx_search_header_contains (CamelSExp *sexp,
			      gint argc,
			      CamelSExpResult **argv,
			      CamelFolderSearch *search)
{
	CamelIMAPXServer *server;
	CamelSExpResult *result;
	const gchar *headername, *command = NULL;
	GString *criteria;
	gint ii, jj;

	/* Match nothing if empty argv or empty summary. */
	if (argc <= 1 ||
	    argv[0]->type != CAMEL_SEXP_RES_STRING ||
	    search->summary->len == 0)
		return imapx_search_result_match_none (sexp, search);

	server = camel_imapx_search_ref_server (CAMEL_IMAPX_SEARCH (search));

	/* This will be NULL if we're offline.  Search from cache. */
	if (server == NULL) {
		/* Chain up to parent's method. */
		return CAMEL_FOLDER_SEARCH_CLASS (camel_imapx_search_parent_class)->
			header_contains (sexp, argc, argv, search);
	}

	/* Build the IMAP search criteria. */

	criteria = g_string_sized_new (128);

	if (search->current != NULL) {
		const gchar *uid;

		/* Limit the search to a single UID. */
		uid = camel_message_info_uid (search->current);
		g_string_append_printf (criteria, "UID %s", uid);
	}

	headername = argv[0]->value.string;
	if (g_ascii_strcasecmp (headername, "From") == 0)
		command = "FROM";
	else if (g_ascii_strcasecmp (headername, "To") == 0)
		command = "TO";
	else if (g_ascii_strcasecmp (headername, "CC") == 0)
		command = "CC";
	else if (g_ascii_strcasecmp (headername, "Bcc") == 0)
		command = "BCC";
	else if (g_ascii_strcasecmp (headername, "Subject") == 0)
		command = "SUBJECT";

	for (ii = 1; ii < argc; ii++) {
		struct _camel_search_words *words;
		const guchar *term;

		if (argv[ii]->type != CAMEL_SEXP_RES_STRING)
			continue;

		/* Handle multiple search words within a single term. */
		term = (const guchar *) argv[ii]->value.string;
		words = camel_search_words_split (term);

		for (jj = 0; jj < words->len; jj++) {
			gchar *cp;

			if (criteria->len > 0)
				g_string_append_c (criteria, ' ');

			if (command)
				g_string_append (criteria, command);
			else
				g_string_append_printf (criteria, "HEADER \"%s\"", headername);

			g_string_append (criteria, " \"");

			cp = words->words[jj]->word;
			for (; *cp != '\0'; cp++) {
				if (*cp == '\\' || *cp == '"')
					g_string_append_c (criteria, '\\');
				g_string_append_c (criteria, *cp);
			}

			g_string_append_c (criteria, '"');
		}
	}

	result = imapx_search_process_criteria (sexp, search, server, criteria, G_STRFUNC);

	g_string_free (criteria, TRUE);
	g_object_unref (server);

	return result;
}

static CamelSExpResult *
imapx_search_header_exists (CamelSExp *sexp,
			    gint argc,
			    CamelSExpResult **argv,
			    CamelFolderSearch *search)
{
	CamelIMAPXServer *server;
	CamelSExpResult *result;
	GString *criteria;
	gint ii;

	/* Match nothing if empty argv or empty summary. */
	if (argc == 0 || search->summary->len == 0)
		return imapx_search_result_match_none (sexp, search);

	server = camel_imapx_search_ref_server (CAMEL_IMAPX_SEARCH (search));

	/* This will be NULL if we're offline.  Search from cache. */
	if (server == NULL) {
		/* Chain up to parent's method. */
		return CAMEL_FOLDER_SEARCH_CLASS (camel_imapx_search_parent_class)->
			header_exists (sexp, argc, argv, search);
	}

	/* Build the IMAP search criteria. */

	criteria = g_string_sized_new (128);

	if (search->current != NULL) {
		const gchar *uid;

		/* Limit the search to a single UID. */
		uid = camel_message_info_uid (search->current);
		g_string_append_printf (criteria, "UID %s", uid);
	}

	for (ii = 0; ii < argc; ii++) {
		const gchar *headername;

		if (argv[ii]->type != CAMEL_SEXP_RES_STRING)
			continue;

		headername = argv[ii]->value.string;

		if (criteria->len > 0)
			g_string_append_c (criteria, ' ');

		g_string_append_printf (criteria, "HEADER \"%s\" \"\"", headername);
	}

	result = imapx_search_process_criteria (sexp, search, server, criteria, G_STRFUNC);

	g_string_free (criteria, TRUE);
	g_object_unref (server);

	return result;
}

static void
camel_imapx_search_class_init (CamelIMAPXSearchClass *class)
{
	GObjectClass *object_class;
	CamelFolderSearchClass *search_class;

	g_type_class_add_private (class, sizeof (CamelIMAPXSearchPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = imapx_search_set_property;
	object_class->get_property = imapx_search_get_property;
	object_class->dispose = imapx_search_dispose;

	search_class = CAMEL_FOLDER_SEARCH_CLASS (class);
	search_class->match_all = imapx_search_match_all;
	search_class->body_contains = imapx_search_body_contains;
	search_class->header_contains = imapx_search_header_contains;
	search_class->header_exists = imapx_search_header_exists;

	g_object_class_install_property (
		object_class,
		PROP_SERVER,
		g_param_spec_object (
			"server",
			"Server",
			"Server proxy for server-side searches",
			CAMEL_TYPE_IMAPX_SERVER,
			G_PARAM_READWRITE |
			G_PARAM_STATIC_STRINGS));
}

static void
camel_imapx_search_init (CamelIMAPXSearch *search)
{
	search->priv = CAMEL_IMAPX_SEARCH_GET_PRIVATE (search);
}

/**
 * camel_imapx_search_new:
 *
 * Returns a new #CamelIMAPXSearch instance.
 *
 * The #CamelIMAPXSearch must be given a #CamelIMAPXSearch:server before
 * it can issue server-side search requests.  Otherwise it will fallback
 * to the default #CamelFolderSearch behavior.
 *
 * Returns: a new #CamelIMAPXSearch
 *
 * Since: 3.8
 **/
CamelFolderSearch *
camel_imapx_search_new (void)
{
	return g_object_new (CAMEL_TYPE_IMAPX_SEARCH, NULL);
}

/**
 * camel_imapx_search_ref_server:
 * @search: a #CamelIMAPXSearch
 *
 * Returns a #CamelIMAPXServer to use for server-side searches,
 * or %NULL when the corresponding #CamelIMAPXStore is offline.
 *
 * The returned #CamelIMAPXSearch is referenced for thread-safety and
 * must be unreferenced with g_object_unref() when finished with it.
 *
 * Returns: a #CamelIMAPXServer, or %NULL
 *
 * Since: 3.8
 **/
CamelIMAPXServer *
camel_imapx_search_ref_server (CamelIMAPXSearch *search)
{
	g_return_val_if_fail (CAMEL_IS_IMAPX_SEARCH (search), NULL);

	return g_weak_ref_get (&search->priv->server);
}

/**
 * camel_imapx_search_set_server:
 * @search: a #CamelIMAPXSearch
 * @server: a #CamelIMAPXServer, or %NULL
 *
 * Sets a #CamelIMAPXServer to use for server-side searches.  Generally
 * this is set for the duration of a single search when online, and then
 * reset to %NULL.
 *
 * Since: 3.8
 **/
void
camel_imapx_search_set_server (CamelIMAPXSearch *search,
                               CamelIMAPXServer *server)
{
	g_return_if_fail (CAMEL_IS_IMAPX_SEARCH (search));

	if (server != NULL)
		g_return_if_fail (CAMEL_IS_IMAPX_SERVER (server));

	g_weak_ref_set (&search->priv->server, server);

	g_object_notify (G_OBJECT (search), "server");
}
