/** Initializes localization and delegates process lifetime to GApplication. */
int main (string[] args) {
    Intl.setlocale (LocaleCategory.ALL, "");
    Intl.bindtextdomain (Characters.GETTEXT_PACKAGE, Characters.LOCALEDIR);
    Intl.bind_textdomain_codeset (Characters.GETTEXT_PACKAGE, "UTF-8");
    Intl.textdomain (Characters.GETTEXT_PACKAGE);

    return new Characters.Application ().run (args);
}
