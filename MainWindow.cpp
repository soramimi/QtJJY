#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <memory>
#include <QDateTime>
#include <QLabel>
#include <QTimer>
#include <QDebug>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>

namespace {

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

} // namespace

static const double PI2 = M_PI * 2;

struct MainWindow::Private {
	Playing playing = Playing::Stop;
	int volume = 32760;
	int sample_fq = 48000;
	double tone_fq = 40000 / 3.0;
	double phase = 0;
	std::shared_ptr<QAudioOutput> audio_output;
	QIODevice *device = nullptr;
	QTimer timer1ms;

	QTimer timer1s;
	QDateTime next_time;
	std::vector<Playing> jjy_data;
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);

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

	connect(&m->timer1ms, &QTimer::timeout, this, &MainWindow::onInterval1ms);
	connect(&m->timer1s, &QTimer::timeout, this, &MainWindow::onInterval1s);
	m->timer1ms.start(1);

	updateUI();

	const QDateTime now = QDateTime::currentDateTime();
	auto ms = 1000 - now.time().msec();
	m->timer1s.setSingleShot(true);
	m->timer1s.start(ms);
}

MainWindow::~MainWindow()
{
	delete m;
	delete ui;
}

void MainWindow::setLabelText(QString const &text)
{
	ui->label->setText(text);
}

void MainWindow::updateUI()
{
	ui->pushButton->setText(m->playing == Playing::Stop ? tr("Start") : tr("Stop"));
}

void MainWindow::outputAudio()
{
	if (m->playing == Playing::Stop) {
		return;
	}

	int volume = m->volume;
	if (m->playing == Playing::Space) {
		volume /= 10;
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
				v += m->phase < M_PI ? volume : -volume;
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

void MainWindow::onInterval1ms()
{
	outputAudio();
}

void MainWindow::makeData(QDateTime const &dt, std::vector<Playing> *out)
{
	out->clear();
	out->reserve(60);

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

	for (int i = 0; i < 60; i++) {
		if (i == 0 || i % 10 == 9) {
			out->push_back(Playing::Marker);
		} else {
			bool v = bits & (1LL << 52);
			out->push_back(v ? Playing::Value1 : Playing::Value0);
			bits <<= 1;
		}
	}

	ui->widget->setData(m->jjy_data);
}

void MainWindow::onInterval1s()
{
	const QDateTime now = QDateTime::currentDateTime();

	{
		QTime t = now.time();
		QString s = QString::asprintf("%02d:%02d:%02d", t.hour(), t.minute(), t.second());
		setLabelText(s);
	}

	int ms = 0;
	if (m->playing == Playing::Marker || m->playing == Playing::Value0 || m->playing == Playing::Value1) {
		m->next_time = m->next_time.addSecs(1);
		ms = now.msecsTo(m->next_time);
		m->playing = Playing::Space;
	} else if (m->playing == Playing::Space) {
		makeData(m->next_time, &m->jjy_data);
		const int second = m->next_time.time().second();
		ui->widget->setPosition(second);
		if (second < m->jjy_data.size()) {
			m->playing = m->jjy_data[second];
			if (m->playing == Playing::Marker) {
				ms = 200;
			} else if (m->playing == Playing::Value0) {
				ms = 800;
			} else if (m->playing == Playing::Value1) {
				ms = 500;
			}
		}
	}
	if (ms == 0) {
		ms = 1000 - now.time().msec();
	}
	m->timer1s.setSingleShot(true);
	m->timer1s.start(ms);
}

void MainWindow::on_pushButton_clicked()
{
	if (m->playing == Playing::Stop) {
		const QDateTime now = QDateTime::currentDateTime();
		int ms = 1000 - now.time().msec();
		m->next_time = now.addMSecs(ms);
		makeData(now, &m->jjy_data);
		m->timer1s.setSingleShot(true);
		m->timer1s.stop();
		m->timer1s.start(ms);
		m->playing = Playing::Space;
	} else {
		ui->widget->setData({});
		m->playing = Playing::Stop;
	}
	updateUI();
}
