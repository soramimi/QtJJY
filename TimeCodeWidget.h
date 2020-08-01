#ifndef TIMECODE_H
#define TIMECODE_H

#include <QWidget>

class TimeCodeWidget : public QWidget {
	Q_OBJECT
private:
	std::vector<char> data_;
	int position_ = -1;
protected:
	void paintEvent(QPaintEvent *event);
public:
	explicit TimeCodeWidget(QWidget *parent = nullptr);
	void setData(std::vector<char> const &data);
	void setPosition(int pos);
};

#endif // TIMECODE_H
