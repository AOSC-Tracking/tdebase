/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void MultiMeterSettingsWidget::init()
{
  m_lowerLimit->setValidator(new KDoubleValidator(m_lowerLimit));
  m_upperLimit->setValidator(new KDoubleValidator(m_upperLimit));

  m_title->setFocus();
}

TQString MultiMeterSettingsWidget::title()
{
    return m_title->text();
}


bool MultiMeterSettingsWidget::showUnit()
{
    return m_showUnit->isChecked();
}


bool MultiMeterSettingsWidget::lowerLimitActive()
{
    return m_lowerLimitActive->isChecked();
}


double MultiMeterSettingsWidget::lowerLimit()
{
    return m_lowerLimit->text().toDouble();
}


bool MultiMeterSettingsWidget::upperLimitActive()
{
    return m_upperLimitActive->isChecked();
}


double MultiMeterSettingsWidget::upperLimit()
{
    return m_upperLimit->text().toDouble();
}


TQColor MultiMeterSettingsWidget::normalDigitColor()
{
    return m_normalDigitColor->color();
}


TQColor MultiMeterSettingsWidget::alarmDigitColor()
{
    return m_alarmDigitColor->color();
}


TQColor MultiMeterSettingsWidget::meterBackgroundColor()
{
    return m_backgroundColor->color();
}


void MultiMeterSettingsWidget::setTitle( const TQString &s )
{
    m_title->setText(s);
}


void MultiMeterSettingsWidget::setShowUnit( bool b )
{
    m_showUnit->setChecked(b);
}


void MultiMeterSettingsWidget::setLowerLimitActive( bool b )
{
    m_lowerLimitActive->setChecked(b);
}


void MultiMeterSettingsWidget::setLowerLimit( double d )
{
    m_lowerLimit->setText(TQString("%1").arg(d));
}


void MultiMeterSettingsWidget::setUpperLimitActive( bool b )
{
    m_upperLimitActive->setChecked(b);
}


void MultiMeterSettingsWidget::setUpperLimit( double d )
{
    m_upperLimit->setText(TQString("%1").arg(d));
}


void MultiMeterSettingsWidget::setNormalDigitColor( const TQColor &c )
{
    m_normalDigitColor->setColor(c);
}


void MultiMeterSettingsWidget::setAlarmDigitColor( const TQColor &c )
{
    m_alarmDigitColor->setColor(c);
}


void MultiMeterSettingsWidget::setMeterBackgroundColor( const TQColor &c )
{
    m_backgroundColor->setColor(c);
}
