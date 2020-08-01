#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioOutput>
#include <stdint.h>

namespace Ui {
class MainWindow;
}

enum class Playing : uint8_t {
	Stop,
	Space,
	Marker,
	Value0,
	Value1,
};

class MainWindow : public QMainWindow {
	Q_OBJECT
private:
	Ui::MainWindow *ui;
	struct Private;
	Private *m;

	void setLabelText(const QString &text);
	void makeData(const QDateTime &dt, std::vector<Playing> *out);
	void updateUI();
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
private slots:
	void outputAudio();
	void onInterval1ms();
	void onInterval1s();
	void on_pushButton_clicked();
};

#endif // MAINWINDOW_H
