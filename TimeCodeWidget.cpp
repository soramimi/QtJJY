#include "TimeCodeWidget.h"
#include <QPainter>

TimeCodeWidget::TimeCodeWidget(QWidget *parent)
	: QWidget(parent)
{

}

void TimeCodeWidget::setData(const std::vector<Playing> &data)
{
	data_ = data;
	position_ = -1;
	update();
}

void TimeCodeWidget::setPosition(int pos)
{
	position_ = pos;
	update();
}

void TimeCodeWidget::paintEvent(QPaintEvent *event)
{
	QPainter pr(this);
	pr.fillRect(rect(), Qt::black);
	int x0 = 1;
	int y = 1;
	int h = height() - 2;
	for (int i = 0; i < data_.size(); i++) {
		int x1 = (width() - 2) * (i + 1) / data_.size() + 1;
		Playing c = data_[i];
		int w = x1 - x0;
		QColor color;
		if (c == Playing::Marker) {
			color = i == position_ ? QColor(255, 0, 0) : QColor(128, 0, 0);
			w = w * 2 / 10;
		} else if (c == Playing::Value0) {
			color = i == position_ ? QColor(0, 255, 0) : QColor(0, 128, 0);
			w = w * 8 / 10;
		} else if (c == Playing::Value1) {
			color = i == position_ ? QColor(255, 255, 0) : QColor(128, 128, 0);
			w = w * 5 / 10;
		}
		if (color.isValid()) {
			pr.fillRect(x0, y, w, h, color);
		}
		x0 = x1;
	}
}
