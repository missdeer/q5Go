#include <QPushButton>
#include <QFileDialog>

#include <fstream>

#include "gogame.h"
#include "sgfpreview.h"

SGFPreview::SGFPreview (QWidget *parent, const QString &dir)
	: QDialog (parent)
{
	setupUi (this);

	game_info info;
	info.name_w = tr ("White").toStdString ();
	info.name_b = tr ("Black").toStdString ();
	m_empty_game = std::make_shared<game_record> (go_board (19), black, info);
	m_game = m_empty_game;

	QVBoxLayout *l = new QVBoxLayout (dialogWidget);
	fileDialog = new QFileDialog (dialogWidget, Qt::Widget);
	fileDialog->setOption (QFileDialog::DontUseNativeDialog, true);
	fileDialog->setWindowFlags (Qt::Widget);
	fileDialog->setNameFilters ({ tr ("SGF files (*.sgf *.SGF)"), tr ("All files (*)") });
	fileDialog->setDirectory (dir);

	setWindowTitle ("Open SGF file");
	l->addWidget (fileDialog);
	l->setContentsMargins (0, 0, 0, 0);
	fileDialog->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
	fileDialog->show ();
	connect (encodingList, &QComboBox::currentTextChanged, this, &SGFPreview::reloadPreview);
	connect (overwriteSGFEncoding, &QGroupBox::toggled, this, &SGFPreview::reloadPreview);
	connect (fileDialog, &QFileDialog::currentChanged, this, &SGFPreview::setPath);
	connect (fileDialog, &QFileDialog::accepted, this, &QDialog::accept);
	connect (fileDialog, &QFileDialog::rejected, this, &QDialog::reject);
	boardView->reset_game (m_game);
	boardView->set_show_coords (false);
}

SGFPreview::~SGFPreview ()
{
}

void SGFPreview::clear ()
{
	boardView->reset_game (m_empty_game);
	m_game = nullptr;

	// ui->displayBoard->clearData ();

	File_WhitePlayer->setText("");
	File_BlackPlayer->setText("");
	File_Date->setText("");
	File_Handicap->setText("");
	File_Result->setText("");
	File_Komi->setText("");
	File_Size->setText("");
	File_Event->setText("");
	File_Round->setText("");
}

QStringList SGFPreview::selected ()
{
	return fileDialog->selectedFiles ();
}

void SGFPreview::setPath(QString path)
{
	clear ();

	try {
		QFile f (path);
		f.open (QIODevice::ReadOnly);
		// IOStreamAdapter adapter (&f);
		sgf *sgf = load_sgf (f);
		if (overwriteSGFEncoding->isChecked ()) {
			m_game = sgf2record (*sgf, QTextCodec::codecForName (encodingList->currentText ().toLatin1 ()));
		} else {
            f.seek(0);
            auto        data = f.readAll();
            QStringList bom  = {"\x00\x00\xfe\xff", "\xff\xfe\x00\x00", "\xef\xbb\xbf", "\xff\xfe", "\xfe\xff"};
            for (const auto &b : bom)
            {
                if (data.left(b.length()) == b)
                {
                    data.mid(b.length()); // remove BOM
                    break;
                }
            }
            QTextCodec *codec = nullptr;
            for (int i = 0; i < encodingList->count(); i++)
            {
                QTextCodec::ConverterState state;
                auto *                     c    = QTextCodec::codecForName(encodingList->itemText(i).toLatin1());
                const QString              text = c->toUnicode(data.constData(), data.size(), &state);
                if (state.invalidChars == 0)
                {
                    codec = c;
                    break;
                }
            }
            m_game = sgf2record(*sgf, codec);
        }
        m_game->set_filename(path.toStdString());

        boardView->reset_game(m_game);
        game_state *st = m_game->get_root();
        for (int i = 0; i < 20 && st->n_children() > 0; i++)
            st = st->next_primary_move();
        boardView->set_displayed(st);

        const game_info &info = m_game->info();
        File_WhitePlayer->setText(QString::fromStdString(info.name_w));
        File_BlackPlayer->setText(QString::fromStdString(info.name_b));
        File_Date->setText(QString::fromStdString(info.date));
        File_Handicap->setText(QString::number(info.handicap));
        File_Result->setText(QString::fromStdString(info.result));
        File_Komi->setText (QString::number (info.komi));
		File_Size->setText (QString::number (st->get_board ().size_x ()));
		File_Event->setText (QString::fromStdString (info.event));
		File_Round->setText (QString::fromStdString (info.round));
	} catch (...) {
	}
}

void SGFPreview::reloadPreview ()
{
	auto files = fileDialog->selectedFiles ();
	if (!files.isEmpty ())
		setPath (files.at (0));
}

void SGFPreview::accept ()
{
	QDialog::accept ();
}
