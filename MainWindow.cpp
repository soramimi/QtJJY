#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <memory>
#include <QDateTime>
#include <QLabel>
#include <QTimer>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>

static const double  PI2 = M_PI * 2;

struct MainWindow::Private {
	QLabel *status_label;
	QTime time;

	bool playing = false;
	int volume = 32760;
	int sample_fq = 48000;
	double tone_fq = 40000 / 3.0;
	double phase = 0;
	std::shared_ptr<QAudioOutput> audio_output;
	QIODevice *device = nullptr;

	QTimer timer_jjy;
	QDateTime dt_start;
	int jjy_count = 0;
	std::vector<char> jjy_data;
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);

	m->status_label = new QLabel(this);
	ui->statusBar->addWidget(m->status_label);

	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(1);
	format.setCodec("audio/pcm");
	format.setSampleRate(m->sample_fq);
	format.setSampleSize(16);
	format.setSampleType(QAudioFormat::SignedInt);

	m->audio_output = std::make_shared<QAudioOutput>(format);
	m->audio_output->setBufferSize(4800);
	m->device = m->audio_output->start();

	startTimer(1);

	connect(&m->timer_jjy, &QTimer::timeout, this, &MainWindow::timer_jjy);
}

MainWindow::~MainWindow()
{
	delete m;
	delete ui;
}

void MainWindow::setStatusLabel(QString const &text)
{
	m->status_label->setText(text);
}

void MainWindow::outputAudio()
{
	if (!m->playing) {
		return;
	}

	std::vector<int16_t> buf;

	while (1) {
		int n = m->audio_output->bytesFree();
		n /= sizeof(int16_t);
		const int N = 96;
		if (n < N) return;

		buf.resize(n);

		double add = PI2 * m->tone_fq / m->sample_fq;
		for (int i = 0; i < n; i++) {
			int v = 0;
			if (add != 0) {
				v += (sin(m->phase) < 0 ? -1 : 1) * m->volume;
			}
			buf[i] = v;
			m->phase += add;
			while (m->phase >= PI2) {
				m->phase -= PI2;
			}
		}

		m->device->write((char const *)&buf[0], n * sizeof(int16_t));
	}
}

void MainWindow::timerEvent(QTimerEvent *)
{
	{
		const QTime t = QTime::currentTime();
		if (m->time != t) {
			m->time = t;
			QString s = QString::asprintf("%02d:%02d:%02d", t.hour(), t.minute(), t.second());
			setStatusLabel(s);
		}
	}
	outputAudio();
}

void MainWindow::setTone(bool on)
{
	m->playing = on;
}

unsigned long convert_ymd_to_cjd(int year, int month, int day)
{
	if (month < 3) {
		month += 9;
		year--;
	} else {
		month -= 3;
	}
	year += 4800;
	int c = year / 100;
	return c * 146097 / 4 + (year - c * 100) * 1461 / 4 + (153 * month + 2) / 5 + day - 32045;
}

bool parity(uint64_t bits)
{
	uint64_t l, h;
	l = bits & 0x5555555555555555;
	h = (bits & 0xaaaaaaaaaaaaaaaa) >> 1;
	bits = l + h;
	l = bits & 0x3333333333333333;
	h = (bits & 0xcccccccccccccccc) >> 2;
	bits = l + h;
	l = bits & 0x0f0f0f0f0f0f0f0f;
	h = (bits & 0xf0f0f0f0f0f0f0f0) >> 4;
	bits = l + h;
	l = bits & 0x00ff00ff00ff00ff;
	h = (bits & 0xff00ff00ff00ff00) >> 8;
	bits = l + h;
	l = bits & 0x0000ffff0000ffff;
	h = (bits & 0xffff0000ffff0000) >> 16;
	bits = l + h;
	l = bits & 0x00000000ffffffff;
	h = (bits & 0xffffffff00000000) >> 32;
	bits = l + h;
	return bits & 1;
}

void MainWindow::startJJY()
{
	QDateTime dt = QDateTime::currentDateTime();
	int ms = 1000 - dt.time().msec();
	m->dt_start = dt.addMSecs(ms + 1000);
	m->timer_jjy.setSingleShot(true);
	m->timer_jjy.start(ms);
}

void MainWindow::timer_jjy()
{
	if (m->jjy_count == 0) {
		m->timer_jjy.setSingleShot(false);
		m->timer_jjy.start(10);
	}

	int len = 0;

	int seconds = m->jjy_count / 100;
	int count = seconds % 60;
	if (count == 0) {
		const QDateTime dt = m->dt_start.addSecs(seconds);
		unsigned long day = convert_ymd_to_cjd(dt.date().year(), dt.date().month(), dt.date().day());
		int wd = (day + 1) % 7;
		day = day - convert_ymd_to_cjd(dt.date().year(), 1, 1) + 1;

		uint64_t bits;
		auto push = [&](bool v){
			bits <<= 1;
			if (v) bits |= 1;
		};

		/* 01/52 */ push((dt.time().minute() / 10) & 4);
		/* 02/51 */ push((dt.time().minute() / 10) & 2);
		/* 03/50 */ push((dt.time().minute() / 10) & 1);
		/* 04/49 */ push(false);
		/* 05/48 */ push((dt.time().minute() % 10) & 8);
		/* 06/47 */ push((dt.time().minute() % 10) & 4);
		/* 07/46 */ push((dt.time().minute() % 10) & 2);
		/* 08/45 */ push((dt.time().minute() % 10) & 1);

		/* 10/44 */ push(false);
		/* 11/43 */ push(false);
		/* 12/42 */ push((dt.time().hour() / 10) & 2);
		/* 13/41 */ push((dt.time().hour() / 10) & 1);
		/* 14/40 */ push(false);
		/* 15/39 */ push((dt.time().hour() % 10) & 8);
		/* 16/38 */ push((dt.time().hour() % 10) & 4);
		/* 17/37 */ push((dt.time().hour() % 10) & 2);
		/* 18/36 */ push((dt.time().hour() % 10) & 1);

		/* 20/35 */ push(false);
		/* 21/34 */ push(false);
		/* 22/33 */ push((day / 100) & 2);
		/* 23/32 */ push((day / 100) & 1);
		/* 24/31 */ push(false);
		/* 25/30 */ push((day / 10 % 10) & 8);
		/* 26/29 */ push((day / 10 % 10) & 4);
		/* 27/28 */ push((day / 10 % 10) & 2);
		/* 28/27 */ push((day / 10 % 10) & 1);

		/* 30/26 */ push((day % 10) & 8);
		/* 31/25 */ push((day % 10) & 4);
		/* 32/24 */ push((day % 10) & 2);
		/* 33/23 */ push((day % 10) & 1);
		/* 34/22 */ push(false);
		/* 35/21 */ push(false);
		/* 36/20 */ push(false); // p1
		/* 37/19 */ push(false); // p2
		/* 38/18 */ push(false);

		/* 40/17 */ push(false);
		/* 41/16 */ push((dt.date().year() / 10 % 10) & 8);
		/* 42/15 */ push((dt.date().year() / 10 % 10) & 4);
		/* 43/14 */ push((dt.date().year() / 10 % 10) & 2);
		/* 44/13 */ push((dt.date().year() / 10 % 10) & 1);
		/* 45/12 */ push((dt.date().year() % 10) & 8);
		/* 46/11 */ push((dt.date().year() % 10) & 4);
		/* 47/10 */ push((dt.date().year() % 10) & 2);
		/* 48/09 */ push((dt.date().year() % 10) & 1);

		/* 50/08 */ push(wd & 4);
		/* 51/07 */ push(wd & 2);
		/* 52/06 */ push(wd & 1);
		/* 53/05 */ push(false);
		/* 54/04 */ push(false);
		/* 55/03 */ push(false);
		/* 56/02 */ push(false);
		/* 57/01 */ push(false);
		/* 58/00 */ push(false);

		if (parity(bits & (0xffLL << 45))) bits |= 1 << 19;
		if (parity(bits & (0x7fLL << 36))) bits |= 1 << 20;

		m->jjy_data.clear();
		m->jjy_data.reserve(60);
		for (int i = 0; i < 60; i++) {
			if (i == 0 || i % 10 == 9) {
				m->jjy_data.push_back('M');
			} else {
				bool v = bits & (1LL << 52);
				m->jjy_data.push_back(v ? '1' : '0');
				bits <<= 1;
			}
		}

		ui->widget->setData(m->jjy_data);
	}
	ui->widget->setPosition(count);

	if (count < m->jjy_data.size()) {
		char c = m->jjy_data[count];
		if (c == 'M') {
			len = 20;
		} else if (c == '0') {
			len = 80;
		} else if (c == '1') {
			len = 50;
		}
	}

	int n = m->jjy_count % 100;
	setTone(n < len);

	m->jjy_count++;
	if (m->jjy_count == 0) {
		startJJY();
	}
}

void MainWindow::on_pushButton_clicked()
{
	if (m->timer_jjy.isActive()) {
		m->timer_jjy.stop();
		ui->widget->setData({});
		setTone(false);
	} else {
		startJJY();
	}
	m->jjy_count = 0;
}
