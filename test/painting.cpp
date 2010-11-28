#include <QtGui>
#include <iostream>

//use this rectangle in source image
const int src_xoff = 15;
const int src_yoff = 23;
const int src_width = 600;
const int src_height = 500;
const QRect src_rect(src_xoff, src_yoff, src_width, src_height);


class Widget : public QWidget {
public:
	Widget():
		QWidget(0)
	{
		msecs = paints = mode = 0;
		img = QImage("stripes.png");

		QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()),
			this, SLOT(repaint()));
		timer->start(150);

	}
protected:
	virtual void paintEvent(QPaintEvent*);
private:
	QImage img;
	int msecs, paints, mode;
};

void Widget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	//painter.setRenderHint(QPainter::SmoothPixmapTransform);

	QTime t;
	t.start();

	switch(mode) {
//scaled (fit to window)
	case 0: //this is how small updates in 0.6 are done
		painter.drawImage(rect(),
			img.copy(src_rect)
			.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		break;
	case 1: //fast transformation
		painter.drawImage(rect(),
			img.copy(src_rect)
			.scaled(size(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
		break;
	case 2: //tell drawImage() that no further scaling is required
	//why different from 1?
		painter.drawImage(rect().topLeft(),
			img.copy(src_rect)
			.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		break;
	case 3: //avoid scaled()
		painter.drawImage(rect(),
			img.copy(src_rect));
		break;
	case 4: //avoid copy() and scaled()
		painter.drawImage(rect(), img, src_rect);
		break;

// not scaled - repeat preceeding tests with dimensions that just happen to require no transformation
// these are all comparable
	case 5: //this is how small updates in 0.6 are done
		painter.drawImage(rect(),
			img.copy(rect())
			.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		break;
	case 6: //fast transformation
		painter.drawImage(rect(),
			img.copy(rect())
			.scaled(size(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
		break;
	case 7: //tell drawImage() that no further scaling is required
		painter.drawImage(rect().topLeft(),
			img.copy(rect())
			.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		break;
	case 8: //avoid scaled()
		painter.drawImage(rect(),
			img.copy(rect()));
		break;
	case 9: //avoid copy() and scaled()
		painter.drawImage(rect(), img, rect());
		break;
//and a few extra
	case 10:
		painter.drawImage(rect().topLeft(), img, rect());
		break;
	case 11:
		painter.drawImage(rect().topLeft(), img);
		break;
	case 12:
		painter.drawImage(rect().topLeft(),
			img.copy(rect()));
		break;
	case 13: //5 with KeepAspectRatio
		painter.drawImage(rect(),
			img.copy(rect())
			.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
		break;
	}

	msecs += t.elapsed();
	paints++;

	const int paints_per_mode = 20;
	const int num_modes = 14;

	if(paints > paints_per_mode) {
		std::cout << mode << ":\t" << double(msecs)/paints << " msecs per paint\n";
		msecs = paints = 0;
		mode++;
		if(mode >= num_modes)
			close();
	}

}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	Widget w;
	w.show();
	
	return app.exec();
}
