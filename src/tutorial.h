#include <QMap>

#include "slideview.h"

QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)

class Tutorial_Slideshow : public BaseSlideView<Ui::SlideshowDialog, QWidget>
{
    void                         update_buttons();
    void                         load_tutorial(int);
    QMap<QTreeWidgetItem *, int> m_item_map;

public:
    Tutorial_Slideshow(QWidget *parent);
};

extern Tutorial_Slideshow *tutorial_dialog;
