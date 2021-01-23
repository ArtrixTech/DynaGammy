
#include "RangeSlider.h"
#include "defs.h"
#include "cfg.h"

namespace
{

const int scHandleSideLength = 11;
const int scSliderBarHeight = 3;
const int scLeftRightMargin = 1;

}


RangeSlider::RangeSlider(QWidget* aParent)
    : QWidget(aParent),
      mMinimum(100),
      mMaximum(cfg["brt_extend"].get<bool>() ? brt_steps_max * 2 : brt_steps_max),
      mLowerValue(cfg["brt_min"].get<int>()),
      mUpperValue(cfg["brt_max"].get<int>()),
      mFirstHandlePressed(false),
      mSecondHandlePressed(false),
      mInterval(mMaximum - mMinimum),
      mBackgroudColorEnabled(QColor(35, 112, 145)),
      mBackgroudColorDisabled(QColor(15, 92, 125)),
      mBackgroudColor(mBackgroudColorEnabled)
{
    setMouseTracking(false);
}

void RangeSlider::paintEvent(QPaintEvent* aEvent)
{
	Q_UNUSED(aEvent);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// Background
	QRectF backgroundRect = QRectF(scLeftRightMargin, (height() - scSliderBarHeight) / 2, width() - scLeftRightMargin * 2, scSliderBarHeight);

	QColor teal      = QColor(35, 112, 145);
	QColor grey      = QColor(76, 76, 76);
	QColor whiteblue = QColor(235, 225, 255);
	//QColor darkteal  = QColor(15, 92, 125);

	painter.setPen(QPen(grey, qreal(0)));
	painter.setBrush(grey);
	painter.drawRoundedRect(backgroundRect, qreal(1), qreal(1));

	painter.setPen(QPen(whiteblue, qreal(0)));
	painter.setBrush(whiteblue);

	QRectF leftHandleRect = firstHandleRect();
	painter.drawRoundedRect(leftHandleRect, 4, 4);
	QRectF rightHandleRect = secondHandleRect();
	painter.drawRoundedRect(rightHandleRect, 4, 4);

	painter.setPen(QPen(teal, qreal(0)));
	painter.setBrush(QBrush(teal));

	QRectF selectedRect(backgroundRect);
	selectedRect.setLeft(leftHandleRect.right() + 0.5);
	selectedRect.setRight(rightHandleRect.left() - 0.5);

	painter.drawRect(selectedRect);
}

QRectF RangeSlider::firstHandleRect() const
{
    float percentage = (mLowerValue - mMinimum) * 1.0 / mInterval;
    return handleRect(percentage * validWidth() + scLeftRightMargin);
}

QRectF RangeSlider::secondHandleRect() const
{
    float percentage = (mUpperValue - mMinimum) * 1.0 / mInterval;
    return handleRect(percentage * validWidth() + scLeftRightMargin + scHandleSideLength);
}

QRectF RangeSlider::handleRect(int aValue) const
{
    return QRect(aValue, (height()-scHandleSideLength) / 2, scHandleSideLength, scHandleSideLength);
}

void RangeSlider::mousePressEvent(QMouseEvent* aEvent)
{
    if(aEvent->buttons() & Qt::LeftButton)
    {
	mSecondHandlePressed = secondHandleRect().contains(aEvent->pos());
	mFirstHandlePressed = !mSecondHandlePressed && firstHandleRect().contains(aEvent->pos());
	if(mFirstHandlePressed)
	{
	    mDelta = aEvent->pos().x() - (firstHandleRect().x() + scHandleSideLength / 2);
	}
	else if(mSecondHandlePressed)
	{
	    mDelta = aEvent->pos().x() - (secondHandleRect().x() + scHandleSideLength / 2);
	}
	if(aEvent->pos().y() >= 2
	   && aEvent->pos().y() <= height()- 2)
	{
	    int step = mInterval / 10 < 1 ? 1 : mInterval / 10;
	    if(aEvent->pos().x() < firstHandleRect().x())
	    {
		setLowerValue(mLowerValue - step);
	    }
	    else if(aEvent->pos().x() > firstHandleRect().x() + scHandleSideLength
		    && aEvent->pos().x() < secondHandleRect().x())
	    {
		if(aEvent->pos().x() - (firstHandleRect().x() + scHandleSideLength) <
		   (secondHandleRect().x() - (firstHandleRect().x() + scHandleSideLength)) / 2)
		{
		    if(mLowerValue + step < mUpperValue)
		    {
			setLowerValue(mLowerValue + step);
		    }
		    else
		    {
			setLowerValue(mUpperValue);
		    }
		}
		else
		{
		    if(mUpperValue - step > mLowerValue)
		    {
			setUpperValue(mUpperValue - step);
		    }
		    else
		    {
			setUpperValue(mLowerValue);
		    }
		}
	    }
	    else if(aEvent->pos().x() > secondHandleRect().x() + scHandleSideLength)
	    {
		setUpperValue(mUpperValue + step);
	    }
	}
    }
}

void RangeSlider::mouseMoveEvent(QMouseEvent* aEvent)
{
    if(aEvent->buttons() & Qt::LeftButton)
    {
	if(mFirstHandlePressed)
	{
	    if(aEvent->pos().x() - mDelta + scHandleSideLength / 2 <= secondHandleRect().x())
	    {
		setLowerValue((aEvent->pos().x() - mDelta - scLeftRightMargin - scHandleSideLength / 2) * 1.0 / validWidth() * mInterval + mMinimum);
	    }
	    else
	    {
		setLowerValue(mUpperValue);
	    }
	}
	else if(mSecondHandlePressed)
	{
	    if(firstHandleRect().x() + scHandleSideLength * 1.5 <= aEvent->pos().x() - mDelta)
	    {
		setUpperValue((aEvent->pos().x() - mDelta - scLeftRightMargin - scHandleSideLength / 2 - scHandleSideLength) * 1.0 / validWidth() * mInterval + mMinimum);
	    }
	    else
	    {
		setUpperValue(mLowerValue);
	    }
	}
    }
}

void RangeSlider::mouseReleaseEvent(QMouseEvent* aEvent)
{
    Q_UNUSED(aEvent);

    mFirstHandlePressed = false;
    mSecondHandlePressed = false;
}

void RangeSlider::changeEvent(QEvent* aEvent)
{
    if(aEvent->type() == QEvent::EnabledChange)
    {
	if(isEnabled())
	{
	    mBackgroudColor = mBackgroudColorEnabled;
	}
	else
	{
	    mBackgroudColor = mBackgroudColorDisabled;
	}
	update();
    }
}

QSize RangeSlider::minimumSizeHint() const
{
    return QSize(scHandleSideLength * 2 + scLeftRightMargin * 2, scHandleSideLength);
}

int RangeSlider::GetMinimun() const
{
    return mMinimum;
}

void RangeSlider::SetMinimum(int aMinimum)
{
    setMinimum(aMinimum);
}

int RangeSlider::GetMaximun() const
{
    return mMaximum;
}

void RangeSlider::SetMaximum(int aMaximum)
{
    setMaximum(aMaximum);
}

int RangeSlider::GetLowerValue() const
{
    return mLowerValue;
}

void RangeSlider::SetLowerValue(int aLowerValue)
{
    setLowerValue(aLowerValue);
}

int RangeSlider::GetUpperValue() const
{
    return mUpperValue;
}

void RangeSlider::SetUpperValue(int aUpperValue)
{
    setUpperValue(aUpperValue);
}

void RangeSlider::setLowerValue(int aLowerValue)
{
    if(aLowerValue > mMaximum)
    {
	aLowerValue = mMaximum;
    }

    if(aLowerValue < mMinimum)
    {
	aLowerValue = mMinimum;
    }

    mLowerValue = aLowerValue;
    emit lowerValueChanged(mLowerValue);

    update();
}

void RangeSlider::setUpperValue(int aUpperValue)
{
    if(aUpperValue > mMaximum)
    {
	aUpperValue = mMaximum;
    }

    if(aUpperValue < mMinimum)
    {
	aUpperValue = mMinimum;
    }

    mUpperValue = aUpperValue;
    emit upperValueChanged(mUpperValue);

    update();
}

void RangeSlider::setMinimum(int aMinimum)
{
    if(aMinimum <= mMaximum)
    {
	mMinimum = aMinimum;
    }
    else
    {
	int oldMax = mMaximum;
	mMinimum = oldMax;
	mMaximum = aMinimum;
    }
    mInterval = mMaximum - mMinimum;
    update();

    setLowerValue(mMinimum);
    setUpperValue(mMaximum);

    emit rangeChanged(mMinimum, mMaximum);
}

void RangeSlider::setMaximum(int aMaximum)
{
    if(aMaximum >= mMinimum)
    {
	mMaximum = aMaximum;
    }
    else
    {
	int oldMin = mMinimum;
	mMaximum = oldMin;
	mMinimum = aMaximum;
    }
    mInterval = mMaximum - mMinimum;
    update();

    setLowerValue(mMinimum);
    setUpperValue(mMaximum);

    emit rangeChanged(mMinimum, mMaximum);
}

int RangeSlider::validWidth() const
{
    return width() - scLeftRightMargin * 2 - scHandleSideLength * 2;
}

void RangeSlider::SetRange(int aMinimum, int mMaximum)
{
    setMinimum(aMinimum);
    setMaximum(mMaximum);
}
