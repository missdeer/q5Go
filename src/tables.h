/*
 *   tables.h
 */

#ifndef TABLES_H
#define TABLES_H

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <functional>
#include <unordered_map>

#include "gs_globals.h"
#include "parser.h"
#include "ui_talk_gui.h"

template<class T> class table_model : public QAbstractItemModel
{
    std::vector<T>                 m_entries;
    int                            m_column = 0;
    Qt::SortOrder                  m_order  = Qt::AscendingOrder;
    std::function<bool(const T &)> m_filter;
    int                            sort_compare(const T &a, const T &b);

public:
    void reset();
    void set_filter(std::function<bool(const T &)> filter)
    {
        m_filter = filter;
    }
    const T *             find(const QModelIndex &) const;
    const std::vector<T> &entries() const
    {
        return m_entries;
    }
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex      index(int row, int col, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex      parent(const QModelIndex &index) const override;
    int              rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int              columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool             removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QVariant         headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual void     sort(int, Qt::SortOrder) override;

    void update_from_map(const std::unordered_map<QString, T> &);
};

//-----------

class Channel
{
private:
    int     nr;
    QString title;
    QString users;
    int     count;

public:
    Channel(int nr_, const QString &title_ = QString(), const QString &users_ = QString(), int count_ = 0)
    {
        nr    = nr_;
        title = title_;
        users = users_;
        count = count_;
    }
    ~Channel() {};
    int get_nr() const
    {
        return nr;
    }
    QString get_title() const
    {
        return title;
    }
    QString get_users() const
    {
        return users;
    }
    int get_count() const
    {
        return count;
    }
    void set_channel(int nr_, const QString &title_, const QString &users_ = QString(), int count_ = 0)
    {
        if (this->nr == nr_)
        {
            if (!title_.isNull())
                this->title = title_;
            if (!users_.isNull())
                this->users = users_;
            if (count_)
                this->count = count_;
        }
    }

    // operators <, ==
    int operator==(Channel h)
    {
        return (this->get_nr() == h.get_nr());
    };
    bool operator<(Channel h)
    {
        return (this->get_nr() < h.get_nr());
    };
};

typedef QList<Channel *> ChannelList;

//-----------

class Talk
    : public QDialog
    , public Ui::TalkGui
{
    Q_OBJECT

    QString m_name;
    QColor  m_default_text_color;

public:
    Talk(const QString &, QWidget *, bool isplayer = true);

    QString get_name() const
    {
        return m_name;
    }
    void set_name(QString &n)
    {
        m_name = n;
    }
    void write(const QString &text, QColor col);

    bool lineedit_has_focus();

public slots:
    void slot_returnPressed();
    void slot_match();

signals:
    void signal_talkto(QString &, QString &);
    void signal_pbRelOneTab(QWidget *);
};

#endif
