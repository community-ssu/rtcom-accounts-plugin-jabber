#include <stdlib.h>
#include <gtk/gtk.h>
#include <hildon/hildon.h>
#include <glade/glade.h>
#include <libaccounts/account-plugin.h>
#include <librtcom-accounts-widgets/rtcom-account-plugin.h>
#include <librtcom-accounts-widgets/rtcom-dialog-context.h>
#include <librtcom-accounts-widgets/rtcom-login.h>
#include <librtcom-accounts-widgets/rtcom-edit.h>
#include <librtcom-accounts-widgets/rtcom-param-int.h>
#include <hildon-uri.h>
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
  int unk;
};
typedef struct _JabberPluginPrivate JabberPluginPrivate;

RtcomAccountPluginClass *parent_class;

ACCOUNT_DEFINE_PLUGIN(JabberPlugin, jabber_plugin, RTCOM_TYPE_ACCOUNT_PLUGIN);

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

static GtkWidget *create_advanced_settings_page(RtcomDialogContext *context)
{
  GtkWidget *dialog= g_object_get_data(G_OBJECT(context), "page_advanced");

  if (!dialog) {
    AccountItem *account;
    GtkWidget *button;
    GHashTable *settings;
    char title[200];

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
                           dgettext("hildon-libs", "wdgt_bd_done"),
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
                           RTCOM_ACCOUNT_ITEM (account));

    g_snprintf(title, sizeof(title),
               g_dgettext("osso-applet-accounts",
                          "accountwizard_ti_advanced_settings"),
               account_service_get_display_name(
                 account_item_get_service(account)));

    gtk_window_set_title(GTK_WINDOW(dialog), title);
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
  }
  else
  {
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

  g_free((gpointer)username_label);
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
