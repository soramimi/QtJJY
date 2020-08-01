#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioOutput>
#include <stdint.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
private:
	Ui::MainWindow *ui;
	struct Private;
	Private *m;

	void setTone(bool on);
	void startJJY();
	void setStatusLabel(const QString &text);
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
private slots:
	void outputAudio();
	void timer_jjy();
	void on_pushButton_clicked();

protected:
	void timerEvent(QTimerEvent *);
};

#endif // MAINWINDOW_H
