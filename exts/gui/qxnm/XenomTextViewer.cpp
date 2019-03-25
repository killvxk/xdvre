#include "XenomTextViewer.h"
#include <qpen.h>
#include <qfontmetrics.h>
#include <qpainter.h>

XenomTextViewer::XenomTextViewer(xdv_handle handle, xdv::viewer::id id, QWidget *parent)
	: XenomPlainTextEdit(parent), viewer_handle_(handle), id_(id)
{
	setFont(QFont("Consolas", 9));
	setReadOnly(true);
	setMouseTracking(true);
	setWordWrapMode(QTextOption::NoWrap);
	highlighter_ = new SyntaxHighlighter(this->document());

	navigationArea_ = new NavigationLineArea(this);
	QObject::connect(this, &QPlainTextEdit::updateRequest, this, &XenomTextViewer::updateBlockArea);
	updateBlockAreaWidth(0);

	QFile f(".\\exts\\css\\style.qss");
	if (f.exists())
	{
		line_color_ = QColor("#000000").light(160);
		highlighter_->setLightColor();
	}
	else
	{
		line_color_ = QColor("#509CE4").light(160);
		highlighter_->setDarkColor();
	}
}

XenomTextViewer::~XenomTextViewer()
{
}

//
//
std::string mnString(unsigned long long ptr)
{
	xdv_handle ah = XdvGetArchitectureHandle();
	xdv_handle ih = XdvGetParserHandle();
	for (int i = 0; i < 3; ++i)
	{
		ptr = XdvGetBeforePtr(ah, ih, ptr);
		if (ptr == 0)
		{
			return "";
		}
	}

	unsigned long long start_ptr = ptr;
	std::string mnstr;
	for (int i = 0; i < 6; ++i)
	{
		xvar navivar = XdvExe("!dasmv.navistr -ptr:%I64x", ptr);
		char * navistr = (char *)ptrvar(navivar);
		if (navistr)
		{
			mnstr += navistr;
			free(navistr);
		}

		xvar codevar = XdvExe("!dasmv.codestr -ptr:%I64x", ptr);
		char * codestr = (char *)ptrvar(codevar);
		if (!codestr)
		{
			break;
		}

		if (ptr == start_ptr)
		{
			mnstr += " >> ";
		}
		mnstr += codestr;
		mnstr += "\n";
		free(codestr);

		xvar sizevar = XdvExe("!dasmv.codesize -ptr:%I64x", ptr);
		unsigned long long size = ullvar(sizevar);
		if (size == 0)
		{
			break;
		}
		ptr += size;
	}

	return mnstr;
}

bool XenomTextViewer::event(QEvent *e)
{
	if (e->type() != QEvent::ToolTip)
	{
		return QPlainTextEdit::event(e);
	}

	IObject *current_object = XdvGetObjectByHandle(viewer_handle_);
	if (!current_object)
	{
		return QPlainTextEdit::event(e);
	}

	QHelpEvent *he = (QHelpEvent *)e;
	QTextCursor cursor = cursorForPosition(he->pos());
	cursor.select(QTextCursor::WordUnderCursor);

#if 0
	if (!cursor.selectedText().isEmpty())
	{
		if (strstr(cursor.selectedText().toStdString().c_str(), "0x"))
		{
			char *end = nullptr;
			unsigned long long ptr = strtoull(cursor.selectedText().toStdString().c_str(), &end, 16);
			std::string mnstr = mnString(ptr);

			return true;
		}
	}
#endif

	return QPlainTextEdit::event(e);
}

void XenomTextViewer::mouseDoubleClickEvent(QMouseEvent *e)
{
	IViewer *viewer = (IViewer *)XdvGetObjectByHandle(viewer_handle_);
	if (!viewer)
	{
		QPlainTextEdit::mouseDoubleClickEvent(e);
		return;
	}

	QTextCursor check_cursor = textCursor();
	check_cursor.select(QTextCursor::LineUnderCursor);
	if (check_cursor.selectedText().size() == 0)
	{
		QPlainTextEdit::mouseDoubleClickEvent(e);
		return;
	}

	if (id_ == xdv::viewer::id::TEXT_VIEWER_B)
	{
		QTextCursor pre_cursor = textCursor();
		pre_cursor.select(QTextCursor::LineUnderCursor);
		viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_PRE_EVENT, pre_cursor.selectedText().toStdString());

		QTextCursor post_cursor = textCursor();
		post_cursor.select(QTextCursor::WordUnderCursor);
		viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_DOUBLE_CLICK, post_cursor.selectedText().toStdString());
	}
	else
	{
		QTextCursor cursor = textCursor();
		cursor.select(QTextCursor::LineUnderCursor);
		viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_DOUBLE_CLICK, cursor.selectedText().toStdString().c_str());
	}
}

void XenomTextViewer::wheelEvent(QWheelEvent *e)
{
	IViewer *viewer = (IViewer *)XdvGetObjectByHandle(viewer_handle_);
	if (!viewer)
	{
		QPlainTextEdit::wheelEvent(e);
		return;
	}

	if (id_ == xdv::viewer::id::TEXT_VIEWER_A)
	{
		QPlainTextEdit::wheelEvent(e);
		return;
	}

	if (e->delta() > 0) // up
	{
		viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_UP, "");
	}
	else // down
	{
		viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_DOWN, "");
	}
}

void XenomTextViewer::keyPressEvent(QKeyEvent *e)
{
	IViewer *viewer = (IViewer *)XdvGetObjectByHandle(viewer_handle_);
	if (!viewer)
	{
		QPlainTextEdit::keyPressEvent(e);
		return;
	}

	if (id_ == xdv::viewer::id::TEXT_VIEWER_A)
	{
		QPlainTextEdit::keyPressEvent(e);
		return;
	}

	switch (e->key())
	{
	case Qt::Key_Backspace:
		viewer->Update(xdv::status::id::XENOM_UPDATE_STSTUS_BACKSPACE, "");
		break;

	case Qt::Key_Space:
	{
		if (id_ == xdv::viewer::id::TEXT_VIEWER_B)
		{
			QTextCursor pre_cursor = textCursor();
			pre_cursor.select(QTextCursor::LineUnderCursor);
			viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_PRE_EVENT, pre_cursor.selectedText().toStdString());

			QTextCursor post_cursor = textCursor();
			post_cursor.select(QTextCursor::WordUnderCursor);
			viewer->Update(xdv::status::id::XENOM_UPDATE_STSTUS_SPACE, post_cursor.selectedText().toStdString());
		}
		else
		{
			QTextCursor cursor = textCursor();
			cursor.select(QTextCursor::LineUnderCursor);
			viewer->Update(xdv::status::id::XENOM_UPDATE_STSTUS_SPACE, cursor.selectedText().toStdString().c_str());
		}

		break;
	}

	case Qt::Key_Up:
	{
		viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_UP, "");
		break;
	}

	case Qt::Key_Down:
	{
		viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_DOWN, "");
		break;
	}

	default:
		QPlainTextEdit::keyPressEvent(e);
		break;
	}
}

void XenomTextViewer::mousePressEvent(QMouseEvent *e)
{
	QPlainTextEdit::mousePressEvent(e);
}

void XenomTextViewer::mouseReleaseEvent(QMouseEvent *e)
{
	syntaxHighlight();
	QPlainTextEdit::mouseReleaseEvent(e);
}

void XenomTextViewer::contextMenuEvent(QContextMenuEvent * e)
{
	QMenu menu("menu", this);
	QList<QAction *> actions = this->actions();
	for (auto action : actions)
	{
		menu.addAction(action);
	}
	menu.addSeparator();

	for (int i = 0; i < context_menu_vector_.size(); ++i)
	{
		menu.addMenu(context_menu_vector_[i]);
	}
	menu.exec(mapToGlobal(e->pos()));
}

void XenomTextViewer::resizeEvent(QResizeEvent *e)
{
	QPlainTextEdit::resizeEvent(e);

	QRect cr = contentsRect();
	navigationArea_->setGeometry(QRect(cr.left(), cr.top(), blockAreaWidth(), cr.height()));
}

// -------------------------------------------------
//
void XenomTextViewer::updateText(QString str)
{
	this->clear();
	this->insertPlainText(str);
}

void XenomTextViewer::clearText()
{
	this->clear();
}

void XenomTextViewer::syntaxHighlight()
{
	QTextEdit::ExtraSelection selection;
	selection.format.setBackground(line_color_);
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	selection.cursor = textCursor();
	selection.cursor.clearSelection();

	//
	//
	QTextCursor cursor = textCursor();
	cursor.select(QTextCursor::WordUnderCursor);

	QTextEdit::ExtraSelection current_word;
	current_word.format.setBackground(line_color_);
	current_word.cursor = cursor;

	QList<QTextEdit::ExtraSelection> extra_selections;
	extra_selections.append(selection);
	extra_selections.append(current_word);
	setExtraSelections(extra_selections);

	//
	// highlighting same text 
	QString select = cursor.selectedText();
	if (strlen(select.toStdString().c_str()) > 1 && strcmp(select.toStdString().c_str(), "00") != 0)
	{
		char *is_ptr = strstr((char *)select.toStdString().c_str(), "0x");
		if (is_ptr)
		{
			is_ptr += 2;
			select = is_ptr;
		}

		highlighter_->setHighlightedString(select);
	}
}

// -------------------------------------------------
//
void XenomTextViewer::addShortcutAction(char * menu, char * menu_icon, char * name, char * shortcut, char * icon)
{
	QList<QAction *> actions = this->actions();
	for (auto action : actions)
	{
		if (strstr(action->text().toStdString().c_str(), name))
		{
			return;
		}
	}

	for (int i = 0; i < context_menu_vector_.size(); ++i)
	{
		QList<QAction *> actions = context_menu_vector_[i]->actions();
		for (auto action : actions)
		{
			if (strstr(action->text().toStdString().c_str(), name))
			{
				return;
			}
		}
	}

	//
	//
	QAction * action = new QAction();
	if (shortcut)
	{
		char * end = nullptr;
		unsigned long sc = strtoul(shortcut, &end, 16);
		if (sc)
		{
			action->setShortcut(Qt::CTRL | sc);
		}
	}

	if (icon)
	{
		std::string icon_path = ":/xenom/Resources/";
		icon_path += icon;

		action->setIcon(QPixmap(icon_path.c_str()));
		action->setIconVisibleInMenu(true);
	}

	action->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
	action->setText(name);
	QObject::connect(action, &QAction::triggered, this, &XenomTextViewer::shortcutAction);

	if (menu)
	{
		QMenu * ctx = nullptr;
		for (int i = 0; i < context_menu_vector_.size(); ++i)
		{
			if (strstr(context_menu_vector_[i]->title().toStdString().c_str(), menu))
			{
				ctx = context_menu_vector_[i];
				break;
			}
		}

		if (!ctx)
		{
			ctx = new QMenu(menu, this);
			if (menu_icon)
			{
				std::string icon_path = ":/xenom/Resources/";
				icon_path += menu_icon;
				ctx->setIcon(QPixmap(icon_path.c_str()));
			}
		}
		ctx->addAction(action);
		context_menu_vector_.push_back(ctx);
	}
	else
	{
		this->addAction(action);
	}
}

void XenomTextViewer::shortcutAction()
{
	IViewer *viewer = (IViewer *)XdvGetObjectByHandle(viewer_handle_);
	if (!viewer)
	{
		return;
	}

	QTextCursor cursor = textCursor();
	cursor.select(QTextCursor::LineUnderCursor);

	QAction * qobj = (QAction*)this->sender();
	QString str = qobj->text();
	str += " -tag:";
	str += cursor.selectedText();
	viewer->Update(xdv::status::id::XENOM_UPDATE_STATUS_SHORTCUT, str.toStdString().c_str());
}

int XenomTextViewer::blockAreaWidth()
{
	int digits = 1;
	int max = qMax(1, blockCount());
	while (max >= 10) 
	{
		max /= 10;
		++digits;
	}

	int space = 7 + fontMetrics().width(QLatin1Char('9')) * digits;

	return space;
}

void XenomTextViewer::updateBlockAreaWidth(int)
{
	setViewportMargins(blockAreaWidth(), 0, 0, 0);
}

void XenomTextViewer::updateBlockArea(const QRect &rect, int dy)
{
	if (dy)
	{
		navigationArea_->scroll(0, dy);
	}
	else
	{
		navigationArea_->update(0, rect.y(), navigationArea_->width(), rect.height());
	}

	if (rect.contains(viewport()->rect()))
		updateBlockAreaWidth(0);
}

void XenomTextViewer::drawBlockPaintEvent(QPaintEvent *event)
{
	typedef struct _tag_point
	{
		unsigned long long dest;
		QLine current_line;
	}point;

	std::map<unsigned long long, point> points;
	QTextBlock block = this->document()->firstBlock();
	int interval = 2;
	for (int i = 0; i < this->document()->blockCount(); ++i)
	{
		if (block.text().size())
		{
			point p;
			char * end = nullptr;
			const char * ptr = block.text().toStdString().c_str();
			unsigned long long current = XdvToUll((char *)ptr);
			if (current)
			{
				const char * dest = strstr(block.text().toStdString().c_str(), "0x");
				if (dest)
				{
					p.dest = XdvToUll((char *)dest);
				}
				else
				{
					p.dest = 0;
				}

				QPoint p1 = QPoint(blockBoundingGeometry(block).topLeft().x(), blockBoundingGeometry(block).topLeft().y());
				QPoint p2 = QPoint(blockBoundingGeometry(block).topRight().x(), blockBoundingGeometry(block).topRight().y());

				p1.setX(p1.x() + blockAreaWidth() - interval);
				p1.setY(blockBoundingGeometry(block).center().y());

				p2.setY(blockBoundingGeometry(block).center().y());
				p.current_line = QLine(p1, p2);

				points.insert(std::pair<unsigned long long, point>(current, p));
				interval += 2;

				if (interval >= blockAreaWidth())
				{
					interval = 0;
				}
			}
		}

		block = block.next();
	}

	QVector<QLine> lines;
	for (auto it : points) // draw line
	{
		unsigned long long dest = it.second.dest;
		auto f = points.find(dest);
		if (f != points.end())
		{
			QLine src = it.second.current_line;
			QLine dest = f->second.current_line;
			QPoint dp1(src.p1().x(), dest.p1().y());
			dest.setP1(dp1);

			lines.push_back(src);
			lines.push_back(dest);
			lines.push_back(QLine(src.p1(), dest.p1()));
		}
	}

	QPen pen;
	pen.setStyle(Qt::DashLine);
	pen.setColor(Qt::blue);

	QPainter painter(navigationArea_);
	painter.setPen(pen);
	painter.drawLines(lines);
}
