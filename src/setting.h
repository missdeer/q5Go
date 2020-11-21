/*
 * setting.h
 */

#ifndef SETTING_H
#define SETTING_H

#include <QColor>
#include <QFont>
#include <QMap>
#include <QMutex>
#include <QString>
#include <QVariant>

// delimiters for variables in settings file
// however, this character must not be used in parameters, e.g. host
#define DELIMITER ':'
#define SETTING_VERSION 1

class QTextStream;

struct Engine
{
    QString title;
    QString path;
    QString args;
    QString komi;
    QString boardsize;
    bool    analysis;
    bool    restrictions;

    Engine(const QString &t, const QString &p, const QString &a, const QString &k, bool an, const QString &size, bool r)
        : title(t)
        , path(p)
        , args(a)
        , komi(k)
        , boardsize(size)
        , analysis(an)
        , restrictions(r)
    {
    }
    Engine(const Engine &) = default;
    Engine(Engine &&)      = default;
    Engine &operator=(Engine &&) = default;
    Engine &operator=(const Engine &) = default;

    static int n_columns()
    {
        return 1;
    }
    QString data(int) const
    {
        return title;
    }
    QVariant icon(int) const
    {
        return QVariant();
    }
};

struct Host
{
    QString      title;
    QString      host;
    QString      login_name;
    QString      password;
    QString      codec;
    unsigned int port = -1;
    /* -1 if unset.  */
    int quiet = -1;

    Host() = default;
    Host(const QString &t, const QString &h, const unsigned int p, const QString &l, const QString &pw, const QString &c, int q)
        : title(t)
        , host(h)
        , login_name(l)
        , password(pw)
        , codec(c)
        , port(p)
        , quiet(q)
    {
    }
    Host(const Host &) = default;
    Host(Host &&)      = default;
    Host &operator=(Host &&) = default;
    Host &operator=(const Host &) = default;

    static int n_columns()
    {
        return 1;
    }
    QString data(int) const
    {
        return title;
    }
    QVariant icon(int) const
    {
        return QVariant();
    }
};

/* A few settings which are used frequently enough that we don't want
   to spend a lookup each time.  The alternative is caching them locally
   in the classes that use them, or passing them around through arguments,
   neither of which option is any more elegant than this.
   These are updated in extract_frequent_settings.  */
struct setting_vals
{
    bool analysis_hideother;
    bool analysis_children;
    int  analysis_vartype;
    int  analysis_winrate;

    int gametree_diaghide;
    int gametree_size;

    int toroid_dups;

    bool clicko_delay;
    bool clicko_hitbox;

    bool   time_warn_board;
    QColor chat_color;
};

class Setting
{
    const QPixmap *m_wood_image {};
    const QPixmap *m_table_image {};
    QString        settingHomeDir;

    QMap<QString, QString> params;

    void extract_lists();
    void write_lists(QTextStream &);
    void lists_to_entries();

    void updateFont(QFont &, QString);

public:
    Setting();
    ~Setting();

    QFont        fontStandard, fontMarks, fontComments, fontLists, fontClocks, fontConsole;
    QString      language;
    setting_vals values;

    bool nmatch_settings_modified = false;
    bool dbpaths_changed          = false;
    bool engines_changed          = false;
    bool hosts_changed            = false;

    /* Protects access from the database thread.  m_dbpaths is only changed by
       the main thread, and only while the lock is held.  */
    QMutex              m_dbpath_lock;
    QStringList         m_dbpaths;
    std::vector<Engine> m_engines;
    std::vector<Host>   m_hosts;

    void clearEntry(const QString &s)
    {
        writeEntry(s, QString());
    }
    void writeEntry(const QString &, const QString &);
    void writeBoolEntry(const QString &s, bool b)
    {
        writeEntry(s, b ? "1" : "0");
    }
    void writeIntEntry(const QString &s, int i)
    {
        writeEntry(s, QString::number(i));
    }
    QString readEntry(const QString &);
    bool    readBoolEntry(const QString &s)
    {
        QString e = readEntry(s);
        return e == "1";
    }
    int readIntEntry(const QString &s)
    {
        bool    ok;
        QString e = readEntry(s);
        if (e.isNull())
            return 0;
        int i = e.toInt(&ok);
        if (ok)
            return i;
        return 0;
    }
    /* Tri-state bool, with -1 indicating unset.  */
    int readTriEntry(const QString &s)
    {
        QString e = readEntry(s);
        return e == "0" ? 0 : e == "1" ? 1 : -1;
    }
    void    loadSettings();
    void    saveSettings();
    QString fontToString(QFont);

    void              extract_frequent_settings();
    QString           getLanguage();
    const QStringList getAvailableLanguages();
    QString           convertNumberToLanguage(int n);
    int               convertLanguageCodeToNumber();
    QString           getTranslationsDirectory();
    bool              getNewVersionWarning();

    void           obtain_skin_images();
    const QPixmap *wood_image()
    {
        return m_wood_image;
    }
    const QPixmap *table_image()
    {
        return m_table_image;
    }
};

extern QString wood_image_resource(int idx, const QString &);

extern Setting *setting;

#endif
