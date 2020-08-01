#ifndef TIMECODE_H
#define TIMECODE_H

#include <QWidget>
#include "MainWindow.h"

class TimeCodeWidget : public QWidget {
	Q_OBJECT
private:
	std::vector<Playing> data_;
	int position_ = -1;
protected:
	void paintEvent(QPaintEvent *event);
public:
	explicit TimeCodeWidget(QWidget *parent = nullptr);
	void setData(const std::vector<Playing> &data);
	void setPosition(int pos);
};

#endif // TIMECODE_H
