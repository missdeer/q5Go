/*
 * gametree.cpp
 */

#include <QHelpEvent>

#include "gametree.h"

#include "board.h"
#include "config.h"
#include "goboard.h"
#include "gogame.h"
#include "gotools.h"
#include "mainwindow.h"
#include "setting.h"
#include "svgbuilder.h"

static QByteArray box_svg = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
                            "<svg width=\"160\" height=\"160\">"
                            "  <path"
                            "     style=\"fill:#a89e97;fill-rule:evenodd;stroke:none\""
                            "     d=\"M 80,50 160,25 80,0 0,25 Z\" />"
                            "  <path"
                            "     style=\"fill:#917c6f;fill-rule:evenodd;stroke:none\""
                            "     d=\"M 80,150 160,125 160,25 80,50 Z\" />"
                            "  <path"
                            "     style=\"fill:#483e37;fill-rule:evenodd;stroke:none\""
                            "     d=\"M 80,50 80,150 0,125 0,25 Z\" />"
                            "  <path"
                            "     style=\"fill:#FFFF00;fill-rule:evenodd;stroke:none\""
                            "     d=\"M 35.2,14 44.8,11 124.8,36 115.2,39 Z\" />"
                            "</svg>";

class GameTreePixmap : public QGraphicsItem
{
    GameTree *         m_view;
    game_state *       m_root;
    int                m_size, m_width;
    game_tree_pixmaps *m_pm;

public:
    GameTreePixmap(GameTree *view, QGraphicsScene *scene, game_state *item_root, int w, int size, game_tree_pixmaps *pms)
        : m_view(view)
        , m_root(item_root)
        , m_size(size)
        , m_width(w)
        , m_pm(pms)
    {
        setZValue(10);
        /* Supposedly faster, and makes it easier to click on edit nodes.  */
        // setShapeMode (QGraphicsPixmapItem::BoundingRectShape);
        scene->addItem(this);
        setAcceptHoverEvents(true);
    }

protected:
    virtual void   mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    virtual void   hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
    virtual void   hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;
    virtual void   contextMenuEvent(QGraphicsSceneContextMenuEvent *e) override;
    virtual void   paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    virtual QRectF boundingRect() const override
    {
        return QRectF(0, 0, m_size * m_width, m_size);
    }
};

void GameTreePixmap::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    game_state *st = m_root;
    int         x  = 0;

    painter->setPen(Qt::NoPen);
    QPen diag_pen(Qt::blue);
    diag_pen.setWidth(2);

    while (st != nullptr)
    {
        QPixmap *src = &m_pm->box;
        if (!st->vis_collapsed())
        {
            if (!st->was_move_p())
                src = &m_pm->e;
            else if (st->get_move_color() == white)
                src = st->has_figure() ? &m_pm->wfig : &m_pm->w;
            else
                src = st->has_figure() ? &m_pm->bfig : &m_pm->b;
        }
        painter->drawPixmap(x * m_size + 1, 1, *src);
        if (st->has_hidden_diagrams())
        {
            painter->setPen(diag_pen);
            painter->drawRect(x * m_size + m_size / 2 + 2, 2, m_size / 2 - 4, m_size / 2 - 4);
            painter->setPen(Qt::NoPen);
        }
        x++;
        if (st->vis_collapsed())
            break;
        st = st->next_primary_move();
    }
}

void GameTreePixmap::contextMenuEvent(QGraphicsSceneContextMenuEvent *e)
{
    QPointF     pos = e->pos();
    int         x   = pos.x() / m_size;
    game_state *st  = m_root;
    while (x > 0)
    {
        st = st->next_primary_move();
        x--;
    }
    m_view->show_menu(st, e->screenPos());
}

void GameTreePixmap::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    QPointF     pos = e->pos();
    int         x   = pos.x() / m_size;
    game_state *st  = m_root;
    while (x > 0)
    {
        st = st->next_primary_move();
        x--;
    }
    if (e->button() == Qt::LeftButton)
    {
        if (e->modifiers() == Qt::ShiftModifier)
            m_view->toggle_collapse(st, false);
        else if (e->modifiers() == Qt::ControlModifier)
            m_view->toggle_collapse(st, true);
        else
            m_view->item_clicked(st);
    }
    else if (e->button() == Qt::MiddleButton)
        m_view->toggle_collapse(st, false);
}

void GameTreePixmap::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    m_view->setDragMode(QGraphicsView::NoDrag);
}

void GameTreePixmap::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
}

GameTree::GameTree(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(0, 0, 30, 30, this))
    , m_header_scene(new QGraphicsScene(0, 0, 30, 30, this))
{
    setFocusPolicy(Qt::NoFocus);
    setScene(m_scene);
    setSceneRect(QRectF());
    setBackgroundBrush(QBrush(Qt::white));

    setAlignment(Qt::AlignTop | Qt::AlignLeft);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setToolTip(tr("The game tree.\nClick nodes to move to them, click empty areas to drag.\n"
                  "Shift-click or middle-click nodes to collapse or expand their sub-variations.\n"
                  "Control-click a collapsed node to expand one level of its children."));

    update_prefs();

    m_previewer = new FigureView(nullptr, true);
}

void GameTree::clear_scene()
{
    /* Removing the scene before clearing it is an attempt at a performance optimization.
       It's not entirely clear whether it really helps.  */
    QTransform t = transform();
    setScene(nullptr);
    m_scene->clear();
    setScene(m_scene);
    setTransform(t);
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void GameTree::resize_header()
{
    m_header_view->resize(viewport()->width(), m_header_view->height());
}

void GameTree::set_board_win(MainWindow *win, QGraphicsView *header)
{
    m_win         = win;
    m_header_view = header;

    QFontMetrics fm(m_header_font);

    m_header_scene->setSceneRect(0, 0, 30, fm.height());
    m_header_view->setScene(m_header_scene);
    m_header_view->resize(m_header_view->width(), fm.height());
    QScrollBar *hscr  = horizontalScrollBar();
    QScrollBar *hscr2 = m_header_view->horizontalScrollBar();
    connect(hscr, &QScrollBar::valueChanged, hscr2, &QAbstractSlider::setSliderPosition);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    connect(verticalScrollBar(), &QAbstractSlider::rangeChanged, [this](int, int) {
        QMetaObject::invokeMethod(this, &GameTree::resize_header, Qt::QueuedConnection);
    });
#else
    header->hide();
#endif
}

void GameTree::update_prefs()
{
    m_size = std::max(30, std::min(120, setting->values.gametree_size));

    clear_scene();

    QFont        f = setting->fontStandard;
    QFontMetrics fm(f);

    int width = 0;
    for (int i = 0; i < 9; i++)
    {
        char  c     = '0' + i;
        char  str[] = {c, c, c, '\0'};
        QRect br    = fm.boundingRect(str);
        width       = std::max(width, br.width());
    }
    if (width > m_size)
    {
        double ratio = m_size * 1.0 / width;
        /* Don't scale too heavily.  */
        ratio = std::max(0.5, ratio);
        f.setPointSize(f.pointSize() * ratio);
    }
    m_header_font = f;
    if (m_header_view != nullptr)
    {
        QFontMetrics fm2(f);
        m_header_scene->setSceneRect(0, 0, m_header_scene->width(), fm2.height());
        m_header_view->setMaximumHeight(fm2.height());
        m_header_view->resize(viewport()->width(), fm2.height());
    }
    int         ssize = m_size - 2;
    int         soff  = ssize / 2;
    svg_builder wstone(ssize, ssize);
    wstone.circle_at(soff, soff, soff * 0.9 - 1, "white", "black", "2");
    svg_builder bstone(ssize, ssize);
    bstone.circle_at(soff, soff, soff * 0.9, "black", "none");
    svg_builder wfig(ssize, ssize);
    wfig.square_at(soff, soff, ssize * 0.9 - 1, "white", "black");
    svg_builder bfig(ssize, ssize);
    bfig.square_at(soff, soff, ssize * 0.9, "black", "none");
    svg_builder edit(ssize, ssize);
    edit.circle_at(soff / 2, soff / 2, (ssize / 4) * 0.9, "black", "none");
    edit.circle_at(ssize - soff / 2, ssize - soff / 2, (ssize / 4) * 0.9, "black", "none");
    edit.circle_at(soff / 2, ssize - soff / 2, (ssize / 4) * 0.9 - 0.5, "white", "black", "1");
    edit.circle_at(ssize - soff / 2, soff / 2, (ssize / 4) * 0.9 - 0.5, "white", "black", "1");
    m_pm.w    = QPixmap(wstone.to_pixmap(ssize, ssize));
    m_pm.b    = QPixmap(bstone.to_pixmap(ssize, ssize));
    m_pm.wfig = QPixmap(wfig.to_pixmap(ssize, ssize));
    m_pm.bfig = QPixmap(bfig.to_pixmap(ssize, ssize));
    m_pm.e    = QPixmap(edit.to_pixmap(ssize, ssize));

    QSvgRenderer renderer(box_svg);
    m_pm.box = QPixmap(ssize, ssize);
    m_pm.box.fill(Qt::transparent);
    QPainter painter;
    painter.begin(&m_pm.box);
    renderer.render(&painter);
    painter.end();

    if (m_game != nullptr)
        update(m_game, m_active, true);
}

void GameTree::resizeEvent(QResizeEvent *e)
{
    QGraphicsView::resizeEvent(e);
    m_header_view->resize(viewport()->width(), m_header_view->height());
    QScrollBar *hscr  = horizontalScrollBar();
    QScrollBar *hscr2 = m_header_view->horizontalScrollBar();
    hscr2->setSliderPosition(hscr->sliderPosition());
}

QSize GameTree::sizeHint() const
{
    return QSize(100, 100);
}

void GameTree::toggle_collapse(game_state *st, bool one_level)
{
    if (one_level)
    {
        if (!st->vis_expand_one())
            return;
    }
    else
        st->toggle_vis_collapse();
    update(m_game, m_active);
}

void GameTree::toggle_figure(game_state *st)
{
    if (st->has_figure())
        st->clear_figure();
    else
        st->set_figure(256, "");
    update(m_game, m_active);
}

void GameTree::item_clicked(game_state *st)
{
    if (m_game == nullptr)
        return;

    if (st == m_active)
        return;
    m_win->set_game_position(st);
}

void GameTree::show_menu(game_state *st, const QPoint &pos)
{
    QMenu menu;
    if (st->vis_collapsed())
    {
        menu.addAction(QIcon(m_pm.box), QObject::tr("Expand subtree"), [=]() { toggle_collapse(st, false); });
        menu.addAction(QObject::tr("Expand one level of child nodes"), [=]() { toggle_collapse(st, true); });
    }
    else
        menu.addAction(QIcon(m_pm.box), QObject::tr("Collapse subtree"), [=]() { toggle_collapse(st, false); });
    if (st->has_figure())
        menu.addAction(QIcon(":/BoardWindow/images/boardwindow/figure.png"), QObject::tr("Clear diagram status for this node"), [=]() {
            toggle_figure(st);
            m_win->update_figures();
        });
    else
        menu.addAction(QIcon(":/BoardWindow/images/boardwindow/figure.png"), QObject::tr("Set this move to be the start of a diagram"), [=]() {
            toggle_figure(st);
            m_win->update_figures();
        });
    menu.addAction(QObject::tr("Navigate to this node"), [=]() { item_clicked(st); });
    menu.exec(pos);
}

void GameTree::update(go_game_ptr gr, game_state *active, bool force)
{
    game_state *r              = gr->get_root();
    bool        changed        = gr != m_game;
    bool        active_changed = m_active != active;
    m_active                   = active;
    if (changed)
    {
        m_previewer->reset_game(gr);
        m_previewer->resizeBoard(250, 250);
        m_previewer->set_show_coords(false);
    }
    if (active_changed)
        do_autocollapse();

    changed |= r->update_visualization(setting->values.gametree_diaghide) || force;
    if (!changed && !active_changed)
        return;

    m_game = gr;
    if (changed)
    {
        const visual_tree &vroot = r->visualization();
        int                w     = vroot.width();
        int                h     = vroot.height();

        m_scene->setSceneRect(0, 0, m_size * w, m_size * h);
        clear_scene();

        m_sel      = nullptr;
        m_path     = nullptr;
        m_path_end = nullptr;

        QPen diag_pen(Qt::blue);
        diag_pen.setWidth(2);

        auto start_run = [&](int x0, int y, int len, game_state *st0) -> bool {
            GameTreePixmap *pm = new GameTreePixmap(this, m_scene, st0, len, m_size, &m_pm);
            pm->setPos(x0 * m_size, y * m_size);
            return true;
        };
        r->render_visualization(0, 0, start_run);

        QPainterPath path;
        QPen         dot_pen;
        dot_pen.setWidth(2);
        dot_pen.setStyle(Qt::DotLine);

        auto draw_line = [&](int x0, int y0, int x1, int y1, bool dotted) -> void {
            if (dotted)
            {
                QLineF line(x0, y0, x1, y1);
                m_scene->addLine(line, dot_pen);
            }
            else
            {
                path.moveTo(x0, y0);
                path.lineTo(x1, y1);
            }
        };
        r->render_visualization(m_size / 2, m_size / 2, m_size, draw_line, true);
        QPen pen;
        pen.setWidth(2);
        m_scene->addPath(path, pen);

        m_header_scene->clear();
        m_header_scene->setSceneRect(0, 0, m_size * w, m_header_scene->height());
        m_header_view->setSceneRect(0, 0, m_size * w, m_header_scene->height());
        for (int i = 0; i < w; i++)
        {
            auto * item   = m_header_scene->addSimpleText(QString::number(i), m_header_font);
            QRectF bounds = item->boundingRect();
            bounds.moveCenter({(i + 0.5) * m_size, m_header_view->height() / 2.});
            item->setPos(bounds.x(), 0);
            if (i % 2)
            {
                auto *ritem = m_header_scene->addRect(i * m_size, 0, m_size, m_header_scene->height(), QPen(Qt::NoPen), QBrush(Qt::white));
                ritem->setZValue(-1);
            }
        }
        m_header_view->verticalScrollBar()->setSliderPosition(0);
    }

    QPen pen;
    pen.setColor(Qt::red);
    pen.setWidth(4);
    QPainterPath path;
    bool         first = false;
    auto         point = [&](int x, int y) -> void {
        if (first)
            path.moveTo(x, y);
        else
            path.lineTo(x, y);
    };
    auto dotted_line = [&](int x0, int y0, int x1, int y1, bool) -> void {
        QLineF line(x0, y0, x1, y1);
        QPen   new_pen = pen;
        new_pen.setStyle(Qt::DotLine);
        delete m_path_end;
        m_path_end = m_scene->addLine(line, new_pen);
    };

    delete m_path_end;
    m_path_end = nullptr;
    r->render_active_trace(m_size / 2, m_size / 2, m_size, point, dotted_line);
    delete m_path;
    m_path = m_scene->addPath(path, pen);
    m_path->setZValue(3);

    int  acx = 0, acy = 0;
    bool found = r->locate_visual(0, 0, active, acx, acy);

    /* Ideally we'd keep the m_sel object around and just move it.  The problem is that when scrolling,
       Qt leaves behind ghost copies at the old location as graphical garbage.  */
    delete m_sel;
    m_sel = m_scene->addRect(0, 0, m_size, m_size, Qt::NoPen, QBrush(Qt::red));
    m_sel->setZValue(-1);
    if (found)
    {
        m_sel->setPos(acx * m_size, acy * m_size);
        ensureVisible(m_sel, m_size / 2, m_size / 2);
    }
    else
        m_sel->hide();
    m_scene->update();
}

bool GameTree::event(QEvent *e)
{
    if (e->type() != QEvent::ToolTip)
        return QGraphicsView::event(e);

    QHelpEvent *helpEvent   = static_cast<QHelpEvent *>(e);
    QPoint      event_point = helpEvent->pos();
    QPointF     point       = mapToScene(event_point);
    int         x           = point.x() / m_size;
    int         y           = point.y() / m_size;
    game_state *root        = m_game->get_root();
    game_state *st          = root->locate_by_vis_coords(x, y, 0, 0);

    if (st != nullptr)
    {
        m_previewer->resizeBoard(250, 250);
        m_previewer->set_crop(find_crop(st));
        m_previewer->set_displayed(st);
        QPixmap pix = m_previewer->draw_position(0);
        /* This node may get deleted before the next tooltip is shown,
           so reset back to something we know is safe.  */
        m_previewer->set_displayed(root);
        QImage     img = pix.toImage();
        QByteArray bytes;
        QBuffer    buffer(&bytes);
        img.save(&buffer, "PNG", 100);
        QString tip = QString("<img src='data:image/png;base64, %0'>").arg(QString(bytes.toBase64()));
        QToolTip::showText(helpEvent->globalPos(), tip);
    }
    else
    {
        QToolTip::hideText();
        m_previewer->set_displayed(root);
        e->ignore();
    }

    return true;
}

void GameTree::do_autocollapse()
{
    if (m_autocollapse)
        m_game->get_root()->collapse_nonactive(m_active);
}

void GameTree::contextMenuEvent(QContextMenuEvent *e)
{
    /* The dragMode test is shorthand for "are we hovering over an item?"  */
    if (dragMode() != QGraphicsView::ScrollHandDrag)
    {
        QGraphicsView::contextMenuEvent(e);
        return;
    }
    QMenu   menu;
    QAction autoAction(tr("&Auto collapse on/off"));
    autoAction.setCheckable(true);
    autoAction.setChecked(m_autocollapse);
    connect(&autoAction, &QAction::triggered, [this](bool on) {
        m_autocollapse = on;
        do_autocollapse();
        update(m_game, m_active);
    });
    QAction expandAction(tr("Expand &all"));
    connect(&expandAction, &QAction::triggered, [this](bool) {
        m_autocollapse = false;
        m_game->get_root()->expand_all();
        update(m_game, m_active);
    });

    menu.addAction(&autoAction);
    menu.addAction(&expandAction);
    menu.exec(e->globalPos());
}

void GameTree::changeEvent(QEvent *e)
{
    QGraphicsView::changeEvent(e);
    if (e->type() != QEvent::EnabledChange)
        return;
    if (isEnabled())
        setForegroundBrush(QBrush(Qt::transparent));
    else
        setForegroundBrush(QBrush(Qt::lightGray, Qt::Dense6Pattern));
}
