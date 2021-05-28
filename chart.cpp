/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "chart.h"
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <QtCore/QRandomGenerator>
#include <QtCore/QDebug>

Chart::Chart(QGraphicsItem *parent, Qt::WindowFlags wFlags):
    QChart(QChart::ChartTypeCartesian, parent, wFlags),
    axisX(new QValueAxis()),
    axisY(new QValueAxis()),
    default_maxY(10),
    default_minY(0),
    cnt_x(0),
    points_per_frame(31)
{
    setTitle("数据曲线");
    legend()->hide();
    setAnimationOptions(QChart::AllAnimations);

    // 初始化坐标轴
    addAxis(axisX, Qt::AlignBottom);
    addAxis(axisY, Qt::AlignLeft);
    axisX->setTickCount(points_per_frame);
    axisX->setRange(0, points_per_frame-1);
    axisX->setVisible(false);
    axisY->setRange(default_minY, default_maxY);
}

Chart::~Chart()
{

}

void Chart::createNewSerie(const QString& name, Qt::GlobalColor color, qreal min, qreal max)
{
    auto s = new QSplineSeries(this);

    QPen pen(color);
    pen.setWidth(3);
    s->setPen(pen);

    series[name] = QSharedPointer<QSplineSeries>(s);
    addSeries(s);
    s->setVisible(false);
    s->attachAxis(axisX);
    s->attachAxis(axisY);

    yRange[name] =qMakePair(min, max);
}

void Chart::updateAxisY()
{
    QPair<int, int> r1 = qMakePair(default_minY, default_maxY);
    auto keys = active_series.keys();
    for(int i = 0; i < keys.size(); i++)                // 找到最大范围
    {
        auto& r2 = yRange[keys[i]];
        r1.first = (i == 0 || r2.first < r1.first) ? r2.first : r1.first;
        r1.second = (i == 0 || r2.second > r1.second) ? r2.second : r1.second;
    }
    axisY->setRange(r1.first, r1.second);
}

void Chart::enableSerie(const QString& name)
{
    if (active_series.find(name) != active_series.end()) return;
    auto iter = active_series.insert(name, series[name]);
    (*iter)->setVisible(true);
    updateAxisY();
}

void Chart::disableSerie(const QString& name)
{
    auto iter = active_series.find(name);
    if (iter == active_series.end()) return;
    (*iter)->setVisible(false);
    active_series.remove(name);
    updateAxisY();
}

void Chart::addNewPoint(const QString& name, qreal value)
{
    series[name]->append(cnt_x, value);
}

void Chart::updateFrame()
{
    if (cnt_x >= points_per_frame)
    {
        axisX->setRange(cnt_x-points_per_frame+1, cnt_x);
    }
    cnt_x++;
}
