// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2007 Licq developers
 *
 * Licq is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Licq is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Licq; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "calendar.h"

#include "config.h"

#ifdef __GLIBC__
#include <langinfo.h>
#endif

#include <QPainter>
#include <QTextCharFormat>


using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::Calendar */

Calendar::Calendar(QWidget* parent)
    : QCalendarWidget(parent)
{
#ifdef __GLIBC__
  // Non-standard locale parameter available in gnu libc only
  int firstday = *nl_langinfo(_NL_TIME_FIRST_WEEKDAY);
  if (firstday > 0)
  {
    // locale data uses: 1=Sunday, 2=Monday..., 7=Saturday
    // Qt data uses: 1=Monday, 2=Tuesday..., 7=Sunday
    firstday -= 1;
    if (firstday == 0)
      firstday = Qt::Sunday;
    setFirstDayOfWeek(static_cast<Qt::DayOfWeek>(firstday));
  }
  else
#endif
    setFirstDayOfWeek(Qt::Monday);
}

void Calendar::markDate(QDate date)
{
  QTextCharFormat textFormat = dateTextFormat(date);
  // Mark dates with bold
  textFormat.setFontWeight(QFont::Bold);
  // Background must be transparent to not overwrite the elipse
  textFormat.setBackground(Qt::transparent);
  setDateTextFormat(date, textFormat);
}

void Calendar::paintCell(QPainter* painter, const QRect& rect, const QDate& date) const
{
  QTextCharFormat format = dateTextFormat(date);
  if (format.fontWeight() == QFont::Bold)
  {
    painter->save();
    const int adjust = 1;
    QRect center = rect.adjusted(adjust, adjust, -adjust, -adjust);
    painter->setPen(Qt::NoPen);
    painter->setRenderHints(painter->renderHints() | QPainter::Antialiasing);
    painter->setBrush(Qt::yellow);
    painter->drawEllipse(center);
    painter->restore();
  }

  QCalendarWidget::paintCell(painter, rect, date);
}