#include <stdlib.h>
#include <gtk/gtk.h>
#include <hildon/hildon.h>
#include <glade/glade.h>
#include <libaccounts/account-plugin.h>
#include <librtcom-accounts-widgets/rtcom-account-plugin.h>
#include <librtcom-accounts-widgets/rtcom-dialog-context.h>
#include <librtcom-accounts-widgets/rtcom-login.h>
#include <librtcom-accounts-widgets/rtcom-edit.h>
#include <librtcom-accounts-widgets/rtcom-param-bool.h>
#include <librtcom-accounts-widgets/rtcom-param-int.h>
#include <librtcom-accounts-widgets/rtcom-param-string.h>
#include <hildon-uri.h>
#include <telepathy-glib/util.h>
#include <config.h>

struct _JabberPlugin {
  RtcomAccountPlugin parent_instance;
};
typedef struct _JabberPlugin JabberPlugin;

struct _JabberPluginClass {
  RtcomAccountPluginClass parent_class;
};
typedef struct _JabberPluginClass JabberPluginClass;

struct _JabberPluginPrivate {
  GtkWidget *dialog;
};
typedef struct _JabberPluginPrivate JabberPluginPrivate;

RtcomAccountPluginClass *parent_class;

ACCOUNT_DEFINE_PLUGIN(JabberPlugin, jabber_plugin, RTCOM_TYPE_ACCOUNT_PLUGIN);

#define JABBER_PLUGIN_TYPE jabber_plugin_get_type()
#define JABBER_PLUGIN_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), JABBER_PLUGIN_TYPE, JabberPluginPrivate))

static void jabber_plugin_init(JabberPlugin *self)
{
  GList *profiles;
  GList *l;

  RTCOM_ACCOUNT_PLUGIN(self)->name = "jabber";
  RTCOM_ACCOUNT_PLUGIN(self)->username_prefill = NULL;
  /*  RTCOM_PLUGIN_CAPABILITY_REGISTER |
      RTCOM_PLUGIN_CAPABILITY_ADVANCED |
      RTCOM_PLUGIN_CAPABILITY_ALLOW_MULTIPLE == 0x15, what is 0x35?
   */
  RTCOM_ACCOUNT_PLUGIN(self)->capabilities = 0x35; /* ??? */

  profiles = mc_profiles_list_by_protocol("jabber");

  for (l = profiles; l; l = l->next) {
    McProfile *profile = (McProfile *)l->data;
    const char *ui = mc_profile_get_configuration_ui(profile);

    if (ui && !strcmp(ui, "jabber-plugin")) {
      rtcom_account_plugin_add_service(RTCOM_ACCOUNT_PLUGIN(self),
                                       mc_profile_get_unique_name(profile));
    }
  }

  mc_profiles_free_list(profiles);
  glade_init();
}

static void jabber_plugin_class_dispose(GObject *object)
{
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void jabber_plugin_class_finalize(GObject *object)
{
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static gchar *_get_string_from_profile(AccountEditContext *ctx,
                                       const gchar *group, const gchar *key)
{
  gchar *rv = NULL;
  McProfile *profile;
  GKeyFile *keyfile;
  RtcomAccountService *service = NULL;

  g_return_val_if_fail(RTCOM_IS_DIALOG_CONTEXT (ctx), rv);
  g_return_val_if_fail(group != NULL, rv);
  g_return_val_if_fail(key != NULL, rv);

  g_object_get(account_edit_context_get_account(ctx),
               "service", &service,
               NULL);
  g_return_val_if_fail(service != NULL, rv);

  profile = mc_profile_lookup(service->parent_instance.name);

  if (!profile) {
    g_warning("%s: can't find profile for service %s", G_STRLOC,
              service->parent_instance.name);

    goto out;
  }

  keyfile = mc_profile_get_keyfile(profile);

  if (keyfile) {
    rv = g_key_file_get_string(keyfile, group, key, NULL);

    if (!rv)
      g_warning("VALUE NOT FOUND");
  }
  else
    g_warning("%s: keyfile is NULL.", G_STRLOC);

  g_object_unref(profile);

out:
  g_object_unref(service);

  return rv;
}

static void on_require_encryption_toggled(GtkWidget *widget)
{
  GladeXML *xml = glade_get_widget_tree(widget);
  GtkWidget *ignore_ssl_errors =
      glade_xml_get_widget(xml, "ignore-ssl-errors-Button-finger");
  GtkWidget *force_old_ssl =
      glade_xml_get_widget(xml, "force-old-ssl-Button-finger");

  if (hildon_check_button_get_active(HILDON_CHECK_BUTTON(widget))) {
    gtk_widget_show(ignore_ssl_errors);
    gtk_widget_show(force_old_ssl);
  } else {
    gtk_widget_hide(ignore_ssl_errors);
    gtk_widget_hide(force_old_ssl);
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(force_old_ssl), FALSE);
  }
}

static void on_force_old_ssl_toggled(GtkWidget *widget)
{
  GladeXML *xml = glade_get_widget_tree(widget);
  gboolean force_old_ssl =
      hildon_check_button_get_active(HILDON_CHECK_BUTTON(widget));
  GtkWidget *port = glade_xml_get_widget(xml, "port");;
  int value = rtcom_param_int_get_value(RTCOM_PARAM_INT(port));

  if (value == G_MININT32 || value == (force_old_ssl ? 5222 : 5223)) {
    rtcom_param_int_set_value(RTCOM_PARAM_INT(port),
                              force_old_ssl ? 5223 : 5222);
  }
}

static GtkWidget *_get_toplevel_widget(RtcomDialogContext *ctx)
{
  GtkWidget *widget = rtcom_dialog_context_get_start_page(ctx);

  if (widget)
    widget = gtk_widget_get_toplevel(widget);

  return widget;
}

static void get_advanced_settings (GtkWidget *widget,
                                   GHashTable *advanced_settings)
{
  const gchar *name;

  name = gtk_widget_get_name (widget);

  if (RTCOM_IS_PARAM_INT(widget))
    {
      gint value = rtcom_param_int_get_value (RTCOM_PARAM_INT(widget));
      g_hash_table_replace(advanced_settings, widget,
          GINT_TO_POINTER(value));
    }
  else if (GTK_IS_ENTRY (widget))
    {
      gchar *data = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));
      g_object_set_data_full(G_OBJECT (widget), "adv_data", data, g_free);
      g_hash_table_replace(advanced_settings, widget, data);
    }
  else if (HILDON_IS_CHECK_BUTTON(widget))
    {
      gboolean active =
        hildon_check_button_get_active(HILDON_CHECK_BUTTON (widget));
      g_hash_table_replace(advanced_settings, widget,
          GINT_TO_POINTER(active));
    }
  else if (GTK_IS_CONTAINER(widget))
    {
      gtk_container_foreach(GTK_CONTAINER (widget),
          (GtkCallback)get_advanced_settings,
          advanced_settings);
    }
  else if (!GTK_IS_LABEL(widget))
    {
      g_warning ("%s: unhandled widget type %s (%s)", G_STRLOC,
          g_type_name(G_TYPE_FROM_INSTANCE(widget)), name);
    }
}

static void set_widget_setting(gpointer key, gpointer value, gpointer userdata)
{
  GtkWidget *widget = key;
  (void)userdata;

  if (RTCOM_IS_PARAM_INT(widget)) {
      rtcom_param_int_set_value(RTCOM_PARAM_INT(widget),
                                GPOINTER_TO_INT(value));
  } else if (GTK_IS_ENTRY(widget)) {
      gtk_entry_set_text(GTK_ENTRY(widget), value);
  } else if (HILDON_IS_CHECK_BUTTON(widget)) {
      hildon_check_button_set_active(HILDON_CHECK_BUTTON(widget),
                                     GPOINTER_TO_INT(value));
  } else {
      g_warning ("%s: unhandled widget type %s (%s)",
                 G_STRLOC,
                 g_type_name (G_TYPE_FROM_INSTANCE(widget)),
                 gtk_widget_get_name(widget));
  }
}

static void on_advanced_settings_response(GtkWidget *dialog, gint response,
                                          RtcomDialogContext *context)
{
  GHashTable *advanced_settings =
      g_object_get_data(G_OBJECT(context), "settings");

  if (response == GTK_RESPONSE_OK) {
    GError *error = NULL;
    GladeXML *xml = glade_get_widget_tree(dialog);
    GtkWidget *page = glade_xml_get_widget(xml, "page");

    if (rtcom_page_validate(RTCOM_PAGE(page), &error)) {
      get_advanced_settings(dialog, advanced_settings);
      gtk_widget_hide(dialog);
    } else {
      g_warning("advanced page validation failed");
      if (error) {
        g_warning("%s: error \"%s\"", G_STRLOC, error->message);
        hildon_banner_show_information(dialog, NULL, error->message);
        g_error_free(error);
      }
    }
  } else {
    g_hash_table_foreach(advanced_settings, set_widget_setting, dialog);
    gtk_widget_hide(dialog);
  }
}

static GtkWidget *create_advanced_settings_page(RtcomDialogContext *context)
{
  GtkWidget *dialog= g_object_get_data(G_OBJECT(context), "page_advanced");

  if (!dialog) {
    AccountItem *account;
    GtkWidget *button;
    GHashTable *settings;
    gchar *title;
    const gchar *format;

    GladeXML *xml = glade_xml_new(PLUGIN_XML_DIR "/jabber-advanced.glade",
                                  NULL,
                                  GETTEXT_PACKAGE);
    rtcom_dialog_context_take_obj(context, G_OBJECT(xml));
    dialog = glade_xml_get_widget(xml, "advanced");

    if (!dialog) {
      g_warning("Unable to load Advanced settings dialog");
      return dialog;
    }

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           g_dgettext("hildon-libs", "wdgt_bd_done"),
                           GTK_RESPONSE_OK,
                           NULL);
    g_object_ref(dialog);
    g_object_set_data_full( G_OBJECT (context),
                            "page_advanced",
                            dialog,
                            g_object_unref);

    button = glade_xml_get_widget(xml, "require-encryption-Button-finger");
    glade_xml_signal_connect(xml,
                             "on_require_encryption_toggled",
                             G_CALLBACK(on_require_encryption_toggled));

    on_require_encryption_toggled(button);

    button = glade_xml_get_widget(xml, "force-old-ssl-Button-finger");
    glade_xml_signal_connect(xml,
                             "on_force_old_ssl_toggled",
                             G_CALLBACK(on_force_old_ssl_toggled));
    on_force_old_ssl_toggled(button);

    button = glade_xml_get_widget(xml, "ignore-ssl-errors-Button-finger");
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(button), TRUE);

    account = account_edit_context_get_account(ACCOUNT_EDIT_CONTEXT(context));
    rtcom_page_set_account(RTCOM_PAGE(glade_xml_get_widget(xml, "page")),
                           RTCOM_ACCOUNT_ITEM(account));
    format = g_dgettext("osso-applet-accounts",
                        "accountwizard_ti_advanced_settings");
    title = g_strdup_printf(format, account_service_get_display_name(
                              account_item_get_service(account)));
    gtk_window_set_title(GTK_WINDOW(dialog), title);

    g_free(title);

    gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                 GTK_WINDOW(_get_toplevel_widget(context)));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

    settings = g_hash_table_new((GHashFunc)g_direct_hash,
                                (GEqualFunc)g_direct_equal);
    get_advanced_settings(dialog, settings);

    g_object_set_data_full( G_OBJECT(context),
                            "settings",
                            settings,
                            (GDestroyNotify)g_hash_table_destroy);
  }

  g_signal_connect_data(dialog,
                        "response",
                        G_CALLBACK(on_advanced_settings_response),
                        context,
                        NULL,
                        0);
  g_signal_connect_data(dialog,
                        "delete-event",
                        G_CALLBACK(gtk_true),
                        NULL,
                        NULL,
                        0);

  return dialog;
}

static void jabber_plugin_on_advanced_cb(gpointer data)
{
  GtkWidget *dialog;
  RtcomDialogContext *context = RTCOM_DIALOG_CONTEXT(data);

  dialog = create_advanced_settings_page(context);
  gtk_widget_show(dialog);
}

static void settings_for_each(gpointer key, gpointer value, gpointer user_data)
{
  GHashTable *hash_table = (GHashTable *)user_data;

  if (RTCOM_IS_PARAM_STRING(key) && value && *((char *) value))
  {
    g_hash_table_insert(hash_table,
                        (gpointer)rtcom_param_string_get_field(key),
                        tp_g_value_slice_new_static_string(
                          (const gchar *)value));
  }
  else if (RTCOM_IS_PARAM_BOOL(key))
  {
    g_hash_table_insert(hash_table,
                        (gpointer)rtcom_param_bool_get_field(key),
                        tp_g_value_slice_new_boolean(GPOINTER_TO_INT(value)));
  }
  else if (RTCOM_IS_PARAM_INT(key))
  {
    g_hash_table_insert(hash_table,
                        (gpointer)rtcom_param_int_get_field(key),
                        tp_g_value_slice_new_uint(GPOINTER_TO_INT(value)));
  }
}

static void connect_cb(GObject *obj, TpConnection *conn, GError *error,
                       gpointer user_data)
{
  JabberPluginPrivate *priv =
      JABBER_PLUGIN_GET_PRIVATE(account_edit_context_get_plugin(
                                  ACCOUNT_EDIT_CONTEXT(obj)));
  (void)conn;

  if (priv->dialog) {
    gtk_widget_destroy(priv->dialog);
    priv->dialog = NULL;
  }

  if (error)
    hildon_banner_show_information(GTK_WIDGET(user_data), NULL, error->message);
  else {
    rtcom_dialog_context_set_start_page(RTCOM_DIALOG_CONTEXT(obj), NULL);
    gtk_dialog_response(
          GTK_DIALOG(_get_toplevel_widget(RTCOM_DIALOG_CONTEXT(obj))),
          1);
  }
}

static void jabber_plugin_new_account(GtkWidget *widget, gint response,
                                      AccountEditContext *ctx)
{
  GladeXML *tree;
  GtkWidget *page;
  GHashTable *settings;
  GtkWidget *username;
  GtkWidget *password;
  AccountItem *account;
  gchar *title;
  const gchar *format;
  GtkWidget *dialog;
  GHashTable *hash_table;
  GError *error = NULL;

  if (response != GTK_RESPONSE_OK) {
    gtk_widget_destroy(widget);

    return;
  }

  tree = glade_get_widget_tree(widget);
  page = glade_xml_get_widget(tree, "page");

  if (!rtcom_page_validate((RtcomPage *)page, &error))
    goto out;

  if (!g_strstr_len(
        gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(tree, "username"))),
        -1, "@")) {
    g_set_error(&error,
                g_quark_from_static_string("account-error-quark"),
                4,
                g_dgettext("osso-applet-accounts",
                           "accountwizard_ib_illegal_server_address"));
    goto out;
  }

  if (strcmp(gtk_entry_get_text(
               GTK_ENTRY(glade_xml_get_widget(tree, "password"))),
             gtk_entry_get_text(
               GTK_ENTRY(glade_xml_get_widget(tree, "password2"))))) {
    g_set_error(&error,
                g_quark_from_static_string("account-error-quark"),
                6, g_dgettext("osso-applet-accounts",
                              "accountwizard_ib_passwords_do_not_match"));
    goto out;
  }

  hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, 0,
                                     (GDestroyNotify )tp_g_value_slice_free);

  settings = (GHashTable *)g_object_get_data(G_OBJECT(ctx), "settings");

  if (settings)
    g_hash_table_foreach(settings, settings_for_each, hash_table);

  username = glade_xml_get_widget(tree, "username");
  password = glade_xml_get_widget(tree, "password");

  g_hash_table_insert(hash_table,
                      "register",
                      tp_g_value_slice_new_boolean(TRUE));

  g_hash_table_insert(hash_table,
                      "account",
                      tp_g_value_slice_new_static_string(
                        gtk_entry_get_text(GTK_ENTRY(username))));

  g_hash_table_insert(hash_table,
                      "password",
                      tp_g_value_slice_new_static_string(
                        gtk_entry_get_text(GTK_ENTRY(password))));


  account = account_edit_context_get_account(ctx);

  rtcom_account_service_connect(
        RTCOM_ACCOUNT_SERVICE(account_item_get_service(account)),
        hash_table,
        &ctx->parent_instance,
        TRUE,
        (RtcomAccountServiceConnectionCb)connect_cb,
        widget);

  g_hash_table_unref(hash_table);

  format = g_dgettext("osso-applet-accounts", "accounts_ti_registering");
  title = g_strdup_printf(format, gtk_entry_get_text(GTK_ENTRY(username)));
  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), title);

  g_free(title);

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
  hildon_gtk_window_set_progress_indicator(GTK_WINDOW(dialog), TRUE);

  if (account->service_icon)
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       gtk_image_new_from_pixbuf(account->service_icon),
                       0, 0, 16);
  if (widget) {
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW((widget)));
    gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);
  }

  JABBER_PLUGIN_GET_PRIVATE(
        account_edit_context_get_plugin(ctx))->dialog = dialog;

  gtk_widget_show_all(dialog);

out:
    if (error) {
      hildon_banner_show_information(widget, NULL, error->message);
      g_error_free(error);
    }
}

static void jabber_plugin_on_register_cb(gpointer userdata)
{
  AccountEditContext *ctx = (AccountEditContext *)userdata;
  gchar *registration_url;

#if 0
  /* No idea why it is needed, not to say that noone g_unref's it */
  GObject *plugin;
  g_object_get(userdata, "plugin", &plugin, 0);
#endif

  registration_url =
      _get_string_from_profile(ctx, "Profile", "RegistrationUrl");

  if (registration_url) {
    GError *error = 0;
    if (!hildon_uri_open(registration_url, NULL, &error)) {
      g_warning("Failed to open browser: %s", error->message);
      g_error_free(error);
    }

    g_free(registration_url);
  } else {
    GladeXML *xml = glade_xml_new(PLUGIN_XML_DIR "jabber-new-account.glade",
                                  NULL,
                                  GETTEXT_PACKAGE);
    GtkWidget *register_dialog = glade_xml_get_widget(xml, "register");
    GtkWidget *advanced_button;

    rtcom_dialog_context_take_obj(RTCOM_DIALOG_CONTEXT(ctx), &xml->parent);

    if (!register_dialog) {
      g_warning("Unable to load Register dialog");

      return;
    }

    advanced_button = glade_xml_get_widget(xml, "advanced-button");

    g_signal_connect_data(
          advanced_button, "clicked",
          G_CALLBACK(jabber_plugin_on_advanced_cb), ctx, NULL,
          G_CONNECT_SWAPPED);

    gtk_dialog_add_buttons(GTK_DIALOG(register_dialog),
                           g_dgettext("osso-applet-accounts",
                                      "accounts_bd_register"),
                           GTK_RESPONSE_OK,
                           NULL);

    rtcom_page_set_account(
          RTCOM_PAGE(glade_xml_get_widget(xml, "page")),
          RTCOM_ACCOUNT_ITEM(account_edit_context_get_account(ctx)));

    gtk_window_set_title(GTK_WINDOW(register_dialog),
                         g_dgettext("osso-applet-accounts",
                                    "accounts_ti_new_jabber_account"));

    gtk_window_set_transient_for(
          GTK_WINDOW(register_dialog),
          GTK_WINDOW(_get_toplevel_widget(RTCOM_DIALOG_CONTEXT(ctx))));

    g_signal_connect_data(
          register_dialog,
          "response",
          G_CALLBACK(jabber_plugin_new_account),
          ctx,
          NULL,
          0);

    gtk_widget_show_all(register_dialog);
  }
}

static void jabber_plugin_class_context_init(RtcomAccountPlugin *plugin,
                                             RtcomDialogContext *context)
{
  GtkWidget *page;
  gboolean editing;
  AccountItem *account;
  gchar *username_label;
  gchar *msg_empty;
  static const gchar *invalid_chars_re = "[:'\"<>&;#\\s]";

  editing = account_edit_context_get_editing(ACCOUNT_EDIT_CONTEXT(context));
  account = account_edit_context_get_account(ACCOUNT_EDIT_CONTEXT(context));
  create_advanced_settings_page(context);

  username_label = _get_string_from_profile(ACCOUNT_EDIT_CONTEXT(context),
                                            "Profile", "UsernameLabel");
  if (!username_label)
    username_label = g_strdup("accounts_fi_user_name_sip");

  msg_empty = _get_string_from_profile(ACCOUNT_EDIT_CONTEXT(context),
                                       "Profile", "MsgEmpty");
  if (!msg_empty)
    msg_empty = g_strdup("accounts_fi_enter_address_and_password_fields_first");

  if (editing) {
    page = g_object_new(
          RTCOM_TYPE_EDIT,
          "username-field", "account",
          "username-invalid-chars-re", invalid_chars_re,
          "username-label", g_dgettext("osso-applet-accounts", username_label),
          "msg-empty", g_dgettext("osso-applet-accounts", msg_empty),
          "items-mask", plugin->capabilities,
          "account", account,
          NULL);
    rtcom_edit_connect_on_advanced(RTCOM_EDIT(page),
                                   G_CALLBACK(jabber_plugin_on_advanced_cb),
                                   context);
  } else {
     gchar *username_prefill =
         _get_string_from_profile(ACCOUNT_EDIT_CONTEXT(context),
                                  "Profile", "Prefill");
    if (!username_prefill)
      username_prefill = g_strdup(plugin->username_prefill);

    page = g_object_new(
          RTCOM_TYPE_LOGIN,
          "username-field", "account",
          "username-invalid-chars-re", invalid_chars_re,
          "username-must-have-at-separator", TRUE,
          "username-prefill", username_prefill,
          "username-label", g_dgettext("osso-applet-accounts", username_label),
          "msg-empty", g_dgettext("osso-applet-accounts", msg_empty),
          "items-mask", plugin->capabilities,
          "account", account,
          NULL);
    g_free(username_prefill);
    rtcom_login_connect_on_register(RTCOM_LOGIN(page),
                                    G_CALLBACK(jabber_plugin_on_register_cb),
                                    context);
    rtcom_login_connect_on_advanced(RTCOM_LOGIN(page),
                                    G_CALLBACK(jabber_plugin_on_advanced_cb),
                                    context);
  }

  g_free(username_label);
  g_free(msg_empty);
  rtcom_dialog_context_set_start_page(context, page);
}

static void jabber_plugin_class_init(JabberPluginClass *klass)
{
  parent_class = g_type_class_peek_parent(klass);
  RTCOM_ACCOUNT_PLUGIN_CLASS(klass)->context_init = jabber_plugin_class_context_init;
  G_OBJECT_CLASS(klass)->dispose =jabber_plugin_class_dispose;
  G_OBJECT_CLASS(klass)->finalize = jabber_plugin_class_finalize;
  g_type_class_add_private(klass, sizeof(JabberPluginPrivate));
}
